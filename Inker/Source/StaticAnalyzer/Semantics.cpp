// Inker/Source/StaticAnalyzer/Semantics.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>

STATIC VOID SemanticEnum(STATIC_ANALYZER *Analyzer, ANALYZER_SYMBOL *Enum)
{
    SEMANTIC_TYPE   *Type = SemanticSymbolType(Analyzer, Enum);
    UINT64           Next = 0;
    SEMANTIC_CONTEXT Context;
    Context.Analyzer        = Analyzer;
    Context.Unit            = Enum->Unit;
    Context.Scope           = Enum->Members;
    Context.EnumInitializer = TRUE;
    for (ANALYZER_SYMBOL *Member : Enum->Members->Ordered)
    {
        SEMANTIC_VALUE Value;
        Value.Type            = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 32);
        Value.IntegerKnown    = TRUE;
        Value.Integer         = Next;
        AST_NODE *Initializer = Member->Node->EnumMember.Value;
        if (Initializer != NULLPTR)
        {
            Value = SemanticExpression(&Context, Initializer);
            if (Value.Type->Kind == SEMANTIC_KIND::ENUM)
            {
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 32);
            }
            if (!SemanticIntegral(Value.Type) || !Value.IntegerKnown)
            {
                SemanticReport(&Context, Initializer, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ENUM_CONSTANT_REQUIRED);
            }
            Value = SemanticConvert(&Context, Value, SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 32), Initializer);
        }
        Value.Type          = Type;
        Member->Type        = Type;
        Member->TypeChecked = TRUE;
        Member->Value       = Value;
        Member->Initialized = TRUE;
        Next                = (Value.Integer + 1) & SemanticMask(32);
    }
}

VOID SemanticCheck(STATIC_ANALYZER *Analyzer)
{
    for (CONST DIAGNOSTIC &Diagnostic : Analyzer->Diagnostics)
    {
        Analyzer->SemanticDiagnostics.insert(Diagnostic.Path + ":" + std::to_string(Diagnostic.Start) + ":" + std::to_string(Diagnostic.Length) + ":" + std::to_string(static_cast<UINT8>(Diagnostic.Level)) + ":" + Diagnostic.Message);
    }
    for (ANALYZER_SYMBOL &Symbol : Analyzer->Symbols)
    {
        if (Symbol.Kind == ANALYZER_SYMBOL_KIND::TYPE && Symbol.Node->Type == AST_NODE_TYPE::ENUM_DECLARATION)
        {
            SemanticEnum(Analyzer, &Symbol);
        }
    }
    for (ANALYZER_SYMBOL &Symbol : Analyzer->Symbols)
    {
        if (Symbol.Kind != ANALYZER_SYMBOL_KIND::ALIAS && Symbol.Kind != ANALYZER_SYMBOL_KIND::ERROR)
        {
            SemanticSymbolType(Analyzer, &Symbol);
        }
    }
    for (CONST auto &Unit : Analyzer->Units)
    {
        if (Unit->Valid)
        {
            SEMANTIC_CONTEXT Context;
            Context.Analyzer = Analyzer;
            Context.Unit     = Unit.get();
            Context.Scope    = Unit->Global;
            SemanticStatement(&Context, Unit->Root);
        }
    }
    for (ANALYZER_SYMBOL &Symbol : Analyzer->Symbols)
    {
        if (Symbol.Kind != ANALYZER_SYMBOL_KIND::FUNCTION)
        {
            continue;
        }
        SEMANTIC_TYPE              *Signature = SemanticSymbolType(Analyzer, &Symbol);
        std::vector<SEMANTIC_VALUE> Arguments;
        BOOLEAN                     Generic = FALSE;
        for (SEMANTIC_TYPE *Type : Signature->Parameters)
        {
            SEMANTIC_VALUE Value;
            Value.Type = Type;
            Arguments.push_back(Value);
            Generic = Generic || SemanticHasAuto(Type);
        }
        SEMANTIC_CONTEXT Context;
        Context.Analyzer       = Analyzer;
        Context.Unit           = Symbol.Unit;
        Context.Scope          = Symbol.Scope;
        Context.DefinitionOnly = Generic;
        SemanticCall(&Context, &Symbol, Arguments, Symbol.Node, Generic);
    }
    for (ANALYZER_SYMBOL &Symbol : Analyzer->Symbols)
    {
        BOOLEAN Variable = Symbol.Kind == ANALYZER_SYMBOL_KIND::VARIABLE || Symbol.Kind == ANALYZER_SYMBOL_KIND::PARAMETER || Symbol.Kind == ANALYZER_SYMBOL_KIND::FIELD;
        if (Variable && !Symbol.Used && Symbol.Access != TOKENS::TOKEN_KW_PUBLIC)
        {
            AnalyzerReport(Analyzer, Symbol.Unit, Symbol.Location, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::UNUSED_SYMBOL, Symbol.Name);
        }
    }
}

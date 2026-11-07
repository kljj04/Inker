// Inker/Source/StaticAnalyzer/Call.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>

SEMANTIC_TYPE *SemanticCall(SEMANTIC_CONTEXT *Caller, ANALYZER_SYMBOL *Function, CONST std::vector<SEMANTIC_VALUE> &Arguments, AST_NODE *Site, BOOLEAN DefinitionOnly)
{
    STATIC_ANALYZER *Analyzer  = Caller->Analyzer;
    SEMANTIC_TYPE   *Signature = SemanticSymbolType(Analyzer, Function);
    if (Arguments.size() != Signature->Parameters.size())
    {
        SemanticReport(Caller, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ARGUMENT_COUNT_MISMATCH, std::to_string(Signature->Parameters.size()), std::to_string(Arguments.size()));
        return SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    std::vector<SEMANTIC_TYPE *> Parameters;
    BOOLEAN                      Generic = FALSE;
    for (SIZE Index = 0; Index < Arguments.size(); Index++)
    {
        SEMANTIC_VALUE Value = SemanticConvert(Caller, Arguments[Index], Signature->Parameters[Index], Arguments[Index].Node == NULLPTR ? Site : Arguments[Index].Node);
        Generic              = Generic || SemanticHasAuto(Signature->Parameters[Index]);
        if (Value.Type->Kind == SEMANTIC_KIND::VOID)
        {
            SemanticReport(Caller, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE, "a function argument");
        }
        if (Value.Type->Kind == SEMANTIC_KIND::ERROR)
        {
            return Value.Type;
        }
        Parameters.push_back(SemanticHasAuto(Signature->Parameters[Index]) ? Value.Type : Signature->Parameters[Index]);
    }
    DefinitionOnly = DefinitionOnly || (Caller->DefinitionOnly && Generic);
    STRING Key     = std::to_string(reinterpret_cast<UINTN>(Function)) + (DefinitionOnly ? ":D" : ":C");
    for (SEMANTIC_TYPE *Type : Parameters)
    {
        Key += ":" + std::to_string(Type->ID);
    }
    auto Existing = Analyzer->VariantCache.find(Key);
    if (Existing != Analyzer->VariantCache.end())
    {
        SEMANTIC_VARIANT *Variant = Existing->second;
        if (Variant->Return != NULLPTR)
        {
            return Variant->Return;
        }
        if (Variant->Checking)
        {
            if (Signature->Return->Kind != SEMANTIC_KIND::AUTO)
            {
                return Signature->Return;
            }
            if (DefinitionOnly)
            {
                return Signature->Return;
            }
            SemanticReport(Caller, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::CANNOT_INFER_TYPE, "recursive AUTO return; specify an explicit return type");
            return SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
        }
    }
    if (Analyzer->CallDepth >= 128)
    {
        SemanticReport(Caller, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        return SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    Analyzer->Variants.emplace_back();
    SEMANTIC_VARIANT *Variant   = &Analyzer->Variants.back();
    Variant->Function           = Function;
    Variant->Parameters         = Parameters;
    Variant->DefinitionOnly     = DefinitionOnly;
    Variant->Checking           = TRUE;
    Analyzer->VariantCache[Key] = Variant;
    SEMANTIC_CONTEXT Context;
    Context.Analyzer                    = Analyzer;
    Context.Unit                        = Function->Unit;
    Context.Scope                       = Function->Members;
    Context.Function                    = Function;
    Context.Variant                     = Variant;
    Context.DefinitionOnly              = DefinitionOnly;
    AST_NODE            *Declaration    = Function->Node;
    BOOLEAN              Method         = Declaration->Type == AST_NODE_TYPE::METHOD_DECLARATION;
    CONST AST_NODE_LIST &ParameterNodes = Method ? Declaration->MethodDeclaration.Parameters : Declaration->FunctionDeclaration.Parameters;
    AST_NODE            *Body           = Method ? Declaration->MethodDeclaration.Body : Declaration->FunctionDeclaration.Body;
    for (SIZE Index = 0; Index < ParameterNodes.Count; Index++)
    {
        auto Symbol = Analyzer->Declarations.find(ParameterNodes.Items[Index]);
        if (Symbol != Analyzer->Declarations.end())
        {
            Context.Locals[Symbol->second] = Parameters[Index];
            Context.Initialized.insert(Symbol->second);
            Variant->Locals[Symbol->second] = Parameters[Index];
        }
    }
    Analyzer->CallDepth++;
    UINT32 Flow = SemanticStatement(&Context, Body);
    Analyzer->CallDepth--;
    SEMANTIC_TYPE *Return = Signature->Return;
    if (Return->Kind == SEMANTIC_KIND::AUTO)
    {
        Return = Context.Returns.empty() ? SemanticType(Analyzer, SEMANTIC_KIND::VOID) : Context.Returns.front().Type;
        for (CONST SEMANTIC_VALUE &Value : Context.Returns)
        {
            if (!SemanticCompatible(Return, Value.Type))
            {
                if (SemanticNumeric(Return) && SemanticNumeric(Value.Type))
                {
                    SemanticConvert(&Context, Value, Return, Value.Node);
                }
                else
                {
                    SemanticReport(&Context, Value.Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, SemanticName(Return), SemanticName(Value.Type));
                }
            }
        }
    }
    if ((Flow & 1) != 0 && Return->Kind != SEMANTIC_KIND::VOID && Return->Kind != SEMANTIC_KIND::ERROR && Return->Kind != SEMANTIC_KIND::AUTO)
    {
        SemanticReport(&Context, Declaration, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::MISSING_RETURN);
    }
    Variant->Return   = Return;
    Variant->Checking = FALSE;
    Variant->Checked  = TRUE;
    return Return;
}

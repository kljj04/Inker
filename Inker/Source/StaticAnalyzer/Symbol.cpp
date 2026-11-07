// Inker/Source/StaticAnalyzer/Symbol.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>

ANALYZER_SCOPE *AnalyzerNewScope(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, ANALYZER_SCOPE *Parent, ANALYZER_SYMBOL *Owner)
{
    Analyzer->Scopes.emplace_back();
    ANALYZER_SCOPE *Scope = &Analyzer->Scopes.back();
    Scope->Parent         = Parent;
    Scope->Owner          = Owner;
    Scope->Unit           = Unit;
    return Scope;
}

ANALYZER_SYMBOL *AnalyzerDeclare(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, ANALYZER_SYMBOL_KIND Kind, STRING_VIEW Name, AST_NODE *Node, TOKENS Access)
{
    STRING Key(Name);
    auto   Existing = Scope->Symbols.find(Key);

    if (Existing != Scope->Symbols.end())
    {
        ANALYZER_SYMBOL *Previous = Existing->second;

        if (Kind == ANALYZER_SYMBOL_KIND::NAMESPACE && Previous->Kind == Kind)
        {
            if (Node != NULLPTR)
            {
                Analyzer->Declarations[Node] = Previous;
            }

            return Previous;
        }

        AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Name), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::REDEFINED_SYMBOL, Name);
        AnalyzerReport(Analyzer, Previous->Unit, Previous->Location, DIAGNOSTIC_LEVEL::NOTE, DIAGNOSTIC_MESSAGE::PREVIOUS_DECLARATION, Name);
        return NULLPTR;
    }

    ANALYZER_SYMBOL *Outer = AnalyzerFind(Scope->Parent, Name);

    if (Outer != NULLPTR && Kind != ANALYZER_SYMBOL_KIND::NAMESPACE)
    {
        AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Name), DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::SHADOWED_SYMBOL, Name);
    }

    Analyzer->Symbols.emplace_back();
    ANALYZER_SYMBOL *Symbol = &Analyzer->Symbols.back();
    Symbol->Kind            = Kind;
    Symbol->Name            = Name;
    Symbol->Node            = Node;
    Symbol->Unit            = Scope->Unit;
    Symbol->Scope           = Scope;
    Symbol->Access          = Access;
    Symbol->Location        = AnalyzerLocation(Scope->Unit, Node, Name);
    Scope->Symbols.emplace(Key, Symbol);
    Scope->Ordered.push_back(Symbol);

    if (Node != NULLPTR)
    {
        Analyzer->Declarations[Node] = Symbol;
    }

    return Symbol;
}

ANALYZER_SYMBOL *AnalyzerFind(ANALYZER_SCOPE *Scope, STRING_VIEW Name)
{
    while (Scope != NULLPTR)
    {
        auto It = Scope->Symbols.find(STRING(Name));

        if (It != Scope->Symbols.end())
        {
            return It->second;
        }

        Scope = Scope->Parent;
    }

    return NULLPTR;
}

ANALYZER_SYMBOL *AnalyzerUnalias(ANALYZER_SYMBOL *Symbol)
{
    SIZE Depth = 0;

    while (Symbol != NULLPTR && Symbol->Kind == ANALYZER_SYMBOL_KIND::ALIAS && Symbol->Target != NULLPTR)
    {
        if (++Depth > 256)
        {
            return NULLPTR;
        }

        Symbol = Symbol->Target;
    }

    return Symbol;
}

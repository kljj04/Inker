// Inker/Source/StaticAnalyzer/Namespace.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>

ANALYZER_SYMBOL *AnalyzerQualified(ANALYZER_SCOPE *Scope, STRING_VIEW Path)
{
    ANALYZER_SYMBOL *Symbol = NULLPTR;

    while (!Path.empty() && Scope != NULLPTR)
    {
        SIZE        Dot  = Path.find('.');
        STRING_VIEW Part = Path.substr(0, Dot);
        auto        It   = Scope->Symbols.find(STRING(Part));

        if (It == Scope->Symbols.end())
        {
            return NULLPTR;
        }

        Symbol = AnalyzerUnalias(It->second);

        if (Symbol == NULLPTR || Dot == STRING_VIEW::npos)
        {
            return Symbol;
        }

        Path.remove_prefix(Dot + 1);
        Scope = Symbol->Members;
    }

    return NULLPTR;
}

STATIC ANALYZER_SYMBOL *AnalyzerExportPath(ANALYZER_SCOPE *Scope, STRING_VIEW Path, BOOLEAN *Inaccessible)
{
    while (!Path.empty() && Scope != NULLPTR)
    {
        SIZE Dot = Path.find('.');
        auto It  = Scope->Symbols.find(STRING(Path.substr(0, Dot)));

        if (It == Scope->Symbols.end())
        {
            return NULLPTR;
        }

        if (It->second->Kind != ANALYZER_SYMBOL_KIND::ERROR && It->second->Access != TOKENS::TOKEN_KW_PUBLIC)
        {
            *Inaccessible = TRUE;
            return NULLPTR;
        }

        ANALYZER_SYMBOL *Symbol = AnalyzerUnalias(It->second);

        if (Symbol == NULLPTR || Dot == STRING_VIEW::npos)
        {
            return Symbol;
        }

        Scope = Symbol->Members;
        Path.remove_prefix(Dot + 1);
    }

    return NULLPTR;
}

STATIC ANALYZER_SYMBOL *AnalyzerImportTarget(ANALYZER_UNIT *Unit, CONST AST_USE_DECLARATION &Use, BOOLEAN *Inaccessible)
{
    if (Use.Path.empty())
    {
        return Unit->Module;
    }

    return AnalyzerExportPath(Unit->Global, AnalyzerNormalizePath(Use.Path), Inaccessible);
}

STATIC BOOLEAN AnalyzerBindOne(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, CONST ANALYZER_IMPORT &Import, BOOLEAN Final)
{
    AST_NODE                  *Node    = Import.Node;
    CONST AST_USE_DECLARATION &Use     = Node->UseDeclaration;
    auto                       ScopeIt = Analyzer->NodeScopes.find(Node);

    if (ScopeIt == Analyzer->NodeScopes.end())
    {
        return TRUE;
    }

    ANALYZER_SCOPE *Scope = ScopeIt->second;
    STRING          Path  = AnalyzerNormalizePath(Use.Path);
    STRING          Alias = Use.Alias.empty() ? std::filesystem::path(STRING(Use.File)).stem().string() : STRING(Use.Alias);
    LEXER           AliasLexer;
    LexerInit(&AliasLexer, Alias);

    if (LexerNextToken(&AliasLexer) != TOKENS::TOKEN_IDENTIFIER || LexerNextToken(&AliasLexer) != TOKENS::TOKEN_EOF)
    {
        AnalyzerReport(Analyzer, Unit, AnalyzerLocation(Unit, Node, Use.File), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_IMPORT_ALIAS, Alias);
        return TRUE;
    }
    ANALYZER_SYMBOL *Target = NULLPTR;

    if (Import.Target != NULLPTR && Import.Target->Valid)
    {
        BOOLEAN Inaccessible = FALSE;
        Target               = AnalyzerImportTarget(Import.Target, Use, &Inaccessible);

        if (Inaccessible)
        {
            AnalyzerReport(Analyzer, Unit, AnalyzerLocation(Unit, Node, Use.Path), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL, Path);
        }
        else if (Target == NULLPTR)
        {
            if (!Final)
            {
                return FALSE;
            }

            AnalyzerReport(Analyzer, Unit, AnalyzerLocation(Unit, Node, Use.Path), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::IMPORT_NOT_FOUND, Path);
        }
        else if (!Use.Path.empty() && Target->Kind != ANALYZER_SYMBOL_KIND::NAMESPACE)
        {
            AnalyzerReport(Analyzer, Unit, AnalyzerLocation(Unit, Node, Use.Path), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::EXPECTED_NAMESPACE, Path);
            Target = NULLPTR;
        }
        else if (Target->Kind == ANALYZER_SYMBOL_KIND::ERROR)
        {
            Target = NULLPTR;
        }
        else if (Target->Access != TOKENS::TOKEN_KW_PUBLIC)
        {
            AnalyzerReport(Analyzer, Unit, AnalyzerLocation(Unit, Node, Use.Path), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL, Path);
            Target = NULLPTR;
        }
    }

    auto Existing = Scope->Symbols.find(Alias);

    if (Target != NULLPTR && Existing != Scope->Symbols.end() && Existing->second->Kind == ANALYZER_SYMBOL_KIND::ALIAS && AnalyzerUnalias(Existing->second) == Target)
    {
        Analyzer->Declarations[Node] = Existing->second;
        return TRUE;
    }

    TOKENS           Access = Analyzer->ImportAccess.contains(Node) ? Analyzer->ImportAccess[Node] : TOKENS::TOKEN_KW_PRIVATE;
    ANALYZER_SYMBOL *Symbol = AnalyzerDeclare(Analyzer, Scope, Target == NULLPTR ? ANALYZER_SYMBOL_KIND::ERROR : ANALYZER_SYMBOL_KIND::ALIAS, Alias, Node, Access);

    if (Symbol != NULLPTR)
    {
        Symbol->Target      = Target;
        Symbol->Initialized = TRUE;
    }

    return TRUE;
}

VOID AnalyzerBindImports(STATIC_ANALYZER *Analyzer)
{
    std::vector<std::pair<ANALYZER_UNIT *, CONST ANALYZER_IMPORT *>> Pending;

    for (CONST auto &Unit : Analyzer->Units)
    {
        for (CONST ANALYZER_IMPORT &Import : Unit->Imports)
        {
            Pending.emplace_back(Unit.get(), &Import);
        }
    }

    while (!Pending.empty())
    {
        std::vector<std::pair<ANALYZER_UNIT *, CONST ANALYZER_IMPORT *>> Remaining;

        for (CONST auto &[Unit, Import] : Pending)
        {
            if (!AnalyzerBindOne(Analyzer, Unit, *Import, FALSE))
            {
                Remaining.emplace_back(Unit, Import);
            }
        }

        if (Remaining.size() == Pending.size())
        {
            for (CONST auto &[Unit, Import] : Remaining)
            {
                AnalyzerBindOne(Analyzer, Unit, *Import, TRUE);
            }

            break;
        }

        Pending = std::move(Remaining);
    }
}

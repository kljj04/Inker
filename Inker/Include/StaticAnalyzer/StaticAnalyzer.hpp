// Inker/Include/StaticAnalyzer/StaticAnalyzer.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Parser/Parser.hpp>
#include <StaticAnalyzer/SemanticTypes.hpp>
#include <filesystem>
#include <memory>
#include <deque>
#include <map>
#include <unordered_map>
#include <functional>

struct ANALYZER_SCOPE;
struct ANALYZER_SYMBOL;
struct ANALYZER_UNIT;

enum class ANALYZER_SYMBOL_KIND : UINT8
{
    ERROR,
    NAMESPACE,
    VARIABLE,
    FUNCTION,
    TYPE,
    FIELD,
    PARAMETER,
    ENUM_MEMBER,
    ALIAS
};

struct ANALYZER_SCOPE
{
    ANALYZER_SCOPE                     *Parent = NULLPTR;
    ANALYZER_SYMBOL                    *Owner  = NULLPTR;
    ANALYZER_UNIT                      *Unit   = NULLPTR;
    std::map<STRING, ANALYZER_SYMBOL *> Symbols;
    std::vector<ANALYZER_SYMBOL *>      Ordered;
};

struct ANALYZER_SYMBOL
{
    ANALYZER_SYMBOL_KIND Kind = ANALYZER_SYMBOL_KIND::ERROR;
    STRING               Name;
    AST_LOC              Location{};
    AST_NODE            *Node         = NULLPTR;
    ANALYZER_UNIT       *Unit         = NULLPTR;
    ANALYZER_SCOPE      *Scope        = NULLPTR;
    ANALYZER_SCOPE      *Members      = NULLPTR;
    ANALYZER_SYMBOL     *Target       = NULLPTR;
    TOKENS               Access       = TOKENS::TOKEN_KW_PRIVATE;
    BOOLEAN              Used         = FALSE;
    BOOLEAN              Initialized  = FALSE;
    SEMANTIC_TYPE       *Type         = NULLPTR;
    BOOLEAN              TypeChecking = FALSE;
    BOOLEAN              TypeChecked  = FALSE;
    SEMANTIC_VALUE       Value;
};

struct ANALYZER_IMPORT
{
    AST_NODE      *Node   = NULLPTR;
    ANALYZER_UNIT *Target = NULLPTR;
};

struct ANALYZER_UNIT
{
    STRING                       Path;
    LEXER                        Lexer{};
    PARSER                       Parser;
    AST_NODE                    *Root   = NULLPTR;
    ANALYZER_SCOPE              *Global = NULLPTR;
    ANALYZER_SYMBOL             *Module = NULLPTR;
    std::vector<SIZE>            LineStarts;
    std::vector<ANALYZER_IMPORT> Imports;
    BOOLEAN                      Loading = TRUE;
    BOOLEAN                      Valid   = FALSE;
};

struct ANALYZER_OPTIONS
{
    STRING                   StdlibRoot;
    std::vector<STRING>      ModuleRoots;
    std::map<STRING, STRING> Overlays;
};

struct STATIC_ANALYZER
{
    ANALYZER_OPTIONS                                    Options;
    std::vector<std::unique_ptr<ANALYZER_UNIT>>         Units;
    std::deque<ANALYZER_SCOPE>                          Scopes;
    std::deque<ANALYZER_SYMBOL>                         Symbols;
    std::map<STRING, ANALYZER_UNIT *>                   Loaded;
    std::vector<DIAGNOSTIC>                             Diagnostics;
    std::vector<STRING>                                 LoadStack;
    std::unordered_map<AST_NODE *, ANALYZER_SYMBOL *>   Declarations;
    std::unordered_map<AST_NODE *, ANALYZER_SCOPE *>    NodeScopes;
    std::unordered_map<AST_NODE *, ANALYZER_SYMBOL *>   Bindings;
    std::unordered_map<AST_NODE *, TOKENS>              ImportAccess;
    std::deque<SEMANTIC_TYPE>                           Types;
    std::map<STRING, SEMANTIC_TYPE *>                   TypeCache;
    std::deque<SEMANTIC_VARIANT>                        Variants;
    std::map<STRING, SEMANTIC_VARIANT *>                VariantCache;
    std::unordered_map<AST_NODE *, SEMANTIC_TYPE *>     DeclaredTypes;
    std::unordered_map<AST_NODE *, SEMANTIC_TYPE *>     ExpressionTypes;
    std::unordered_map<AST_NODE *, SEMANTIC_VALUE>      Conversions;
    std::unordered_map<AST_NODE *, std::vector<UINT16>> CharacterArrays;
    std::set<STRING>                                    SemanticDiagnostics;
    SIZE                                                CallDepth = 0;
};

STRING           AnalyzerCanonicalPath(CONST std::filesystem::path &Path);
STRING           AnalyzerNormalizePath(STRING_VIEW Path);
ANALYZER_OPTIONS AnalyzerDefaultOptions(CONST STRING &Executable);
ANALYZER_UNIT   *AnalyzerLoad(STATIC_ANALYZER *Analyzer, CONST STRING &Path, ANALYZER_UNIT *Importer = NULLPTR, AST_NODE *Import = NULLPTR);
VOID             AnalyzerWalk(AST_NODE *Root, CONST std::function<VOID(AST_NODE *)> &Visitor);
AST_LOC          AnalyzerLocation(CONST ANALYZER_UNIT *Unit, CONST AST_NODE *Node, STRING_VIEW Name = {});
VOID             AnalyzerReport(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, AST_LOC Location, DIAGNOSTIC_LEVEL Level, DIAGNOSTIC_MESSAGE Message, STRING_VIEW First = {}, STRING_VIEW Second = {});
ANALYZER_SCOPE  *AnalyzerNewScope(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, ANALYZER_SCOPE *Parent = NULLPTR, ANALYZER_SYMBOL *Owner = NULLPTR);
ANALYZER_SYMBOL *AnalyzerDeclare(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, ANALYZER_SYMBOL_KIND Kind, STRING_VIEW Name, AST_NODE *Node, TOKENS Access);
ANALYZER_SYMBOL *AnalyzerFind(ANALYZER_SCOPE *Scope, STRING_VIEW Name);
ANALYZER_SYMBOL *AnalyzerUnalias(ANALYZER_SYMBOL *Symbol);

VOID             AnalyzerCollect(STATIC_ANALYZER *Analyzer);
VOID             AnalyzerBindImports(STATIC_ANALYZER *Analyzer);
ANALYZER_SYMBOL *AnalyzerQualified(ANALYZER_SCOPE *Scope, STRING_VIEW Path);

VOID           AnalyzerResolveNames(STATIC_ANALYZER *Analyzer);
ANALYZER_UNIT *StaticAnalyzerAnalyze(STATIC_ANALYZER *Analyzer, CONST STRING &Path);

VOID StaticAnalyzerReset(STATIC_ANALYZER *Analyzer);

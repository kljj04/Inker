// Inker/Include/StaticAnalyzer/SemanticTypes.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Parser/AST.hpp>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>

struct ANALYZER_SYMBOL;
struct ANALYZER_SCOPE;
struct ANALYZER_UNIT;
struct STATIC_ANALYZER;

enum class SEMANTIC_KIND : UINT8
{
    ERROR,
    VOID,
    BOOLEAN,
    STRING,
    INTEGER,
    FLOAT,
    CHARACTER,
    POINTER,
    ARRAY,
    RECORD,
    ENUM,
    FUNCTION,
    NAMESPACE,
    TYPE_OBJECT,
    NULL_VALUE,
    AUTO
};

struct SEMANTIC_TYPE
{
    SEMANTIC_KIND                Kind     = SEMANTIC_KIND::ERROR;
    SIZE                         ID       = 0;
    UINT16                       Bits     = 0;
    BOOLEAN                      Unsigned = FALSE;
    SEMANTIC_TYPE               *Element  = NULLPTR;
    SIZE                         Count    = 0;
    BOOLEAN                      Sized    = FALSE;
    ANALYZER_SYMBOL             *Owner    = NULLPTR;
    std::vector<SEMANTIC_TYPE *> Parameters;
    SEMANTIC_TYPE               *Return = NULLPTR;
};

struct SEMANTIC_VALUE
{
    SEMANTIC_TYPE   *Type         = NULLPTR;
    BOOLEAN          Literal      = FALSE;
    BOOLEAN          IntegerKnown = FALSE;
    UINT64           Integer      = 0;
    BOOLEAN          FloatKnown   = FALSE;
    FLOAT64          Float        = 0;
    BOOLEAN          TextKnown    = FALSE;
    STRING_VIEW      Text;
    AST_NODE        *Node   = NULLPTR;
    ANALYZER_SYMBOL *Symbol = NULLPTR;
    BOOLEAN          LValue = FALSE;
    STRING           AccessPath;
};

struct SEMANTIC_VARIANT
{
    ANALYZER_SYMBOL                                *Function = NULLPTR;
    std::vector<SEMANTIC_TYPE *>                    Parameters;
    SEMANTIC_TYPE                                  *Return         = NULLPTR;
    BOOLEAN                                         Checking       = FALSE;
    BOOLEAN                                         Checked        = FALSE;
    BOOLEAN                                         DefinitionOnly = FALSE;
    std::unordered_map<AST_NODE *, SEMANTIC_TYPE *> NodeTypes;
    std::unordered_map<AST_NODE *, SEMANTIC_VALUE>  Conversions;
    std::map<ANALYZER_SYMBOL *, SEMANTIC_TYPE *>    Locals;
};

struct SEMANTIC_CONTEXT
{
    STATIC_ANALYZER                             *Analyzer = NULLPTR;
    ANALYZER_UNIT                               *Unit     = NULLPTR;
    ANALYZER_SCOPE                              *Scope    = NULLPTR;
    ANALYZER_SYMBOL                             *Function = NULLPTR;
    SEMANTIC_VARIANT                            *Variant  = NULLPTR;
    std::map<ANALYZER_SYMBOL *, SEMANTIC_TYPE *> Locals;
    std::map<ANALYZER_SYMBOL *, SEMANTIC_VALUE>  Known;
    std::set<ANALYZER_SYMBOL *>                  Initialized;
    std::set<STRING>                             InitializedPaths;
    std::vector<SEMANTIC_VALUE>                  Returns;
    BOOLEAN                                      DefinitionOnly  = FALSE;
    SIZE                                         Depth           = 0;
    BOOLEAN                                      Unreachable     = FALSE;
    BOOLEAN                                      CheckingType    = FALSE;
    BOOLEAN                                      EnumInitializer = FALSE;
};

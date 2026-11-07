// Inker/Include/StaticAnalyzer/Semantics.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <StaticAnalyzer/StaticAnalyzer.hpp>
#include <StaticAnalyzer/SemanticTypes.hpp>

SEMANTIC_TYPE *SemanticType(STATIC_ANALYZER *Analyzer, SEMANTIC_KIND Kind, UINT16 Bits = 0, BOOLEAN Unsigned = FALSE, SEMANTIC_TYPE *Element = NULLPTR, SIZE Count = 0, BOOLEAN Sized = FALSE, ANALYZER_SYMBOL *Owner = NULLPTR);
STRING         SemanticName(CONST SEMANTIC_TYPE *Type);
BOOLEAN        SemanticNumeric(CONST SEMANTIC_TYPE *Type);
BOOLEAN        SemanticIntegral(CONST SEMANTIC_TYPE *Type);
BOOLEAN        SemanticHasAuto(CONST SEMANTIC_TYPE *Type);
BOOLEAN        SemanticCompatible(CONST SEMANTIC_TYPE *Expected, CONST SEMANTIC_TYPE *Actual);
SEMANTIC_TYPE *SemanticDeclared(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node);
SEMANTIC_TYPE *SemanticSymbolType(STATIC_ANALYZER *Analyzer, ANALYZER_SYMBOL *Symbol);
VOID           SemanticReport(SEMANTIC_CONTEXT *Context, AST_NODE *Node, DIAGNOSTIC_LEVEL Level, DIAGNOSTIC_MESSAGE Message, STRING_VIEW First = {}, STRING_VIEW Second = {}, STRING_VIEW Third = {});
SEMANTIC_VALUE SemanticExpression(SEMANTIC_CONTEXT *Context, AST_NODE *Node, SEMANTIC_TYPE *Expected = NULLPTR, BOOLEAN Read = TRUE);
SEMANTIC_VALUE SemanticConvert(SEMANTIC_CONTEXT *Context, SEMANTIC_VALUE Value, SEMANTIC_TYPE *Expected, AST_NODE *Site);
SEMANTIC_VALUE SemanticBinary(SEMANTIC_CONTEXT *Context, TOKENS Operator, SEMANTIC_VALUE Left, SEMANTIC_VALUE Right, AST_NODE *Site);
SEMANTIC_TYPE *SemanticCall(SEMANTIC_CONTEXT *Caller, ANALYZER_SYMBOL *Function, CONST std::vector<SEMANTIC_VALUE> &Arguments, AST_NODE *Site, BOOLEAN DefinitionOnly = FALSE);
UINT32         SemanticStatement(SEMANTIC_CONTEXT *Context, AST_NODE *Node);
VOID           SemanticCheck(STATIC_ANALYZER *Analyzer);
INT64          SemanticSigned(UINT64 Value, UINT16 Bits);
UINT64         SemanticMask(UINT16 Bits);

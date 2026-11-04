// Inker/Include/Parser/AST.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Types.hpp>
#include <Lexer/Tokens.hpp>

enum class AST_NODE_TYPE
{
    INTEGER,
    IDENTIFIER,
    UNARY,
    BINARY,
    VARIABLE_DECLARATION,
    EXPRESSION_STATEMENT,
    BLOCK
};

struct AST_NODE
{
    AST_NODE_TYPE Type;
    STRING        Name;
    INT64         Integer;
    TOKENS        Operator;
    AST_NODE     *Left;
    AST_NODE     *Right;
    AST_NODE     *Expression;
    AST_NODE    **Statements;
    SIZE          StatementCount;
};
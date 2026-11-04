// Inker/Include/Lexer/Lexer.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Types.hpp>
#include <Lexer/Tokens.hpp>

enum class LEXER_ERROR : UINT8
{
    NONE,
    INVALID_CHARACTER,
    INVALID_NUMBER,
    INVALID_ESCAPE,
    UNTERMINATED_STRING,
    UNTERMINATED_COMMENT,
    UNTERMINATED_INTERPOLATION,
    NESTING_LIMIT
};

enum class LEXER_MODE : UINT8
{
    STRING,
    EXPRESSION
};

typedef struct
{
    LEXER_MODE Mode;
    CHAR8      Quote;
    SIZE       BracketDepth;
} LEXER_FRAME;

typedef struct
{
    STRING      Source;
    SIZE        Position;
    SIZE        Line;
    SIZE        Column;
    SIZE        TokenStart;
    SIZE        TokenLength;
    SIZE        TokenLine;
    SIZE        TokenColumn;
    LEXER_ERROR Error;
    SIZE        ErrorLine;
    SIZE        ErrorColumn;
    LEXER_FRAME Frames[64];
    SIZE        FrameCount;
} LEXER;

VOID   LexerInit(LEXER *Lexer, CONST STRING &Source);
TOKENS LexerNextToken(LEXER *Lexer);
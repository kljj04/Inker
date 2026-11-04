// Inker/Include/Diagnostics/Messages.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Lexer/Lexer.hpp>
#include <Types.hpp>

enum class DIAGNOSTIC_MESSAGE
{
    NONE,
    INVALID_CHARACTER,
    INVALID_NUMBER,
    INVALID_ESCAPE,
    UNTERMINATED_STRING,
    UNTERMINATED_COMMENT,
    UNTERMINATED_INTERPOLATION,
    NESTING_LIMIT,
    EXPECTED_IDENTIFIER,
    EXPECTED_EXPRESSION,
    EXPECTED_SEMICOLON,
    EXPECTED_RIGHT_PAREN,
    UNEXPECTED_TOKEN
};

CONST CHAR8 *DiagnosticMessageString(DIAGNOSTIC_MESSAGE Message);
CONST CHAR8 *DiagnosticLexerMessage(LEXER_ERROR Error);
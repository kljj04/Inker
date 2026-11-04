// Inker/Source/Diagnostics/Messages.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Diagnostics/Messages.hpp>

CONST CHAR8 *DiagnosticMessageString(DIAGNOSTIC_MESSAGE Message)
{
    switch (Message)
    {
        case DIAGNOSTIC_MESSAGE::NONE:
        {
            return "";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_CHARACTER:
        {
            return "invalid character";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_NUMBER:
        {
            return "invalid number";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ESCAPE:
        {
            return "invalid escape sequence";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_STRING:
        {
            return "unterminated string";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_COMMENT:
        {
            return "unterminated block comment";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_INTERPOLATION:
        {
            return "unterminated interpolation";
        }

        case DIAGNOSTIC_MESSAGE::NESTING_LIMIT:
        {
            return "maximum nesting depth exceeded";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_IDENTIFIER:
        {
            return "expected identifier";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_EXPRESSION:
        {
            return "expected expression";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_SEMICOLON:
        {
            return "expected ';'";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_RIGHT_PAREN:
        {
            return "expected ')'";
        }

        case DIAGNOSTIC_MESSAGE::UNEXPECTED_TOKEN:
        {
            return "unexpected token";
        }

        default:
        {
            return "unknown diagnostic message";
        }
    }
}

CONST CHAR8 *DiagnosticLexerMessage(LEXER_ERROR Error)
{
    switch (Error)
    {
        case LEXER_ERROR::INVALID_CHARACTER:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_CHARACTER);
        }

        case LEXER_ERROR::INVALID_NUMBER:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_NUMBER);
        }

        case LEXER_ERROR::INVALID_ESCAPE:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_ESCAPE);
        }

        case LEXER_ERROR::UNTERMINATED_STRING:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_STRING);
        }

        case LEXER_ERROR::UNTERMINATED_COMMENT:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_COMMENT);
        }

        case LEXER_ERROR::UNTERMINATED_INTERPOLATION:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_INTERPOLATION);
        }

        case LEXER_ERROR::NESTING_LIMIT:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        }

        default:
        {
            return "unknown lexer error";
        }
    }
}
// Inker/Include/Parser/Parser.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Diagnostics/Messages.hpp>
#include <Types.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/AST.hpp>

struct PARSER
{
    LEXER              *Lexer;
    TOKENS              Current;
    TOKENS              Previous;
    SIZE                CurrentStart;
    SIZE                CurrentLength;
    SIZE                PreviousStart;
    SIZE                PreviousLength;
    DIAGNOSTIC_MESSAGE  ErrorMessage;
    SIZE                ErrorStart;
    SIZE                ErrorLength;
};

VOID      ParserInit(PARSER *Parser, LEXER *Lexer);
AST_NODE *ParserParse(PARSER *Parser);
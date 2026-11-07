// Inker/Include/Parser/Parser.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Diagnostics/Messages.hpp>
#include <Diagnostics/Diagnostics.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/AST.hpp>
#include <vector>

struct PARSER_TOKEN
{
    TOKENS  Type;
    AST_LOC Location;
    BOOLEAN StringEnd;
};

struct PARSER_DIAGNOSTIC
{
    DIAGNOSTIC_MESSAGE Message;
    STRING             Argument;
    AST_LOC            Location;
    DIAGNOSTIC_LEVEL   Level = DIAGNOSTIC_LEVEL::ERROR;
    STRING             FixIt{};
};

struct PARSER
{
    LEXER                         *Lexer          = NULLPTR;
    TOKENS                         Current        = TOKENS::TOKEN_EOF;
    TOKENS                         Previous       = TOKENS::TOKEN_EOF;
    SIZE                           CurrentStart   = 0;
    SIZE                           CurrentLength  = 0;
    SIZE                           PreviousStart  = 0;
    SIZE                           PreviousLength = 0;
    DIAGNOSTIC_MESSAGE             ErrorMessage   = DIAGNOSTIC_MESSAGE::NONE;
    STRING                         ErrorArgument;
    SIZE                           ErrorStart  = 0;
    SIZE                           ErrorLength = 0;
    AST_ARENA                      Arena;
    std::vector<PARSER_TOKEN>      Tokens;
    std::vector<PARSER_DIAGNOSTIC> Diagnostics;
    SIZE                           Position      = 0;
    SIZE                           Depth         = 0;
    SIZE                           LoopDepth     = 0;
    SIZE                           FunctionDepth = 0;
    BOOLEAN                        Parsed        = FALSE;
    AST_NODE                      *Root          = NULLPTR;

    std::vector<STRING_VIEW> TypeNames;

    ~PARSER(VOID);
};

struct PARSER_FAILURE
{
};

struct PARSER_DEPTH
{
    PARSER  *Parser;
    EXPLICIT PARSER_DEPTH(PARSER *Parser);
    ~PARSER_DEPTH(VOID);
};

VOID          ParserInit(PARSER *Parser, LEXER *Lexer);
VOID          ParserDestroy(PARSER *Parser) NOEXCEPT;
AST_NODE     *ParserParse(PARSER *Parser);
TOKENS        ParserPeek(CONST PARSER *Parser, SIZE Offset = 0);
VOID          ParserAdvance(PARSER *Parser);
BOOLEAN       ParserMatch(PARSER *Parser, TOKENS Token);
VOID          ParserConsume(PARSER *Parser, TOKENS Token, STRING_VIEW Expected, STRING_VIEW Context = "statement");
STRING_VIEW   ParserText(CONST PARSER *Parser);
AST_LOC       ParserLocation(CONST PARSER *Parser);
AST_LOC       ParserFinish(CONST PARSER *Parser, AST_LOC Start);
VOID          ParserReport(PARSER *Parser, DIAGNOSTIC_LEVEL Level, AST_LOC Location, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument = {}, STRING_VIEW FixIt = {});
NORETURN VOID ParserFail(PARSER *Parser, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument = {}, STRING_VIEW FixIt = {});
NORETURN VOID ParserFailAt(PARSER *Parser, AST_LOC Location, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument = {}, STRING_VIEW FixIt = {});
AST_NODE     *ParserNode(PARSER *Parser, AST_NODE_TYPE Type, AST_LOC Location);
VOID          ParserAppend(PARSER *Parser, AST_NODE_LIST *List, AST_NODE *Node);
VOID          ParserRecover(PARSER *Parser, SIZE Start, BOOLEAN InBlock);
VOID          ParserSequence(PARSER *Parser, AST_NODE_LIST *List, BOOLEAN InBlock, BOOLEAN Members = FALSE);
AST_NODE     *ParserExpression(PARSER *Parser, INT32 MinPower = 1);
AST_NODE     *ParserString(PARSER *Parser);
AST_NODE     *ParserStatement(PARSER *Parser);
AST_NODE     *ParserBlock(PARSER *Parser);
AST_NODE     *ParserDeclaration(PARSER *Parser, BOOLEAN Member = FALSE, BOOLEAN Terminator = TRUE);
BOOLEAN       ParserIsDeclaration(CONST PARSER *Parser);
BOOLEAN       ParserIsType(TOKENS Token);
AST_NODE     *ParserType(PARSER *Parser);
BOOLEAN       ParserAssignable(CONST AST_NODE *Node);

AST_NODE *ParserArrayDeclarator(PARSER *Parser, AST_NODE *Type);

// Inker/Source/Parser/Parser.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>
#include <new>
#include <algorithm>

PARSER::~PARSER(VOID)
{
    ASTArenaDestroy(&Arena);
}

VOID ParserDestroy(PARSER *Parser) NOEXCEPT
{
    ASTArenaDestroy(&Parser->Arena);
    Parser->Root = NULLPTR;
    Parser->Tokens.clear();
    Parser->Diagnostics.clear();
    Parser->TypeNames.clear();
    Parser->Parsed = FALSE;
}

VOID ParserInit(PARSER *Parser, LEXER *Lexer)
{
    ParserDestroy(Parser);
    Parser->Lexer        = Lexer;
    Parser->Current      = TOKENS::TOKEN_EOF;
    Parser->Previous     = TOKENS::TOKEN_EOF;
    Parser->CurrentStart = Parser->CurrentLength = 0;
    Parser->PreviousStart = Parser->PreviousLength = 0;
    Parser->ErrorMessage                           = DIAGNOSTIC_MESSAGE::NONE;
    Parser->ErrorArgument.clear();
    Parser->ErrorStart = Parser->ErrorLength = 0;
    Parser->Position = Parser->Depth = Parser->LoopDepth = Parser->FunctionDepth = 0;
}

TOKENS ParserPeek(CONST PARSER *Parser, SIZE Offset)
{
    if (Parser->Position >= Parser->Tokens.size() || Offset >= Parser->Tokens.size() - Parser->Position)
    {
        return TOKENS::TOKEN_EOF;
    }

    return Parser->Tokens[Parser->Position + Offset].Type;
}

AST_LOC ParserLocation(CONST PARSER *Parser)
{
    return Parser->Tokens.empty() ? AST_LOC{} : Parser->Tokens[Parser->Position].Location;
}

VOID ParserAdvance(PARSER *Parser)
{
    Parser->Previous       = Parser->Current;
    Parser->PreviousStart  = Parser->CurrentStart;
    Parser->PreviousLength = Parser->CurrentLength;

    if (Parser->Position + 1 < Parser->Tokens.size())
    {
        Parser->Position++;
    }

    Parser->Current       = ParserPeek(Parser);
    AST_LOC Location      = ParserLocation(Parser);
    Parser->CurrentStart  = Location.Start;
    Parser->CurrentLength = Location.Length;
}

BOOLEAN ParserMatch(PARSER *Parser, TOKENS Token)
{
    if (ParserPeek(Parser) != Token)
    {
        return FALSE;
    }

    ParserAdvance(Parser);

    return TRUE;
}

STATIC AST_LOC ParserInsertionLocation(CONST PARSER *Parser)
{
    if (Parser->Position == 0)
    {
        AST_LOC Location = ParserLocation(Parser);
        Location.Length  = 0;
        return Location;
    }

    AST_LOC Location = Parser->Tokens[Parser->Position - 1].Location;
    SIZE    End      = Location.Start + Location.Length;

    for (SIZE Index = Location.Start; Index < End; Index++)
    {
        if (Parser->Lexer->Source[Index] == '\n')
        {
            Location.Line++;
            Location.Column = 1;
        }
        else
        {
            Location.Column++;
        }
    }

    Location.Start  = End;
    Location.Length = 0;
    return Location;
}

VOID ParserConsume(PARSER *Parser, TOKENS Token, STRING_VIEW Expected, STRING_VIEW Context)
{
    if (ParserMatch(Parser, Token))
    {
        return;
    }

    if (Token == TOKENS::TOKEN_SP_SEMICOLON)
    {
        ParserFailAt(Parser, ParserInsertionLocation(Parser), DIAGNOSTIC_MESSAGE::MISSING_SEMICOLON, Context, ";");
    }

    if (Token == TOKENS::TOKEN_IDENTIFIER)
    {
        ParserFail(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_NAME, Expected);
    }

    AST_LOC Location  = ParserLocation(Parser);
    AST_LOC Insertion = ParserInsertionLocation(Parser);
    BOOLEAN Closing   = Token == TOKENS::TOKEN_SP_RPAREN || Token == TOKENS::TOKEN_SP_RBRACKET || Token == TOKENS::TOKEN_SP_RBRACE || Token == TOKENS::TOKEN_SP_INTERPOLATION_END;
    BOOLEAN Boundary  = ParserPeek(Parser) == TOKENS::TOKEN_EOF || ParserPeek(Parser) == TOKENS::TOKEN_SP_RBRACE || ParserPeek(Parser) == TOKENS::TOKEN_SP_SEMICOLON || Location.Line > Insertion.Line;

    if (Closing && Boundary)
    {
        Location = Insertion;
    }

    ParserFailAt(Parser, Location, DIAGNOSTIC_MESSAGE::EXPECTED_TOKEN, Expected, Closing && Boundary ? Expected : STRING_VIEW{});
}

STRING_VIEW ParserText(CONST PARSER *Parser)
{
    return STRING_VIEW(Parser->Lexer->Source).substr(Parser->CurrentStart, Parser->CurrentLength);
}

AST_LOC ParserFinish(CONST PARSER *Parser, AST_LOC Start)
{
    SIZE End     = Parser->PreviousStart + Parser->PreviousLength;
    Start.Length = End >= Start.Start ? End - Start.Start : Start.Length;

    return Start;
}

NORETURN VOID ParserFail(PARSER *Parser, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument, STRING_VIEW FixIt)
{
    ParserFailAt(Parser, ParserLocation(Parser), Message, Argument, FixIt);
}

VOID ParserReport(PARSER *Parser, DIAGNOSTIC_LEVEL Level, AST_LOC Location, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument, STRING_VIEW FixIt)
{
    if (Argument.empty() && Parser->Lexer != NULLPTR)
    {
        Argument = STRING_VIEW(Parser->Lexer->Source).substr(Location.Start, Location.Length);
    }

    if (Level == DIAGNOSTIC_LEVEL::ERROR && Parser->ErrorMessage == DIAGNOSTIC_MESSAGE::NONE)
    {
        Parser->ErrorMessage  = Message;
        Parser->ErrorArgument = Argument;
        Parser->ErrorStart    = Location.Start;
        Parser->ErrorLength   = Location.Length;
    }

    Parser->Diagnostics.push_back({Message, STRING(Argument), Location, Level, STRING(FixIt)});
}

NORETURN VOID ParserFailAt(PARSER *Parser, AST_LOC Location, DIAGNOSTIC_MESSAGE Message, STRING_VIEW Argument, STRING_VIEW FixIt)
{
    ParserReport(Parser, DIAGNOSTIC_LEVEL::ERROR, Location, Message, Argument, FixIt);
    throw PARSER_FAILURE{};
}

PARSER_DEPTH::PARSER_DEPTH(PARSER *Value) : Parser(Value)
{
    if (Parser->Depth >= 128)
    {
        ParserFail(Parser, DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
    }

    Parser->Depth++;
}

PARSER_DEPTH::~PARSER_DEPTH(VOID)
{
    Parser->Depth--;
}

AST_NODE *ParserNode(PARSER *Parser, AST_NODE_TYPE Type, AST_LOC Location)
{
    AST_NODE *Node = ASTCreateNode(&Parser->Arena, Type, Location);

    if (Node == NULLPTR)
    {
        throw std::bad_alloc{};
    }

    return Node;
}

VOID ParserAppend(PARSER *Parser, AST_NODE_LIST *List, AST_NODE *Node)
{
    if (!ASTNodeListAppend(&Parser->Arena, List, Node))
    {
        throw std::bad_alloc{};
    }
}

VOID ParserRecover(PARSER *Parser, SIZE Start, BOOLEAN InBlock)
{
    SIZE Braces   = 0;
    SIZE Parens   = 0;
    SIZE Brackets = 0;

    for (SIZE Index = Start; Index < Parser->Position; Index++)
    {
        TOKENS Token = Parser->Tokens[Index].Type;

        if (Token == TOKENS::TOKEN_SP_LPAREN)
        {
            Parens++;
        }
        else if (Token == TOKENS::TOKEN_SP_RPAREN && Parens > 0)
        {
            Parens--;
        }
        else if (Token == TOKENS::TOKEN_SP_LBRACKET)
        {
            Brackets++;
        }
        else if (Token == TOKENS::TOKEN_SP_RBRACKET && Brackets > 0)
        {
            Brackets--;
        }

        if (Token == TOKENS::TOKEN_SP_LBRACE)
        {
            Braces++;
        }
        else if (Token == TOKENS::TOKEN_SP_RBRACE && Braces > 0)
        {
            Braces--;
        }
    }

    while (ParserPeek(Parser) != TOKENS::TOKEN_EOF)
    {
        TOKENS Token = ParserPeek(Parser);

        if (Braces == 0)
        {
            if (Token == TOKENS::TOKEN_SP_RBRACE && InBlock)
            {
                return;
            }

            if (Token == TOKENS::TOKEN_SP_SEMICOLON)
            {
                ParserAdvance(Parser);
                return;
            }

            if (Parser->Position > Start && Parens == 0 && Brackets == 0 && Token == TOKENS::TOKEN_IDENTIFIER && ParserLocation(Parser).Line > Parser->Tokens[Parser->Position - 1].Location.Line)
            {
                return;
            }

            if (Parser->Position > Start && (ParserIsDeclaration(Parser) || Token == TOKENS::TOKEN_KW_IF || Token == TOKENS::TOKEN_KW_WHILE || Token == TOKENS::TOKEN_KW_FOR || Token == TOKENS::TOKEN_KW_RETURN))
            {
                return;
            }
        }

        if (Token == TOKENS::TOKEN_SP_LPAREN)
        {
            Parens++;
        }
        else if (Token == TOKENS::TOKEN_SP_RPAREN && Parens > 0)
        {
            Parens--;
        }
        else if (Token == TOKENS::TOKEN_SP_LBRACKET)
        {
            Brackets++;
        }
        else if (Token == TOKENS::TOKEN_SP_RBRACKET && Brackets > 0)
        {
            Brackets--;
        }

        if (Token == TOKENS::TOKEN_SP_LBRACE)
        {
            Braces++;
        }
        else if (Token == TOKENS::TOKEN_SP_RBRACE && Braces > 0)
        {
            Braces--;
        }

        ParserAdvance(Parser);

        if (Braces == 0 && Token == TOKENS::TOKEN_SP_RBRACE)
        {
            return;
        }
    }
}

VOID ParserSequence(PARSER *Parser, AST_NODE_LIST *List, BOOLEAN InBlock, BOOLEAN Members)
{
    while (ParserPeek(Parser) != TOKENS::TOKEN_EOF && !(InBlock && ParserPeek(Parser) == TOKENS::TOKEN_SP_RBRACE))
    {
        SIZE Start = Parser->Position;
        SIZE Count = List->Count;

        try
        {
            TOKENS Token = ParserPeek(Parser);

            if ((Members || Parser->FunctionDepth == 0) && (Token == TOKENS::TOKEN_KW_PUBLIC || Token == TOKENS::TOKEN_KW_PRIVATE || Token == TOKENS::TOKEN_KW_PROTECTED))
            {
                AST_NODE *Access                = ParserNode(Parser, AST_NODE_TYPE::ACCESS_MODIFIER, ParserLocation(Parser));
                Access->AccessModifier.Modifier = Token;
                ParserAdvance(Parser);
                ParserAppend(Parser, List, Access);
            }

            AST_NODE *Node;

            if (Members)
            {
                Node = ParserDeclaration(Parser, TRUE);
            }
            else
            {
                Node = ParserIsDeclaration(Parser) ? ParserDeclaration(Parser) : ParserStatement(Parser);
            }

            ParserAppend(Parser, List, Node);
        }
        catch (CONST PARSER_FAILURE &)
        {
            List->Count = Count;
            ParserRecover(Parser, Start, InBlock);
        }
    }
}

AST_NODE *ParserParse(PARSER *Parser)
{
    if (Parser->Parsed)
    {
        return Parser->ErrorMessage == DIAGNOSTIC_MESSAGE::NONE ? Parser->Root : NULLPTR;
    }

    Parser->Parsed = TRUE;

    try
    {
        if (Parser->Lexer == NULLPTR)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_ARGUMENT, "null lexer");
        }

        for (;;)
        {
            SIZE    Frames   = Parser->Lexer->FrameCount;
            BOOLEAN InString = Frames > 0 && Parser->Lexer->Frames[Frames - 1].Mode == LEXER_MODE::STRING;
            TOKENS  Token    = LexerNextToken(Parser->Lexer);
            AST_LOC Location{Parser->Lexer->TokenStart, Parser->Lexer->TokenLength, Parser->Lexer->TokenLine, Parser->Lexer->TokenColumn};
            Parser->Tokens.push_back({Token, Location, Token == TOKENS::TOKEN_LIT_STRING && (InString ? Parser->Lexer->FrameCount < Frames : Parser->Lexer->FrameCount == Frames)});

            if (Token == TOKENS::TOKEN_UNKNOWN || Token == TOKENS::TOKEN_EOF)
            {
                break;
            }
        }

        Parser->Position      = 0;
        Parser->Current       = ParserPeek(Parser);
        Parser->CurrentStart  = ParserLocation(Parser).Start;
        Parser->CurrentLength = ParserLocation(Parser).Length;

        if (Parser->Tokens.back().Type == TOKENS::TOKEN_UNKNOWN)
        {
            Parser->Position      = Parser->Tokens.size() - 1;
            Parser->Current       = ParserPeek(Parser);
            Parser->CurrentStart  = ParserLocation(Parser).Start;
            Parser->CurrentLength = ParserLocation(Parser).Length;
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::UNEXPECTED_TOKEN);
        }

        Parser->Root = ParserNode(Parser, AST_NODE_TYPE::PROGRAM, {0, Parser->Lexer->Source.size(), 1, 1});
        ParserSequence(Parser, &Parser->Root->Program, FALSE);
    }
    catch (CONST PARSER_FAILURE &)
    {
        Parser->Root = NULLPTR;
    }
    catch (CONST std::bad_alloc &)
    {
        Parser->ErrorMessage = DIAGNOSTIC_MESSAGE::INTERNAL_ERROR;
        Parser->ErrorArgument.clear();
        Parser->ErrorStart  = Parser->CurrentStart;
        Parser->ErrorLength = Parser->CurrentLength;
        Parser->Root        = NULLPTR;
    }

    return Parser->ErrorMessage == DIAGNOSTIC_MESSAGE::NONE ? Parser->Root : NULLPTR;
}

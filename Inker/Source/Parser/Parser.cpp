
 // Inker/Source/Parser/Parser.cpp
 // Inker Project
 // Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>

#include <cerrno>
#include <cstdlib>

STATIC BOOLEAN ParserHasError(PARSER *Parser)
{
    return Parser->ErrorMessage != DIAGNOSTIC_MESSAGE::NONE;
}

STATIC VOID ParserSetError(PARSER *Parser, DIAGNOSTIC_MESSAGE Message)
{
    if (ParserHasError(Parser))
    {
        return;
    }

    Parser->ErrorMessage = Message;
    Parser->ErrorStart   = Parser->CurrentStart;
    Parser->ErrorLength  = Parser->CurrentLength;
}

STATIC VOID ParserAdvance(PARSER *Parser)
{
    if (ParserHasError(Parser))
    {
        return;
    }

    Parser->Previous       = Parser->Current;
    Parser->PreviousStart  = Parser->CurrentStart;
    Parser->PreviousLength = Parser->CurrentLength;

    Parser->Current       = LexerNextToken(Parser->Lexer);
    Parser->CurrentStart  = Parser->Lexer->TokenStart;
    Parser->CurrentLength = Parser->Lexer->TokenLength;

    if (Parser->Current == TOKENS::TOKEN_UNKNOWN)
    {
        ParserSetError(Parser, DIAGNOSTIC_MESSAGE::UNEXPECTED_TOKEN);
    }
}

STATIC BOOLEAN ParserMatch(PARSER *Parser, TOKENS Type)
{
    if (ParserHasError(Parser) || Parser->Current != Type)
    {
        return FALSE;
    }

    ParserAdvance(Parser);
    return TRUE;
}

STATIC BOOLEAN ParserExpect(PARSER *Parser, TOKENS Type)
{
    if (ParserHasError(Parser))
    {
        return FALSE;
    }

    if (Parser->Current == Type)
    {
        ParserAdvance(Parser);
        return !ParserHasError(Parser);
    }

    DIAGNOSTIC_MESSAGE Message = DIAGNOSTIC_MESSAGE::UNEXPECTED_TOKEN;

    switch (Type)
    {
        case TOKENS::TOKEN_IDENTIFIER:
        {
            Message = DIAGNOSTIC_MESSAGE::EXPECTED_IDENTIFIER;
            break;
        }

        case TOKENS::TOKEN_SP_SEMICOLON:
        {
            Message = DIAGNOSTIC_MESSAGE::EXPECTED_SEMICOLON;
            break;
        }

        case TOKENS::TOKEN_SP_RIGHT_PAREN:
        {
            Message = DIAGNOSTIC_MESSAGE::EXPECTED_RIGHT_PAREN;
            break;
        }

        default:
        {
            break;
        }
    }

    ParserSetError(Parser, Message);
    return FALSE;
}

STATIC STRING ParserTokenText(PARSER *Parser)
{
    return Parser->Lexer->Source.substr(Parser->CurrentStart, Parser->CurrentLength);
}

STATIC AST_NODE *ParserCreateNode(AST_NODE_TYPE Type)
{
    AST_NODE *Node = new AST_NODE{};

    Node->Type = Type;

    return Node;
}

STATIC VOID ParserFreeNode(AST_NODE *Node)
{
    if (Node == NULLPTR)
    {
        return;
    }

    ParserFreeNode(Node->Left);
    ParserFreeNode(Node->Right);
    ParserFreeNode(Node->Expression);

    for (SIZE Index = 0; Index < Node->StatementCount; Index++)
    {
        ParserFreeNode(Node->Statements[Index]);
    }

    delete[] Node->Statements;
    delete Node;
}

STATIC AST_NODE *ParserParseExpression(PARSER *Parser);

STATIC AST_NODE *ParserParsePrimary(PARSER *Parser)
{
    if (ParserHasError(Parser))
    {
        return NULLPTR;
    }

    if (Parser->Current == TOKENS::TOKEN_LIT_INTEGER)
    {
        STRING Text = ParserTokenText(Parser);
        CHAR8 *End  = NULLPTR;

        errno = 0;

        CONST CHAR8 *Start = Text.c_str();
        INT64 Value = std::strtoll(Start, &End, 0);

        if (End == Start || *End != '\0' || errno == ERANGE)
        {
            ParserSetError(Parser, DIAGNOSTIC_MESSAGE::INVALID_NUMBER);
            return NULLPTR;
        }

        AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::INTEGER);

        Node->Integer = Value;

        ParserAdvance(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Node);
            return NULLPTR;
        }

        return Node;
    }

    if (Parser->Current == TOKENS::TOKEN_IDENTIFIER)
    {
        AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::IDENTIFIER);

        Node->Name = ParserTokenText(Parser);

        ParserAdvance(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Node);
            return NULLPTR;
        }

        return Node;
    }

    if (Parser->Current == TOKENS::TOKEN_SP_LEFT_PAREN)
    {
        ParserAdvance(Parser);

        AST_NODE *Node = ParserParseExpression(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Node);
            return NULLPTR;
        }

        if (!ParserExpect(Parser, TOKENS::TOKEN_SP_RIGHT_PAREN))
        {
            ParserFreeNode(Node);
            return NULLPTR;
        }

        return Node;
    }

    ParserSetError(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_EXPRESSION);
    return NULLPTR;
}

STATIC AST_NODE *ParserParseUnary(PARSER *Parser)
{
    if (ParserHasError(Parser))
    {
        return NULLPTR;
    }

    if (Parser->Current == TOKENS::TOKEN_OP_PLUS ||
        Parser->Current == TOKENS::TOKEN_OP_MINUS ||
        Parser->Current == TOKENS::TOKEN_OP_NOT ||
        Parser->Current == TOKENS::TOKEN_OP_BIT_NOT)
    {
        TOKENS Operator = Parser->Current;

        ParserAdvance(Parser);

        AST_NODE *Expression = ParserParseUnary(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Expression);
            return NULLPTR;
        }

        AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::UNARY);

        Node->Operator   = Operator;
        Node->Expression = Expression;

        return Node;
    }

    return ParserParsePrimary(Parser);
}

STATIC AST_NODE *ParserParseMultiplicative(PARSER *Parser)
{
    AST_NODE *Left = ParserParseUnary(Parser);

    if (ParserHasError(Parser))
    {
        ParserFreeNode(Left);
        return NULLPTR;
    }

    while (!ParserHasError(Parser) &&
           (Parser->Current == TOKENS::TOKEN_OP_MULTIPLY ||
            Parser->Current == TOKENS::TOKEN_OP_DIVIDE ||
            Parser->Current == TOKENS::TOKEN_OP_MODULO))
    {
        TOKENS Operator = Parser->Current;

        ParserAdvance(Parser);

        AST_NODE *Right = ParserParseUnary(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Left);
            ParserFreeNode(Right);
            return NULLPTR;
        }

        AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::BINARY);

        Node->Operator = Operator;
        Node->Left     = Left;
        Node->Right    = Right;

        Left = Node;
    }

    return Left;
}

STATIC AST_NODE *ParserParseAdditive(PARSER *Parser)
{
    AST_NODE *Left = ParserParseMultiplicative(Parser);

    if (ParserHasError(Parser))
    {
        ParserFreeNode(Left);
        return NULLPTR;
    }

    while (!ParserHasError(Parser) &&
           (Parser->Current == TOKENS::TOKEN_OP_PLUS ||
            Parser->Current == TOKENS::TOKEN_OP_MINUS))
    {
        TOKENS Operator = Parser->Current;

        ParserAdvance(Parser);

        AST_NODE *Right = ParserParseMultiplicative(Parser);

        if (ParserHasError(Parser))
        {
            ParserFreeNode(Left);
            ParserFreeNode(Right);
            return NULLPTR;
        }

        AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::BINARY);

        Node->Operator = Operator;
        Node->Left     = Left;
        Node->Right    = Right;

        Left = Node;
    }

    return Left;
}

STATIC AST_NODE *ParserParseExpression(PARSER *Parser)
{
    return ParserParseAdditive(Parser);
}

STATIC AST_NODE *ParserParseVariableDeclaration(PARSER *Parser)
{
    ParserAdvance(Parser);

    if (!ParserExpect(Parser, TOKENS::TOKEN_IDENTIFIER))
    {
        return NULLPTR;
    }

    AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::VARIABLE_DECLARATION);

    Node->Name = Parser->Lexer->Source.substr(Parser->PreviousStart, Parser->PreviousLength);

    if (!ParserExpect(Parser, TOKENS::TOKEN_OP_ASSIGN))
    {
        ParserFreeNode(Node);
        return NULLPTR;
    }

    Node->Expression = ParserParseExpression(Parser);

    if (ParserHasError(Parser))
    {
        ParserFreeNode(Node);
        return NULLPTR;
    }

    if (!ParserExpect(Parser, TOKENS::TOKEN_SP_SEMICOLON))
    {
        ParserFreeNode(Node);
        return NULLPTR;
    }

    return Node;
}

STATIC AST_NODE *ParserParseStatement(PARSER *Parser)
{
    if (Parser->Current == TOKENS::TOKEN_KW_SET)
    {
        return ParserParseVariableDeclaration(Parser);
    }

    AST_NODE *Node = ParserCreateNode(AST_NODE_TYPE::EXPRESSION_STATEMENT);

    Node->Expression = ParserParseExpression(Parser);

    if (ParserHasError(Parser))
    {
        ParserFreeNode(Node);
        return NULLPTR;
    }

    if (!ParserExpect(Parser, TOKENS::TOKEN_SP_SEMICOLON))
    {
        ParserFreeNode(Node);
        return NULLPTR;
    }

    return Node;
}

VOID ParserInit(PARSER *Parser, LEXER *Lexer)
{
    *Parser = {};

    Parser->Lexer = Lexer;

    ParserAdvance(Parser);
}

AST_NODE *ParserParse(PARSER *Parser)
{
    if (ParserHasError(Parser))
    {
        return NULLPTR;
    }

    AST_NODE *Root = ParserCreateNode(AST_NODE_TYPE::BLOCK);

    while (!ParserHasError(Parser) && Parser->Current != TOKENS::TOKEN_EOF)
    {
        AST_NODE *Statement = ParserParseStatement(Parser);

        if (ParserHasError(Parser) || Statement == NULLPTR)
        {
            ParserFreeNode(Statement);
            ParserFreeNode(Root);
            return NULLPTR;
        }

        AST_NODE **Statements = new AST_NODE *[Root->StatementCount + 1];

        for (SIZE Index = 0; Index < Root->StatementCount; Index++)
        {
            Statements[Index] = Root->Statements[Index];
        }

        Statements[Root->StatementCount] = Statement;

        delete[] Root->Statements;

        Root->Statements = Statements;
        Root->StatementCount++;
    }

    if (ParserHasError(Parser))
    {
        ParserFreeNode(Root);
        return NULLPTR;
    }

    return Root;
}

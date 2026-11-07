// Inker/Source/Parser/Statement.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>

AST_NODE *ParserBlock(PARSER *Parser)
{
    PARSER_DEPTH Guard(Parser);
    AST_LOC      Start = ParserLocation(Parser);
    ParserConsume(Parser, TOKENS::TOKEN_SP_LBRACE, "{");
    AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::BLOCK, Start);
    ParserSequence(Parser, &Node->Block.Statements, TRUE);
    ParserConsume(Parser, TOKENS::TOKEN_SP_RBRACE, "}");
    Node->Location = ParserFinish(Parser, Start);

    return Node;
}

STATIC AST_NODE *ParserCondition(PARSER *Parser)
{
    ParserConsume(Parser, TOKENS::TOKEN_SP_LPAREN, "(");
    AST_NODE *Condition = ParserExpression(Parser);
    ParserConsume(Parser, TOKENS::TOKEN_SP_RPAREN, ")");

    return Condition;
}

STATIC AST_NODE *ParserLoopBody(PARSER *Parser)
{
    Parser->LoopDepth++;

    try
    {
        AST_NODE *Body = ParserStatement(Parser);
        Parser->LoopDepth--;

        return Body;
    }
    catch (...)
    {
        Parser->LoopDepth--;
        throw;
    }
}

AST_NODE *ParserStatement(PARSER *Parser)
{
    PARSER_DEPTH Guard(Parser);
    AST_LOC      Start = ParserLocation(Parser);
    TOKENS       Token = ParserPeek(Parser);

    if (Token == TOKENS::TOKEN_SP_LBRACE)
    {
        return ParserBlock(Parser);
    }

    AST_NODE *Node;

    if (ParserMatch(Parser, TOKENS::TOKEN_KW_IF))
    {
        Node                = ParserNode(Parser, AST_NODE_TYPE::IF, Start);
        Node->If.Condition  = ParserCondition(Parser);
        Node->If.ThenBranch = ParserStatement(Parser);

        if (ParserMatch(Parser, TOKENS::TOKEN_KW_ELSE))
        {
            Node->If.ElseBranch = ParserStatement(Parser);
        }
    }
    else if (ParserMatch(Parser, TOKENS::TOKEN_KW_WHILE))
    {
        Node                  = ParserNode(Parser, AST_NODE_TYPE::WHILE, Start);
        Node->While.Condition = ParserCondition(Parser);
        Node->While.Body      = ParserLoopBody(Parser);
    }
    else if (ParserMatch(Parser, TOKENS::TOKEN_KW_FOR))
    {
        Node = ParserNode(Parser, AST_NODE_TYPE::FOR, Start);
        ParserConsume(Parser, TOKENS::TOKEN_SP_LPAREN, "(");

        if (!ParserIsDeclaration(Parser))
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR);
        }

        AST_NODE *Initializer = ParserDeclaration(Parser, FALSE, FALSE);

        if (Initializer->Type != AST_NODE_TYPE::VARIABLE_DECLARATION || Initializer->VariableDeclaration.Initializer == NULLPTR)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR);
        }

        Node->For.Initializer = Initializer;
        if (!ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA))
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR);
        }
        Node->For.Condition = ParserExpression(Parser);
        if (!ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA))
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR);
        }
        Node->For.Increment = ParserExpression(Parser);
        AST_NODE *Step      = Node->For.Increment;

        while (Step->Type == AST_NODE_TYPE::UNARY && !Step->Unary.Postfix && (Step->Unary.Operator == TOKENS::TOKEN_OP_PLUS || Step->Unary.Operator == TOKENS::TOKEN_OP_MINUS))
        {
            Step = Step->Unary.Operand;
        }

        if ((Step->Type == AST_NODE_TYPE::INTEGER && Step->Integer.Value == 0) || (Step->Type == AST_NODE_TYPE::FLOAT && Step->Float.Value == 0))
        {
            ParserFailAt(Parser, Node->For.Increment->Location, DIAGNOSTIC_MESSAGE::ZERO_RANGE_STEP);
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_RPAREN, ")");
        Node->For.Body = ParserLoopBody(Parser);
    }
    else if (Token == TOKENS::TOKEN_KW_BREAK || Token == TOKENS::TOKEN_KW_CONTINUE)
    {
        if (Parser->LoopDepth == 0)
        {
            ParserFailAt(Parser, Start, DIAGNOSTIC_MESSAGE::LOOP_CONTROL_OUTSIDE_LOOP);
        }

        ParserAdvance(Parser);
        ParserConsume(Parser, TOKENS::TOKEN_SP_SEMICOLON, ";", Token == TOKENS::TOKEN_KW_BREAK ? "break statement" : "continue statement");
        Node = ParserNode(Parser, Token == TOKENS::TOKEN_KW_BREAK ? AST_NODE_TYPE::BREAK : AST_NODE_TYPE::CONTINUE, Start);
    }
    else if (ParserMatch(Parser, TOKENS::TOKEN_KW_RETURN))
    {
        if (Parser->FunctionDepth == 0)
        {
            ParserFailAt(Parser, Start, DIAGNOSTIC_MESSAGE::RETURN_OUTSIDE_FUNCTION);
        }

        Node = ParserNode(Parser, AST_NODE_TYPE::RETURN, Start);

        if (ParserPeek(Parser) != TOKENS::TOKEN_SP_SEMICOLON)
        {
            Node->Return.Value = ParserExpression(Parser);
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_SEMICOLON, ";", "return statement");
    }
    else
    {
        Node = ParserNode(Parser, AST_NODE_TYPE::EXPRESSION_STATEMENT, Start);

        if (ParserPeek(Parser) != TOKENS::TOKEN_SP_SEMICOLON)
        {
            Node->ExpressionStatement.Expression = ParserExpression(Parser);
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_SEMICOLON, ";", "expression");
    }

    Node->Location = ParserFinish(Parser, Start);

    return Node;
}

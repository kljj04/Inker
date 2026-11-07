// Inker/Source/Parser/Expression.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>
#include <charconv>
#include <limits>
#include <cmath>

STATIC INT32 ParserPower(TOKENS Token)
{
    switch (Token)
    {
        case TOKENS::TOKEN_OP_ASSIGN:
        case TOKENS::TOKEN_OP_PLUS_ASSIGN:
        case TOKENS::TOKEN_OP_MINUS_ASSIGN:
        case TOKENS::TOKEN_OP_MULTIPLY_ASSIGN:
        case TOKENS::TOKEN_OP_DIVIDE_ASSIGN:
        case TOKENS::TOKEN_OP_MODULO_ASSIGN:
        case TOKENS::TOKEN_OP_BIT_AND_ASSIGN:
        case TOKENS::TOKEN_OP_BIT_OR_ASSIGN:
        case TOKENS::TOKEN_OP_BIT_XOR_ASSIGN:
        case TOKENS::TOKEN_OP_SHL_ASSIGN:
        case TOKENS::TOKEN_OP_SHR_ASSIGN:

            return 1;
        case TOKENS::TOKEN_SP_QUESTION:

            return 2;
        case TOKENS::TOKEN_OP_OR:

            return 3;
        case TOKENS::TOKEN_OP_AND:

            return 4;
        case TOKENS::TOKEN_OP_BIT_OR:

            return 5;
        case TOKENS::TOKEN_OP_BIT_XOR:

            return 6;
        case TOKENS::TOKEN_OP_BIT_AND:

            return 7;
        case TOKENS::TOKEN_OP_EQUAL:
        case TOKENS::TOKEN_OP_NOT_EQUAL:

            return 8;
        case TOKENS::TOKEN_OP_LESS:
        case TOKENS::TOKEN_OP_LESS_EQUAL:
        case TOKENS::TOKEN_OP_GREATER:
        case TOKENS::TOKEN_OP_GREATER_EQUAL:

            return 9;
        case TOKENS::TOKEN_OP_SHL:
        case TOKENS::TOKEN_OP_SHR:
        case TOKENS::TOKEN_OP_ROL:
        case TOKENS::TOKEN_OP_ROR:

            return 10;
        case TOKENS::TOKEN_OP_PLUS:
        case TOKENS::TOKEN_OP_MINUS:

            return 11;
        case TOKENS::TOKEN_OP_MULTIPLY:
        case TOKENS::TOKEN_OP_DIVIDE:
        case TOKENS::TOKEN_OP_MODULO:

            return 12;
        case TOKENS::TOKEN_SP_LPAREN:
        case TOKENS::TOKEN_SP_LBRACKET:
        case TOKENS::TOKEN_SP_DOT:
        case TOKENS::TOKEN_OP_INCREMENT:
        case TOKENS::TOKEN_OP_DECREMENT:

            return 14;
        default:

            return 0;
    }
}

BOOLEAN ParserAssignable(CONST AST_NODE *Node)
{
    return Node->Type == AST_NODE_TYPE::IDENTIFIER || Node->Type == AST_NODE_TYPE::MEMBER_ACCESS || Node->Type == AST_NODE_TYPE::ARRAY_ACCESS || (Node->Type == AST_NODE_TYPE::UNARY && Node->Unary.Operator == TOKENS::TOKEN_OP_MULTIPLY && !Node->Unary.Postfix);
}

STATIC UINT64 ParserInteger(PARSER *Parser, STRING_VIEW Text)
{
    INT32 Base = 10;

    if (Text.size() > 2 && Text[0] == '0')
    {
        CHAR8 Prefix = Text[1];

        if (Prefix == 'x' || Prefix == 'X')
        {
            Base = 16;
        }

        if (Prefix == 'b' || Prefix == 'B')
        {
            Base = 2;
        }

        if (Prefix == 'o' || Prefix == 'O')
        {
            Base = 8;
        }

        if (Base != 10)
        {
            Text.remove_prefix(2);
        }
    }

    UINT64 Value  = 0;
    auto   Result = std::from_chars(Text.data(), Text.data() + Text.size(), Value, Base);

    if (Result.ec != std::errc{} || Result.ptr != Text.data() + Text.size())
    {
        ParserFail(Parser, DIAGNOSTIC_MESSAGE::OUT_OF_RANGE);
    }

    return Value;
}

STATIC AST_NODE *ParserPrimary(PARSER *Parser)
{
    TOKENS  Token = ParserPeek(Parser);
    AST_LOC Start = ParserLocation(Parser);

    if (Token == TOKENS::TOKEN_LIT_STRING)
    {
        return ParserString(Parser);
    }

    if (ParserMatch(Parser, TOKENS::TOKEN_SP_LPAREN))
    {
        AST_NODE *Node = ParserExpression(Parser);
        ParserConsume(Parser, TOKENS::TOKEN_SP_RPAREN, ")");
        Node->Location = ParserFinish(Parser, Start);

        return Node;
    }

    if (ParserMatch(Parser, TOKENS::TOKEN_SP_LBRACKET))
    {
        AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::ARRAY, Start);

        if (ParserPeek(Parser) != TOKENS::TOKEN_SP_RBRACKET)
        {
            do
            {
                ParserAppend(Parser, &Node->Array.Elements, ParserExpression(Parser));

                if (ParserPeek(Parser) == TOKENS::TOKEN_SP_RBRACKET)
                {
                    break;
                }
            } while (ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA));
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_RBRACKET, "]");
        Node->Location = ParserFinish(Parser, Start);

        return Node;
    }

    if (Token == TOKENS::TOKEN_OP_PLUS || Token == TOKENS::TOKEN_OP_MINUS || Token == TOKENS::TOKEN_OP_NOT || Token == TOKENS::TOKEN_OP_BIT_NOT || Token == TOKENS::TOKEN_OP_INCREMENT || Token == TOKENS::TOKEN_OP_DECREMENT || Token == TOKENS::TOKEN_OP_MULTIPLY || Token == TOKENS::TOKEN_OP_BIT_AND)
    {
        ParserAdvance(Parser);

        if (Token == TOKENS::TOKEN_OP_MINUS && ParserPeek(Parser) == TOKENS::TOKEN_LIT_INTEGER && ParserInteger(Parser, ParserText(Parser)) == UINT64(std::numeric_limits<INT64>::max()) + 1)
        {
            ParserAdvance(Parser);
            AST_NODE *Node      = ParserNode(Parser, AST_NODE_TYPE::INTEGER, ParserFinish(Parser, Start));
            Node->Integer.Value = std::numeric_limits<INT64>::min();

            return Node;
        }

        AST_NODE *Operand = ParserExpression(Parser, 13);

        if ((Token == TOKENS::TOKEN_OP_INCREMENT || Token == TOKENS::TOKEN_OP_DECREMENT || Token == TOKENS::TOKEN_OP_BIT_AND) && !ParserAssignable(Operand))
        {
            ParserFailAt(Parser, Start, Token == TOKENS::TOKEN_OP_BIT_AND ? DIAGNOSTIC_MESSAGE::INVALID_OPERAND : DIAGNOSTIC_MESSAGE::INVALID_INCREMENT_TARGET);
        }

        AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::UNARY, ParserFinish(Parser, Start));
        Node->Unary    = {Token, Operand, FALSE};

        return Node;
    }

    AST_NODE *Node = NULLPTR;

    if (Token == TOKENS::TOKEN_LIT_INTEGER)
    {
        UINT64 Value = ParserInteger(Parser, ParserText(Parser));

        if (Value > UINT64(std::numeric_limits<INT64>::max()))
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::OUT_OF_RANGE);
        }

        Node                = ParserNode(Parser, AST_NODE_TYPE::INTEGER, Start);
        Node->Integer.Value = static_cast<INT64>(Value);
    }
    else if (Token == TOKENS::TOKEN_LIT_FLOAT)
    {
        STRING_VIEW Text   = ParserText(Parser);
        FLOAT64     Value  = 0;
        auto        Result = std::from_chars(Text.data(), Text.data() + Text.size(), Value);

        if (Result.ec != std::errc{} || Result.ptr != Text.data() + Text.size() || !std::isfinite(Value))
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::OUT_OF_RANGE);
        }

        Node              = ParserNode(Parser, AST_NODE_TYPE::FLOAT, Start);
        Node->Float.Value = Value;
    }
    else if (Token == TOKENS::TOKEN_IDENTIFIER)
    {
        Node                  = ParserNode(Parser, AST_NODE_TYPE::IDENTIFIER, Start);
        Node->Identifier.Name = ParserText(Parser);
    }
    else if (Token == TOKENS::TOKEN_KW_TRUE || Token == TOKENS::TOKEN_KW_FALSE)
    {
        Node                = ParserNode(Parser, AST_NODE_TYPE::BOOLEAN, Start);
        Node->Boolean.Value = Token == TOKENS::TOKEN_KW_TRUE;
    }
    else if (Token == TOKENS::TOKEN_KW_NULL)
    {
        Node = ParserNode(Parser, AST_NODE_TYPE::NULL_LITERAL, Start);
    }
    else
    {
        ParserFail(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_EXPRESSION);
    }

    ParserAdvance(Parser);

    return Node;
}

AST_NODE *ParserExpression(PARSER *Parser, INT32 MinPower)
{
    PARSER_DEPTH Guard(Parser);
    AST_NODE    *Left = ParserPrimary(Parser);

    while (ParserPower(ParserPeek(Parser)) >= MinPower)
    {
        TOKENS  Token            = ParserPeek(Parser);
        INT32   Power            = ParserPower(Token);
        AST_LOC Start            = Left->Location;
        AST_LOC OperatorLocation = ParserLocation(Parser);
        ParserAdvance(Parser);

        if (Token == TOKENS::TOKEN_SP_LPAREN)
        {
            AST_NODE *Node            = ParserNode(Parser, AST_NODE_TYPE::FUNCTION_CALL, Start);
            Node->FunctionCall.Callee = Left;

            if (ParserPeek(Parser) != TOKENS::TOKEN_SP_RPAREN)
            {
                do
                {
                    AST_LOC   ArgumentStart  = ParserLocation(Parser);
                    AST_NODE *Value          = ParserExpression(Parser);
                    AST_NODE *Argument       = ParserNode(Parser, AST_NODE_TYPE::ARGUMENT, ParserFinish(Parser, ArgumentStart));
                    Argument->Argument.Value = Value;
                    ParserAppend(Parser, &Node->FunctionCall.Arguments, Argument);
                } while (ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA));
            }

            ParserConsume(Parser, TOKENS::TOKEN_SP_RPAREN, ")");
            Left = Node;
        }
        else if (Token == TOKENS::TOKEN_SP_LBRACKET)
        {
            AST_NODE *Index = ParserExpression(Parser);
            ParserConsume(Parser, TOKENS::TOKEN_SP_RBRACKET, "]");
            AST_NODE *Node    = ParserNode(Parser, AST_NODE_TYPE::ARRAY_ACCESS, Start);
            Node->ArrayAccess = {Left, Index};
            Left              = Node;
        }
        else if (Token == TOKENS::TOKEN_SP_DOT)
        {
            STRING_VIEW Name = ParserText(Parser);
            ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "identifier");
            AST_NODE *Node     = ParserNode(Parser, AST_NODE_TYPE::MEMBER_ACCESS, Start);
            Node->MemberAccess = {Left, Name};
            Left               = Node;
        }
        else if (Token == TOKENS::TOKEN_OP_INCREMENT || Token == TOKENS::TOKEN_OP_DECREMENT)
        {
            if (!ParserAssignable(Left))
            {
                ParserFailAt(Parser, OperatorLocation, DIAGNOSTIC_MESSAGE::INVALID_INCREMENT_TARGET);
            }

            AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::UNARY, Start);
            Node->Unary    = {Token, Left, TRUE};
            Left           = Node;
        }
        else if (Token == TOKENS::TOKEN_SP_QUESTION)
        {
            AST_NODE *Then = ParserExpression(Parser);
            ParserConsume(Parser, TOKENS::TOKEN_SP_COLON, ":");
            AST_NODE *Else = ParserExpression(Parser, 1);
            AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::TERNARY, Start);
            Node->Ternary  = {Left, Then, Else};
            Left           = Node;
        }
        else
        {
            if (Power == 1 && !ParserAssignable(Left))
            {
                ParserFailAt(Parser, OperatorLocation, DIAGNOSTIC_MESSAGE::INVALID_ASSIGNMENT_TARGET);
            }

            AST_NODE *Right = ParserExpression(Parser, Power == 1 ? Power : Power + 1);
            AST_NODE *Node  = ParserNode(Parser, Power == 1 ? AST_NODE_TYPE::ASSIGNMENT : AST_NODE_TYPE::BINARY, Start);

            if (Power == 1)
            {
                Node->Assignment = {Token, Left, Right};
            }
            else
            {
                Node->Binary = {Token, Left, Right};
            }

            Left = Node;
        }

        Left->Location = ParserFinish(Parser, Start);
    }

    return Left;
}

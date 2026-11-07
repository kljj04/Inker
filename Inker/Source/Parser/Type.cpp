// Inker/Source/Parser/Type.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>

BOOLEAN ParserIsType(TOKENS Token)
{
    return (Token >= TOKENS::TOKEN_TYPE_VOID && Token <= TOKENS::TOKEN_TYPE_AUTO64) || Token == TOKENS::TOKEN_KW_LONG || Token == TOKENS::TOKEN_KW_SIGNED || Token == TOKENS::TOKEN_KW_UNSIGNED;
}

AST_NODE *ParserType(PARSER *Parser)
{
    PARSER_DEPTH Guard(Parser);
    AST_LOC      Start = ParserLocation(Parser);
    TOKENS       Token = ParserPeek(Parser);

    if (!ParserIsType(Token) && Token != TOKENS::TOKEN_IDENTIFIER)
    {
        ParserFail(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_TYPE);
    }

    BOOLEAN     Compound = Token == TOKENS::TOKEN_KW_SIGNED || Token == TOKENS::TOKEN_KW_UNSIGNED || Token == TOKENS::TOKEN_KW_LONG;
    STRING_VIEW Word     = ParserText(Parser);
    Compound             = Compound || Word == "int" || Word == "short" || Word == "char" || Word == "double";

    if (Compound)
    {
        BOOLEAN Unsigned  = FALSE;
        BOOLEAN Signed    = FALSE;
        BOOLEAN Short     = FALSE;
        BOOLEAN Int       = FALSE;
        BOOLEAN Character = FALSE;
        BOOLEAN Double    = FALSE;
        SIZE    Longs     = 0;

        while (TRUE)
        {
            STRING_VIEW Part = ParserText(Parser);

            if (Part == "signed" || Part == "unsigned")
            {
                if (Signed || Unsigned)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }

                Unsigned = Part == "unsigned";
                Signed   = !Unsigned;
            }
            else if (Part == "long")
            {
                if (++Longs > 2)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }
            }
            else if (Part == "short")
            {
                if (Short)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }

                Short = TRUE;
            }
            else if (Part == "int")
            {
                if (Int)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }

                Int = TRUE;
            }
            else if (Part == "char")
            {
                if (Character)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }

                Character = TRUE;
            }
            else if (Part == "double")
            {
                if (Double)
                {
                    ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
                }

                Double = TRUE;
            }
            else
            {
                break;
            }

            ParserAdvance(Parser);
        }

        if ((Short && Longs > 0) || (Character && (Int || Short || Longs > 0 || Double)) || (Double && (Int || Short || Signed || Unsigned)))
        {
            ParserFailAt(Parser, ParserFinish(Parser, Start), DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS);
        }

        if (Double && Longs > 0)
        {
            ParserFailAt(Parser, ParserFinish(Parser, Start), DIAGNOSTIC_MESSAGE::UNSUPPORTED_FEATURE, "long double");
        }

        if (Double)
        {
            Token = TOKENS::TOKEN_TYPE_FLOAT64;
        }
        else if (Character)
        {
            Token = TOKENS::TOKEN_TYPE_CHAR8;

            if (Unsigned)
            {
                Token = TOKENS::TOKEN_TYPE_UINT8;
            }
            else if (Signed)
            {
                Token = TOKENS::TOKEN_TYPE_INT8;
            }
        }
        else if (Longs > 0)
        {
            Token = Unsigned ? TOKENS::TOKEN_TYPE_UINT64 : TOKENS::TOKEN_TYPE_INT64;
        }
        else if (Short)
        {
            Token = Unsigned ? TOKENS::TOKEN_TYPE_UINT16 : TOKENS::TOKEN_TYPE_INT16;
        }
        else
        {
            Token = Unsigned ? TOKENS::TOKEN_TYPE_UINT32 : TOKENS::TOKEN_TYPE_INT32;
        }
    }
    else
    {
        ParserAdvance(Parser);

        if (Token == TOKENS::TOKEN_IDENTIFIER)
        {
            while (ParserMatch(Parser, TOKENS::TOKEN_SP_DOT))
            {
                ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "type name");
            }
        }
    }

    AST_NODE *Node = ParserNode(Parser, AST_NODE_TYPE::TYPE, ParserFinish(Parser, Start));
    Node->TypeInfo = {Token, STRING_VIEW(Parser->Lexer->Source).substr(Start.Start, Node->Location.Length)};
    while (ParserMatch(Parser, TOKENS::TOKEN_OP_MULTIPLY))
    {
        AST_NODE *Pointer             = ParserNode(Parser, AST_NODE_TYPE::POINTER_TYPE, ParserFinish(Parser, Start));
        Pointer->PointerType.BaseType = Node;
        Node                          = Pointer;
    }

    Node->Location = ParserFinish(Parser, Start);
    return Node;
}

AST_NODE *ParserArrayDeclarator(PARSER *Parser, AST_NODE *Type)
{
    AST_LOC                 Start = Type->Location;
    std::vector<AST_NODE *> Dimensions;

    while (ParserMatch(Parser, TOKENS::TOKEN_SP_LBRACKET))
    {
        AST_NODE *Size = ParserPeek(Parser) == TOKENS::TOKEN_SP_RBRACKET ? NULLPTR : ParserExpression(Parser, 2);

        if (Size != NULLPTR && ((Size->Type == AST_NODE_TYPE::INTEGER && Size->Integer.Value <= 0) || (Size->Type == AST_NODE_TYPE::UNARY && Size->Unary.Operator == TOKENS::TOKEN_OP_MINUS && Size->Unary.Operand->Type == AST_NODE_TYPE::INTEGER)))
        {
            ParserFailAt(Parser, Size->Location, DIAGNOSTIC_MESSAGE::INVALID_ARRAY_SIZE);
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_RBRACKET, "]");
        AST_NODE *Array       = ParserNode(Parser, AST_NODE_TYPE::ARRAY_TYPE, ParserFinish(Parser, Start));
        Array->ArrayType.Size = Size;
        Dimensions.push_back(Array);
    }

    for (auto It = Dimensions.rbegin(); It != Dimensions.rend(); ++It)
    {
        (*It)->ArrayType.ElementType = Type;
        Type                         = *It;
    }

    return Type;
}

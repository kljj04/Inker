// Inker/Source/Parser/Interpolation.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>
#include <cstring>
#include <new>

STATIC STRING_VIEW ParserDecode(PARSER *Parser, STRING_VIEW Text)
{
    if (Text.find('\\') == STRING_VIEW::npos)
    {
        return Text;
    }

    if (Text.empty())
    {
        return {};
    }

    CHAR8 *Storage = static_cast<CHAR8 *>(ASTArenaAllocate(&Parser->Arena, Text.size(), ALIGNOF(CHAR8)));

    if (Storage == NULLPTR)
    {
        throw std::bad_alloc{};
    }

    SIZE Count = 0;

    for (SIZE Index = 0; Index < Text.size(); Index++)
    {
        CHAR8 Value = Text[Index];

        if (Value == '\\')
        {
            if (++Index == Text.size())
            {
                ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_ESCAPE);
            }

            Value = Text[Index];

            switch (Value)
            {
                case '0':
                    Value = '\0';
                    break;
                case 'a':
                    Value = '\a';
                    break;
                case 'b':
                    Value = '\b';
                    break;
                case 'f':
                    Value = '\f';
                    break;
                case 'n':
                    Value = '\n';
                    break;
                case 'r':
                    Value = '\r';
                    break;
                case 't':
                    Value = '\t';
                    break;
                case 'v':
                    Value = '\v';
                    break;
                default:
                    break;
            }
        }

        Storage[Count++] = Value;
    }

    return STRING_VIEW(Storage, Count);
}

AST_NODE *ParserString(PARSER *Parser)
{
    PARSER_DEPTH Guard(Parser);
    AST_LOC      Start   = ParserLocation(Parser);
    AST_NODE    *Result  = NULLPTR;
    AST_NODE    *First   = NULLPTR;
    BOOLEAN      Initial = TRUE;

    for (;;)
    {
        if (ParserPeek(Parser) == TOKENS::TOKEN_LIT_STRING)
        {
            AST_LOC     Location = ParserLocation(Parser);
            STRING_VIEW Text     = ParserText(Parser);
            BOOLEAN     End      = Parser->Tokens[Parser->Position].StringEnd;

            if (Initial)
            {
                Text.remove_prefix(1);
                Initial = FALSE;
            }

            if (End)
            {
                Text.remove_suffix(1);
            }

            AST_NODE *Segment     = ParserNode(Parser, AST_NODE_TYPE::STRING, Location);
            Segment->String.Value = ParserDecode(Parser, Text);
            ParserAdvance(Parser);

            if (Result != NULLPTR)
            {
                ParserAppend(Parser, &Result->StringInterpolation.Segments, Segment);
            }
            else
            {
                First = Segment;
            }

            if (End)
            {
                if (Result == NULLPTR)
                {
                    Result = First;
                }

                Result->Location = ParserFinish(Parser, Start);

                return Result;
            }
        }

        if (Result == NULLPTR)
        {
            Result = ParserNode(Parser, AST_NODE_TYPE::STRING_INTERPOLATION, Start);

            if (First != NULLPTR)
            {
                ParserAppend(Parser, &Result->StringInterpolation.Segments, First);
            }
        }

        AST_LOC Location = ParserLocation(Parser);
        ParserConsume(Parser, TOKENS::TOKEN_SP_INTERPOLATION_START, "[");
        AST_NODE *Expression = ParserExpression(Parser);
        ParserConsume(Parser, TOKENS::TOKEN_SP_INTERPOLATION_END, "]");
        AST_NODE *Segment                           = ParserNode(Parser, AST_NODE_TYPE::INTERPOLATION_EXPRESSION, ParserFinish(Parser, Location));
        Segment->InterpolationExpression.Expression = Expression;
        ParserAppend(Parser, &Result->StringInterpolation.Segments, Segment);
    }
}

// Inker/Source/Lexer/Lexer.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Lexer/Tokens.hpp>
#include <Lexer/Lexer.hpp>

VOID LexerInit(LEXER *Lexer, CONST STRING &Source)
{
    Lexer->Source      = Source;
    Lexer->Position    = 0;
    Lexer->Line        = 1;
    Lexer->Column      = 1;
    Lexer->TokenStart  = 0;
    Lexer->TokenLength = 0;
    Lexer->TokenLine   = 1;
    Lexer->TokenColumn = 1;
    Lexer->Error       = LEXER_ERROR::NONE;
    Lexer->ErrorLine   = 0;
    Lexer->ErrorColumn = 0;
    Lexer->FrameCount  = 0;
}

STATIC CHAR8 LexerPeek(CONST LEXER *Lexer, SIZE Offset)
{
    if (Offset >= Lexer->Source.size() - Lexer->Position)
    {
        return '\0';
    }

    return Lexer->Source[Lexer->Position + Offset];
}

STATIC CHAR8 LexerAdvance(LEXER *Lexer)
{
    if (Lexer->Position >= Lexer->Source.size())
    {
        return '\0';
    }

    CHAR8 Character = Lexer->Source[Lexer->Position++];

    if (Character == '\n')
    {
        Lexer->Line++;
        Lexer->Column = 1;
    }
    else
    {
        Lexer->Column++;
    }

    return Character;
}

STATIC BOOLEAN LexerMatch(LEXER *Lexer, CHAR8 Expected)
{
    if (LexerPeek(Lexer, 0) != Expected || Lexer->Position >= Lexer->Source.size())
    {
        return FALSE;
    }

    LexerAdvance(Lexer);
    return TRUE;
}

STATIC TOKENS LexerSetError(LEXER *Lexer, LEXER_ERROR Error)
{
    if (Lexer->Error == LEXER_ERROR::NONE)
    {
        Lexer->Error       = Error;
        Lexer->ErrorLine   = Lexer->Line;
        Lexer->ErrorColumn = Lexer->Column;
    }

    return TOKENS::TOKEN_UNKNOWN;
}

STATIC BOOLEAN LexerIsDigit(CHAR8 Character)
{
    return Character >= '0' && Character <= '9';
}

STATIC BOOLEAN LexerIsAlpha(CHAR8 Character)
{
    return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z');
}

STATIC BOOLEAN LexerIsIdentifierStart(CHAR8 Character)
{
    return LexerIsAlpha(Character) || Character == '_';
}

STATIC BOOLEAN LexerIsIdentifierPart(CHAR8 Character)
{
    return LexerIsIdentifierStart(Character) || LexerIsDigit(Character);
}

STATIC BOOLEAN LexerIsDigitBase(CHAR8 Character, UINT8 Base)
{
    if (Character >= '0' && Character <= '9')
    {
        return Character - '0' < Base;
    }

    if (Base == 16)
    {
        if (Character >= 'a' && Character <= 'f')
        {
            return TRUE;
        }

        if (Character >= 'A' && Character <= 'F')
        {
            return TRUE;
        }
    }

    return FALSE;
}

STATIC BOOLEAN LexerPushFrame(LEXER *Lexer, LEXER_MODE Mode, CHAR8 Quote)
{
    if (Lexer->FrameCount >= 64)
    {
        LexerSetError(Lexer, LEXER_ERROR::NESTING_LIMIT);
        return FALSE;
    }

    LEXER_FRAME *Frame = &Lexer->Frames[Lexer->FrameCount++];

    Frame->Mode         = Mode;
    Frame->Quote        = Quote;
    Frame->BracketDepth = 0;

    return TRUE;
}

STATIC VOID LexerPopFrame(LEXER *Lexer)
{
    if (Lexer->FrameCount > 0)
    {
        Lexer->FrameCount--;
    }
}

STATIC BOOLEAN LexerSkipIgnored(LEXER *Lexer)
{
    while (Lexer->Position < Lexer->Source.size())
    {
        CHAR8 Character = LexerPeek(Lexer, 0);

        if (Character == ' ' || Character == '\t' || Character == '\r' || Character == '\n')
        {
            LexerAdvance(Lexer);
            continue;
        }

        if (Character == '/' && LexerPeek(Lexer, 1) == '/')
        {
            LexerAdvance(Lexer);
            LexerAdvance(Lexer);

            while (Lexer->Position < Lexer->Source.size() && LexerPeek(Lexer, 0) != '\n')
            {
                LexerAdvance(Lexer);
            }

            continue;
        }

        if (Character == '/' && LexerPeek(Lexer, 1) == '*')
        {
            LexerAdvance(Lexer);
            LexerAdvance(Lexer);

            BOOLEAN Closed = FALSE;

            while (Lexer->Position < Lexer->Source.size())
            {
                if (LexerPeek(Lexer, 0) == '*' && LexerPeek(Lexer, 1) == '/')
                {
                    LexerAdvance(Lexer);
                    LexerAdvance(Lexer);
                    Closed = TRUE;
                    break;
                }

                LexerAdvance(Lexer);
            }

            if (!Closed)
            {
                LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_COMMENT);
                return FALSE;
            }

            continue;
        }

        break;
    }

    return TRUE;
}

STATIC TOKENS LexerIdentifyKW(LEXER *Lexer)
{
    while (LexerIsIdentifierPart(LexerPeek(Lexer, 0)) && Lexer->Position < Lexer->Source.size())
    {
        LexerAdvance(Lexer);
    }

    STRING_VIEW Value(Lexer->Source.data() + Lexer->TokenStart, Lexer->Position - Lexer->TokenStart);

    if (Value == "use")
    {
        return TOKENS::TOKEN_KW_USE;
    }

    if (Value == "import")
    {
        return TOKENS::TOKEN_KW_IMPORT;
    }

    if (Value == "as")
    {
        return TOKENS::TOKEN_KW_AS;
    }

    if (Value == "set")
    {
        return TOKENS::TOKEN_KW_SET;
    }

    if (Value == "namespace")
    {
        return TOKENS::TOKEN_KW_NAMESPACE;
    }

    if (Value == "struct")
    {
        return TOKENS::TOKEN_KW_STRUCT;
    }

    if (Value == "enum")
    {
        return TOKENS::TOKEN_KW_ENUM;
    }

    if (Value == "class")
    {
        return TOKENS::TOKEN_KW_CLASS;
    }

    if (Value == "if")
    {
        return TOKENS::TOKEN_KW_IF;
    }

    if (Value == "else")
    {
        return TOKENS::TOKEN_KW_ELSE;
    }

    if (Value == "while")
    {
        return TOKENS::TOKEN_KW_WHILE;
    }

    if (Value == "for")
    {
        return TOKENS::TOKEN_KW_FOR;
    }

    if (Value == "break")
    {
        return TOKENS::TOKEN_KW_BREAK;
    }

    if (Value == "continue")
    {
        return TOKENS::TOKEN_KW_CONTINUE;
    }

    if (Value == "return")
    {
        return TOKENS::TOKEN_KW_RETURN;
    }

    if (Value == "true")
    {
        return TOKENS::TOKEN_KW_TRUE;
    }

    if (Value == "false")
    {
        return TOKENS::TOKEN_KW_FALSE;
    }

    if (Value == "null")
    {
        return TOKENS::TOKEN_KW_NULL;
    }

    if (Value == "long")
    {
        return TOKENS::TOKEN_KW_LONG;
    }

    if (Value == "signed")
    {
        return TOKENS::TOKEN_KW_SIGNED;
    }

    if (Value == "unsigned")
    {
        return TOKENS::TOKEN_KW_UNSIGNED;
    }

    if (Value == "public" || Value == "PUBLIC" || Value == "Public")
    {
        return TOKENS::TOKEN_KW_PUBLIC;
    }

    if (Value == "private" || Value == "PRIVATE" || Value == "Private")
    {
        return TOKENS::TOKEN_KW_PRIVATE;
    }

    if (Value == "protected" || Value == "PROTECTED" || Value == "Protected")
    {
        return TOKENS::TOKEN_KW_PROTECTED;
    }

    if (Value == "VOID" || Value == "void")
    {
        return TOKENS::TOKEN_TYPE_VOID;
    }

    if (Value == "bool" || Value == "boolean" || Value == "BOOL" || Value == "BOOLEAN")
    {
        return TOKENS::TOKEN_TYPE_BOOLEAN;
    }

    if (Value == "string" || Value == "STRING")
    {
        return TOKENS::TOKEN_TYPE_STRING;
    }

    if (Value == "INT8")
    {
        return TOKENS::TOKEN_TYPE_INT8;
    }

    if (Value == "INT16" || Value == "short")
    {
        return TOKENS::TOKEN_TYPE_INT16;
    }

    if (Value == "INT32" || Value == "int")
    {
        return TOKENS::TOKEN_TYPE_INT32;
    }

    if (Value == "INT64")
    {
        return TOKENS::TOKEN_TYPE_INT64;
    }

    if (Value == "UINT8")
    {
        return TOKENS::TOKEN_TYPE_UINT8;
    }

    if (Value == "UINT16")
    {
        return TOKENS::TOKEN_TYPE_UINT16;
    }

    if (Value == "UINT32")
    {
        return TOKENS::TOKEN_TYPE_UINT32;
    }

    if (Value == "UINT64")
    {
        return TOKENS::TOKEN_TYPE_UINT64;
    }

    if (Value == "FLOAT32" || Value == "float" || Value == "FLOAT")
    {
        return TOKENS::TOKEN_TYPE_FLOAT32;
    }

    if (Value == "FLOAT64" || Value == "double")
    {
        return TOKENS::TOKEN_TYPE_FLOAT64;
    }

    if (Value == "char" || Value == "CHAR8")
    {
        return TOKENS::TOKEN_TYPE_CHAR8;
    }

    if (Value == "CHAR16")
    {
        return TOKENS::TOKEN_TYPE_CHAR16;
    }

    if (Value == "AUTO8")
    {
        return TOKENS::TOKEN_TYPE_AUTO8;
    }

    if (Value == "AUTO16")
    {
        return TOKENS::TOKEN_TYPE_AUTO16;
    }

    if (Value == "AUTO32" || Value == "auto")
    {
        return TOKENS::TOKEN_TYPE_AUTO32;
    }

    if (Value == "AUTO64")
    {
        return TOKENS::TOKEN_TYPE_AUTO64;
    }

    return TOKENS::TOKEN_IDENTIFIER;
}

STATIC TOKENS LexerIdentifyNumber(LEXER *Lexer)
{
    UINT8   Base    = 10;
    BOOLEAN IsFloat = FALSE;

    if (LexerPeek(Lexer, 0) == '0')
    {
        CHAR8 Prefix = LexerPeek(Lexer, 1);

        if (Prefix == 'x' || Prefix == 'X')
        {
            Base = 16;
        }
        else if (Prefix == 'b' || Prefix == 'B')
        {
            Base = 2;
        }
        else if (Prefix == 'o' || Prefix == 'O')
        {
            Base = 8;
        }

        if (Base != 10)
        {
            LexerAdvance(Lexer);
            LexerAdvance(Lexer);

            SIZE DigitCount = 0;

            while (LexerIsDigitBase(LexerPeek(Lexer, 0), Base) && Lexer->Position < Lexer->Source.size())
            {
                LexerAdvance(Lexer);
                DigitCount++;
            }

            if (DigitCount == 0 || LexerIsIdentifierPart(LexerPeek(Lexer, 0)))
            {
                return LexerSetError(Lexer, LEXER_ERROR::INVALID_NUMBER);
            }

            return TOKENS::TOKEN_LIT_INTEGER;
        }
    }

    if (LexerPeek(Lexer, 0) == '.')
    {
        IsFloat = TRUE;
        LexerAdvance(Lexer);
    }

    while (LexerIsDigit(LexerPeek(Lexer, 0)) && Lexer->Position < Lexer->Source.size())
    {
        LexerAdvance(Lexer);
    }

    if (!IsFloat && LexerPeek(Lexer, 0) == '.' && LexerPeek(Lexer, 1) != '.')
    {
        IsFloat = TRUE;
        LexerAdvance(Lexer);

        while (LexerIsDigit(LexerPeek(Lexer, 0)) && Lexer->Position < Lexer->Source.size())
        {
            LexerAdvance(Lexer);
        }
    }

    if (LexerPeek(Lexer, 0) == 'e' || LexerPeek(Lexer, 0) == 'E')
    {
        IsFloat = TRUE;
        LexerAdvance(Lexer);

        if (LexerPeek(Lexer, 0) == '+' || LexerPeek(Lexer, 0) == '-')
        {
            LexerAdvance(Lexer);
        }

        if (!LexerIsDigit(LexerPeek(Lexer, 0)))
        {
            return LexerSetError(Lexer, LEXER_ERROR::INVALID_NUMBER);
        }

        while (LexerIsDigit(LexerPeek(Lexer, 0)) && Lexer->Position < Lexer->Source.size())
        {
            LexerAdvance(Lexer);
        }
    }

    if (LexerIsIdentifierPart(LexerPeek(Lexer, 0)))
    {
        return LexerSetError(Lexer, LEXER_ERROR::INVALID_NUMBER);
    }

    return IsFloat ? TOKENS::TOKEN_LIT_FLOAT : TOKENS::TOKEN_LIT_INTEGER;
}

STATIC BOOLEAN LexerIsValidEscape(CHAR8 Character)
{
    switch (Character)
    {
        case '0':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case '\\':
        case '\'':
        case '"':
        case '[':
        case ']':
        {
            return TRUE;
        }

        default:
        {
            return FALSE;
        }
    }
}

STATIC TOKENS LexerIdentifyString(LEXER *Lexer)
{
    if (Lexer->FrameCount == 0)
    {
        return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
    }

    LEXER_FRAME *Frame = &Lexer->Frames[Lexer->FrameCount - 1];

    if (Frame->Mode != LEXER_MODE::STRING)
    {
        return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
    }

    CHAR8 Quote = Frame->Quote;

    while (Lexer->Position < Lexer->Source.size())
    {
        CHAR8 Character = LexerPeek(Lexer, 0);

        if (Character == '\\')
        {
            LexerAdvance(Lexer);

            if (Lexer->Position >= Lexer->Source.size())
            {
                return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
            }

            if (!LexerIsValidEscape(LexerPeek(Lexer, 0)))
            {
                return LexerSetError(Lexer, LEXER_ERROR::INVALID_ESCAPE);
            }

            LexerAdvance(Lexer);
            continue;
        }

        if (Character == Quote)
        {
            LexerAdvance(Lexer);
            LexerPopFrame(Lexer);
            return TOKENS::TOKEN_LIT_STRING;
        }

        if (Character == '[')
        {
            if (Lexer->Position > Lexer->TokenStart)
            {
                return TOKENS::TOKEN_LIT_STRING;
            }

            LexerAdvance(Lexer);

            if (!LexerPushFrame(Lexer, LEXER_MODE::EXPRESSION, '\0'))
            {
                return TOKENS::TOKEN_UNKNOWN;
            }

            return TOKENS::TOKEN_SP_INTERPOLATION_START;
        }

        if (Character == '\n' || Character == '\r')
        {
            return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
        }

        LexerAdvance(Lexer);
    }

    return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
}

STATIC TOKENS LexerIdentifyOP(LEXER *Lexer)
{
    CHAR8 Character = LexerAdvance(Lexer);

    switch (Character)
    {
        case '+':
        {
            if (LexerMatch(Lexer, '+'))
            {
                return TOKENS::TOKEN_OP_INCREMENT;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_PLUS_ASSIGN;
            }

            return TOKENS::TOKEN_OP_PLUS;
        }

        case '-':
        {
            if (LexerMatch(Lexer, '-'))
            {
                return TOKENS::TOKEN_OP_DECREMENT;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_MINUS_ASSIGN;
            }

            return TOKENS::TOKEN_OP_MINUS;
        }

        case '*':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_MULTIPLY_ASSIGN;
            }

            return TOKENS::TOKEN_OP_MULTIPLY;
        }

        case '/':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_DIVIDE_ASSIGN;
            }

            return TOKENS::TOKEN_OP_DIVIDE;
        }

        case '%':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_MODULO_ASSIGN;
            }

            return TOKENS::TOKEN_OP_MODULO;
        }

        case '=':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_EQUAL;
            }

            return TOKENS::TOKEN_OP_ASSIGN;
        }

        case '!':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_NOT_EQUAL;
            }

            return TOKENS::TOKEN_OP_NOT;
        }

        case '&':
        {
            if (LexerMatch(Lexer, '&'))
            {
                return TOKENS::TOKEN_OP_AND;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_BIT_AND_ASSIGN;
            }

            return TOKENS::TOKEN_OP_BIT_AND;
        }

        case '|':
        {
            if (LexerMatch(Lexer, '|'))
            {
                return TOKENS::TOKEN_OP_OR;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_BIT_OR_ASSIGN;
            }

            return TOKENS::TOKEN_OP_BIT_OR;
        }

        case '^':
        {
            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_BIT_XOR_ASSIGN;
            }

            return TOKENS::TOKEN_OP_BIT_XOR;
        }

        case '~':
        {
            return TOKENS::TOKEN_OP_BIT_NOT;
        }

        case '<':
        {
            if (LexerMatch(Lexer, '<'))
            {
                if (LexerMatch(Lexer, '<'))
                {
                    return TOKENS::TOKEN_OP_ROL;
                }

                if (LexerMatch(Lexer, '='))
                {
                    return TOKENS::TOKEN_OP_SHL_ASSIGN;
                }

                return TOKENS::TOKEN_OP_SHL;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_LESS_EQUAL;
            }

            return TOKENS::TOKEN_OP_LESS;
        }

        case '>':
        {
            if (LexerMatch(Lexer, '>'))
            {
                if (LexerMatch(Lexer, '>'))
                {
                    return TOKENS::TOKEN_OP_ROR;
                }

                if (LexerMatch(Lexer, '='))
                {
                    return TOKENS::TOKEN_OP_SHR_ASSIGN;
                }

                return TOKENS::TOKEN_OP_SHR;
            }

            if (LexerMatch(Lexer, '='))
            {
                return TOKENS::TOKEN_OP_GREATER_EQUAL;
            }

            return TOKENS::TOKEN_OP_GREATER;
        }

        case '?':
        {
            return TOKENS::TOKEN_SP_QUESTION;
        }

        case '(':
        {
            return TOKENS::TOKEN_SP_LPAREN;
        }

        case ')':
        {
            return TOKENS::TOKEN_SP_RPAREN;
        }

        case '{':
        {
            return TOKENS::TOKEN_SP_LBRACE;
        }

        case '}':
        {
            return TOKENS::TOKEN_SP_RBRACE;
        }

        case '[':
        {
            if (Lexer->FrameCount > 0)
            {
                LEXER_FRAME *Frame = &Lexer->Frames[Lexer->FrameCount - 1];

                if (Frame->Mode == LEXER_MODE::EXPRESSION)
                {
                    Frame->BracketDepth++;
                }
            }

            return TOKENS::TOKEN_SP_LBRACKET;
        }

        case ']':
        {
            if (Lexer->FrameCount > 0)
            {
                LEXER_FRAME *Frame = &Lexer->Frames[Lexer->FrameCount - 1];

                if (Frame->Mode == LEXER_MODE::EXPRESSION)
                {
                    if (Frame->BracketDepth == 0)
                    {
                        LexerPopFrame(Lexer);
                        return TOKENS::TOKEN_SP_INTERPOLATION_END;
                    }

                    Frame->BracketDepth--;
                }
            }

            return TOKENS::TOKEN_SP_RBRACKET;
        }

        case ',':
        {
            return TOKENS::TOKEN_SP_COMMA;
        }

        case '.':
        {
            return TOKENS::TOKEN_SP_DOT;
        }

        case ':':
        {
            return TOKENS::TOKEN_SP_COLON;
        }

        case ';':
        {
            return TOKENS::TOKEN_SP_SEMICOLON;
        }

        default:
        {
            return LexerSetError(Lexer, LEXER_ERROR::INVALID_CHARACTER);
        }
    }
}

STATIC TOKENS LexerIdentify(LEXER *Lexer)
{
    if (Lexer->FrameCount > 0 && Lexer->Frames[Lexer->FrameCount - 1].Mode == LEXER_MODE::STRING)
    {
        return LexerIdentifyString(Lexer);
    }

    CHAR8 Character = LexerPeek(Lexer, 0);

    if (LexerIsIdentifierStart(Character))
    {
        return LexerIdentifyKW(Lexer);
    }

    if (LexerIsDigit(Character) || (Character == '.' && LexerIsDigit(LexerPeek(Lexer, 1))))
    {
        return LexerIdentifyNumber(Lexer);
    }

    if (Character == '"' || Character == '\'')
    {
        LexerAdvance(Lexer);

        if (!LexerPushFrame(Lexer, LEXER_MODE::STRING, Character))
        {
            return TOKENS::TOKEN_UNKNOWN;
        }

        return LexerIdentifyString(Lexer);
    }

    return LexerIdentifyOP(Lexer);
}

TOKENS LexerNextToken(LEXER *Lexer)
{
    if (Lexer->Error != LEXER_ERROR::NONE)
    {
        return TOKENS::TOKEN_EOF;
    }

    BOOLEAN InString = Lexer->FrameCount > 0 && Lexer->Frames[Lexer->FrameCount - 1].Mode == LEXER_MODE::STRING;

    if (!InString)
    {
        if (!LexerSkipIgnored(Lexer))
        {
            return TOKENS::TOKEN_UNKNOWN;
        }
    }

    Lexer->TokenStart  = Lexer->Position;
    Lexer->TokenLength = 0;
    Lexer->TokenLine   = Lexer->Line;
    Lexer->TokenColumn = Lexer->Column;

    if (Lexer->Position >= Lexer->Source.size())
    {
        if (Lexer->FrameCount > 0)
        {
            LEXER_FRAME *Frame = &Lexer->Frames[Lexer->FrameCount - 1];

            if (Frame->Mode == LEXER_MODE::EXPRESSION)
            {
                return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_INTERPOLATION);
            }

            return LexerSetError(Lexer, LEXER_ERROR::UNTERMINATED_STRING);
        }

        return TOKENS::TOKEN_EOF;
    }

    TOKENS Token = LexerIdentify(Lexer);

    Lexer->TokenLength = Lexer->Position - Lexer->TokenStart;

    return Token;
}
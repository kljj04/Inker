// Inker/Source/StaticAnalyzer/Operator.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

SEMANTIC_VALUE SemanticBinary(SEMANTIC_CONTEXT *Context, TOKENS Operator, SEMANTIC_VALUE Left, SEMANTIC_VALUE Right, AST_NODE *Site)
{
    STATIC_ANALYZER *Analyzer = Context->Analyzer;
    SEMANTIC_VALUE   Result;
    Result.Type    = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    Result.Node    = Site;
    Result.Literal = Left.Literal && Right.Literal;
    if (Left.Type->Kind == SEMANTIC_KIND::ERROR || Right.Type->Kind == SEMANTIC_KIND::ERROR)
    {
        return Result;
    }
    if (Context->EnumInitializer)
    {
        if (Left.Type->Kind == SEMANTIC_KIND::ENUM)
        {
            Left.Type = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 32);
        }
        if (Right.Type->Kind == SEMANTIC_KIND::ENUM)
        {
            Right.Type = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 32);
        }
    }
    BOOLEAN Equality = Operator == TOKENS::TOKEN_OP_EQUAL || Operator == TOKENS::TOKEN_OP_NOT_EQUAL;
    BOOLEAN Compare  = Equality || Operator == TOKENS::TOKEN_OP_LESS || Operator == TOKENS::TOKEN_OP_LESS_EQUAL || Operator == TOKENS::TOKEN_OP_GREATER || Operator == TOKENS::TOKEN_OP_GREATER_EQUAL;
    if (Left.Type->Kind == SEMANTIC_KIND::AUTO || Right.Type->Kind == SEMANTIC_KIND::AUTO)
    {
        Result.Type = Compare ? SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE) : Left.Type->Kind == SEMANTIC_KIND::AUTO ? Left.Type : Right.Type;
        return Result;
    }
    if (Left.Type->Kind == SEMANTIC_KIND::STRING && Right.Type->Kind == SEMANTIC_KIND::STRING && (Operator == TOKENS::TOKEN_OP_PLUS || Equality))
    {
        Result.Type = Equality ? SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE) : Left.Type;
        return Result;
    }
    if (Equality && ((Left.Type == Right.Type && (Left.Type->Kind == SEMANTIC_KIND::BOOLEAN || Left.Type->Kind == SEMANTIC_KIND::POINTER || Left.Type->Kind == SEMANTIC_KIND::ENUM || Left.Type->Kind == SEMANTIC_KIND::NULL_VALUE)) || (Left.Type->Kind == SEMANTIC_KIND::POINTER && Right.Type->Kind == SEMANTIC_KIND::NULL_VALUE) || (Right.Type->Kind == SEMANTIC_KIND::POINTER && Left.Type->Kind == SEMANTIC_KIND::NULL_VALUE)))
    {
        Result.Type = SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE);
        if (Left.IntegerKnown && Right.IntegerKnown)
        {
            Result.IntegerKnown = TRUE;
            Result.Integer      = Operator == TOKENS::TOKEN_OP_EQUAL ? Left.Integer == Right.Integer : Left.Integer != Right.Integer;
        }
        return Result;
    }
    if (!SemanticNumeric(Left.Type) || !SemanticNumeric(Right.Type))
    {
        SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_BINARY_TYPES, SemanticName(Left.Type), SemanticName(Right.Type));
        return Result;
    }
    BOOLEAN IntegralOnly = Operator == TOKENS::TOKEN_OP_MODULO || Operator == TOKENS::TOKEN_OP_BIT_AND || Operator == TOKENS::TOKEN_OP_BIT_OR || Operator == TOKENS::TOKEN_OP_BIT_XOR || Operator == TOKENS::TOKEN_OP_SHL || Operator == TOKENS::TOKEN_OP_SHR || Operator == TOKENS::TOKEN_OP_ROL || Operator == TOKENS::TOKEN_OP_ROR;
    if (IntegralOnly && (!SemanticIntegral(Left.Type) || !SemanticIntegral(Right.Type)))
    {
        SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_OPERAND, "integer operator");
        return Result;
    }
    SEMANTIC_TYPE *Common;
    if (Left.Type->Kind == SEMANTIC_KIND::FLOAT || Right.Type->Kind == SEMANTIC_KIND::FLOAT)
    {
        UINT16 Bits = std::max(Left.Type->Kind == SEMANTIC_KIND::FLOAT ? Left.Type->Bits : UINT16(0), Right.Type->Kind == SEMANTIC_KIND::FLOAT ? Right.Type->Bits : UINT16(0));
        Common      = SemanticType(Analyzer, SEMANTIC_KIND::FLOAT, Bits);
    }
    else
    {
        UINT16  LeftBits  = std::max(UINT16(32), Left.Type->Bits);
        UINT16  RightBits = std::max(UINT16(32), Right.Type->Bits);
        BOOLEAN Unsigned  = (Left.Type->Unsigned && Left.Type->Bits >= 32 && LeftBits >= RightBits) || (Right.Type->Unsigned && Right.Type->Bits >= 32 && RightBits >= LeftBits);
        Common            = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, std::max(LeftBits, RightBits), Unsigned);
    }
    Left        = SemanticConvert(Context, Left, Common, Left.Node == NULLPTR ? Site : Left.Node);
    Right       = SemanticConvert(Context, Right, Common, Right.Node == NULLPTR ? Site : Right.Node);
    Result.Type = Compare ? SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE) : Common;
    if (Common->Kind == SEMANTIC_KIND::FLOAT)
    {
        if (!Left.FloatKnown || !Right.FloatKnown)
        {
            return Result;
        }
        FLOAT64 L = Left.Float;
        FLOAT64 R = Right.Float;
        if (Operator == TOKENS::TOKEN_OP_DIVIDE && R == 0)
        {
            if (!Context->Unreachable)
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::DIVISION_BY_ZERO);
            }
            return Result;
        }
        if (Compare)
        {
            Result.IntegerKnown = TRUE;
            switch (Operator)
            {
                case TOKENS::TOKEN_OP_EQUAL:
                    Result.Integer = L == R;
                    break;
                case TOKENS::TOKEN_OP_NOT_EQUAL:
                    Result.Integer = L != R;
                    break;
                case TOKENS::TOKEN_OP_LESS:
                    Result.Integer = L < R;
                    break;
                case TOKENS::TOKEN_OP_LESS_EQUAL:
                    Result.Integer = L <= R;
                    break;
                case TOKENS::TOKEN_OP_GREATER:
                    Result.Integer = L > R;
                    break;
                default:
                    Result.Integer = L >= R;
                    break;
            }
        }
        else
        {
            Result.FloatKnown = TRUE;
            switch (Operator)
            {
                case TOKENS::TOKEN_OP_PLUS:
                    Result.Float = L + R;
                    break;
                case TOKENS::TOKEN_OP_MINUS:
                    Result.Float = L - R;
                    break;
                case TOKENS::TOKEN_OP_MULTIPLY:
                    Result.Float = L * R;
                    break;
                default:
                    Result.Float = L / R;
                    break;
            }
            Result = SemanticConvert(Context, Result, Common, Site);
        }
        return Result;
    }
    if (!Left.IntegerKnown || !Right.IntegerKnown)
    {
        return Result;
    }
    UINT16 Bits = Common->Bits;
    UINT64 Mask = SemanticMask(Bits);
    UINT64 L    = Left.Integer & Mask;
    UINT64 R    = Right.Integer & Mask;
    INT128 LS   = Common->Unsigned ? static_cast<INT128>(L) : static_cast<INT128>(SemanticSigned(L, Bits));
    INT128 RS   = Common->Unsigned ? static_cast<INT128>(R) : static_cast<INT128>(SemanticSigned(R, Bits));
    if ((Operator == TOKENS::TOKEN_OP_DIVIDE || Operator == TOKENS::TOKEN_OP_MODULO) && R == 0)
    {
        if (!Context->Unreachable)
        {
            SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::DIVISION_BY_ZERO);
        }
        return Result;
    }
    if (Compare)
    {
        Result.IntegerKnown = TRUE;
        switch (Operator)
        {
            case TOKENS::TOKEN_OP_EQUAL:
                Result.Integer = LS == RS;
                break;
            case TOKENS::TOKEN_OP_NOT_EQUAL:
                Result.Integer = LS != RS;
                break;
            case TOKENS::TOKEN_OP_LESS:
                Result.Integer = LS < RS;
                break;
            case TOKENS::TOKEN_OP_LESS_EQUAL:
                Result.Integer = LS <= RS;
                break;
            case TOKENS::TOKEN_OP_GREATER:
                Result.Integer = LS > RS;
                break;
            default:
                Result.Integer = LS >= RS;
                break;
        }
        return Result;
    }
    BOOLEAN Overflow = FALSE;
    UINT64  Value    = 0;
    switch (Operator)
    {
        case TOKENS::TOKEN_OP_PLUS:
        case TOKENS::TOKEN_OP_MINUS:
        case TOKENS::TOKEN_OP_MULTIPLY:
        {
            if (Common->Unsigned)
            {
                UINT128 Wide = Operator == TOKENS::TOKEN_OP_PLUS ? static_cast<UINT128>(L) + R : Operator == TOKENS::TOKEN_OP_MULTIPLY ? static_cast<UINT128>(L) * R : static_cast<UINT128>(L) - R;
                Overflow     = Wide > Mask;
                Value        = static_cast<UINT64>(Wide) & Mask;
            }
            else
            {
                INT128 Wide    = Operator == TOKENS::TOKEN_OP_PLUS ? LS + RS : Operator == TOKENS::TOKEN_OP_MULTIPLY ? LS * RS : LS - RS;
                INT128 Minimum = -(INT128(1) << (Bits - 1));
                INT128 Maximum = (INT128(1) << (Bits - 1)) - 1;
                Overflow       = Wide < Minimum || Wide > Maximum;
                Value          = static_cast<UINT64>(Wide) & Mask;
            }
            break;
        }
        case TOKENS::TOKEN_OP_DIVIDE:
            Value    = static_cast<UINT64>(LS / RS) & Mask;
            Overflow = !Common->Unsigned && LS == -(INT128(1) << (Bits - 1)) && RS == -1;
            break;
        case TOKENS::TOKEN_OP_MODULO:
            Value = static_cast<UINT64>(LS % RS) & Mask;
            break;
        case TOKENS::TOKEN_OP_BIT_AND:
            Value = L & R;
            break;
        case TOKENS::TOKEN_OP_BIT_OR:
            Value = L | R;
            break;
        case TOKENS::TOKEN_OP_BIT_XOR:
            Value = L ^ R;
            break;
        default:
        {
            if (RS < 0 || RS >= Bits)
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_SHIFT, std::to_string(static_cast<INT64>(RS)), std::to_string(Bits));
                return Result;
            }
            UINT16 Count = static_cast<UINT16>(RS);
            if (Operator == TOKENS::TOKEN_OP_SHL)
            {
                Value = (L << Count) & Mask;
            }
            else if (Operator == TOKENS::TOKEN_OP_SHR)
            {
                Value = Common->Unsigned ? L >> Count : static_cast<UINT64>(SemanticSigned(L, Bits) >> Count) & Mask;
            }
            else if (Count == 0)
            {
                Value = L;
            }
            else if (Operator == TOKENS::TOKEN_OP_ROL)
            {
                Value = ((L << Count) | (L >> (Bits - Count))) & Mask;
            }
            else
            {
                Value = ((L >> Count) | (L << (Bits - Count))) & Mask;
            }
            break;
        }
    }
    if (Overflow && !Context->Unreachable)
    {
        STRING After = Common->Unsigned ? std::to_string(Value) : std::to_string(SemanticSigned(Value, Bits));
        SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::INTEGER_WRAP, "constant expression", SemanticName(Common), After);
    }
    Result.IntegerKnown = TRUE;
    Result.Integer      = Value;
    return Result;
}

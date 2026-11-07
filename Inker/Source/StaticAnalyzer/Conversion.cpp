// Inker/Source/StaticAnalyzer/Conversion.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>
#include <cmath>
#include <limits>
#include <algorithm>

STATIC VOID SemanticRemember(SEMANTIC_CONTEXT *Context, AST_NODE *Node, CONST SEMANTIC_VALUE &Value)
{
    if (Node == NULLPTR)
    {
        return;
    }
    if (Context->Variant != NULLPTR)
    {
        Context->Variant->Conversions[Node] = Value;
    }
    else
    {
        Context->Analyzer->Conversions[Node] = Value;
    }
}

STATIC BOOLEAN SemanticUTF16(STRING_VIEW Text, std::vector<UINT16> *Result)
{
    for (SIZE Index = 0; Index < Text.size();)
    {
        UINT8  First   = static_cast<UINT8>(Text[Index++]);
        UINT32 Value   = First;
        SIZE   Count   = 1;
        UINT32 Minimum = 0;
        if (First >= 0xC2 && First <= 0xDF)
        {
            Count   = 2;
            Value   = First & 31;
            Minimum = 0x80;
        }
        else if (First >= 0xE0 && First <= 0xEF)
        {
            Count   = 3;
            Value   = First & 15;
            Minimum = 0x800;
        }
        else if (First >= 0xF0 && First <= 0xF4)
        {
            Count   = 4;
            Value   = First & 7;
            Minimum = 0x10000;
        }
        else if (First >= 0x80)
        {
            return FALSE;
        }
        if (Count - 1 > Text.size() - Index)
        {
            return FALSE;
        }
        for (SIZE Part = 1; Part < Count; Part++)
        {
            UINT8 Byte = static_cast<UINT8>(Text[Index++]);
            if ((Byte & 0xC0) != 0x80)
            {
                return FALSE;
            }
            Value = (Value << 6) | (Byte & 63);
        }
        if (Value < Minimum || Value > 0x10FFFF || (Value >= 0xD800 && Value <= 0xDFFF))
        {
            return FALSE;
        }
        if (Value > 0xFFFF)
        {
            Value -= 0x10000;
            Result->push_back(static_cast<UINT16>(0xD800 + (Value >> 10)));
            Result->push_back(static_cast<UINT16>(0xDC00 + (Value & 1023)));
        }
        else
        {
            Result->push_back(static_cast<UINT16>(Value));
        }
    }
    return TRUE;
}

SEMANTIC_VALUE SemanticConvert(SEMANTIC_CONTEXT *Context, SEMANTIC_VALUE Value, SEMANTIC_TYPE *Expected, AST_NODE *Site)
{
    STATIC_ANALYZER *Analyzer = Context->Analyzer;
    if (Value.Type == NULLPTR)
    {
        Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    if (Expected == NULLPTR || Expected->Kind == SEMANTIC_KIND::ERROR || Value.Type->Kind == SEMANTIC_KIND::ERROR)
    {
        return Value;
    }
    if (Context->DefinitionOnly && Value.Type->Kind == SEMANTIC_KIND::AUTO)
    {
        if (Expected->Kind != SEMANTIC_KIND::AUTO)
        {
            Value.Type = Expected;
        }
        return Value;
    }

    if (Expected->Kind == SEMANTIC_KIND::AUTO)
    {
        if (Value.Type->Kind == SEMANTIC_KIND::FLOAT)
        {
            if (Expected->Bits < 32)
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNSUPPORTED_AUTO_TYPE, SemanticName(Expected), SemanticName(Value.Type));
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                return Value;
            }
            Expected = SemanticType(Analyzer, SEMANTIC_KIND::FLOAT, Expected->Bits);
        }
        else if (Value.Type->Kind == SEMANTIC_KIND::INTEGER)
        {
            Expected = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, Expected->Bits, Value.Type->Unsigned);
        }
        else if (Value.Type->Kind == SEMANTIC_KIND::ARRAY)
        {
            SEMANTIC_TYPE *Element = Value.Type->Element;
            SEMANTIC_VALUE Dummy;
            Dummy.Type = Element;
            Dummy      = SemanticConvert(Context, Dummy, Expected, Site);
            Expected   = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Dummy.Type, Value.Type->Count, Value.Type->Sized);
        }
        else
        {
            Expected = Value.Type;
        }
    }

    if (Expected->Kind == SEMANTIC_KIND::ARRAY && Value.Type->Kind == SEMANTIC_KIND::STRING)
    {
        SEMANTIC_TYPE *Element = Expected->Element;
        if (Element->Kind == SEMANTIC_KIND::AUTO)
        {
            if (Element->Bits != 8 && Element->Bits != 16)
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNSUPPORTED_AUTO_TYPE, SemanticName(Element), "character array (use AUTO8 or AUTO16)");
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                return Value;
            }
            Element = SemanticType(Analyzer, SEMANTIC_KIND::CHARACTER, Element->Bits, TRUE);
        }
        if (Element->Kind == SEMANTIC_KIND::CHARACTER)
        {
            if (!Value.TextKnown)
            {
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Element, Expected->Count, Expected->Sized);
                SemanticRemember(Context, Site, Value);
                return Value;
            }
            std::vector<UINT16> Units;
            if (Element->Bits == 8)
            {
                for (UINT8 Byte : Value.Text)
                {
                    Units.push_back(Byte);
                }
            }
            else if (!SemanticUTF16(Value.Text, &Units))
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_LITERAL, "invalid UTF-8 character array");
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                return Value;
            }
            Units.push_back(0);
            if (Expected->Sized && Units.size() > Expected->Count)
            {
                SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ARRAY_CAPACITY, std::to_string(Units.size()), std::to_string(Expected->Count));
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                return Value;
            }
            SIZE Count = Expected->Sized ? Expected->Count : Units.size();
            if (Count <= 1024 * 1024)
            {
                Units.resize(Count, 0);
            }
            if (Site != NULLPTR)
            {
                Analyzer->CharacterArrays[Site] = std::move(Units);
            }
            Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Element, Count, TRUE);
            SemanticRemember(Context, Site, Value);
            return Value;
        }
    }

    if (Expected->Kind == SEMANTIC_KIND::ARRAY && Value.Type->Kind == SEMANTIC_KIND::ARRAY)
    {
        SEMANTIC_TYPE *Element = Expected->Element;
        if (SemanticHasAuto(Element))
        {
            SEMANTIC_VALUE Dummy;
            Dummy.Type = Value.Type->Element;
            Dummy      = SemanticConvert(Context, Dummy, Element, Site);
            Element    = Dummy.Type;
        }
        SEMANTIC_TYPE *Target = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Element, Expected->Sized ? Expected->Count : Value.Type->Count, Expected->Sized || Value.Type->Sized);
        if (!SemanticCompatible(Target, Value.Type))
        {
            SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, SemanticName(Target), SemanticName(Value.Type));
            Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
        }
        else
        {
            Value.Type = Target;
        }
        SemanticRemember(Context, Site, Value);
        return Value;
    }

    if (SemanticNumeric(Expected) && SemanticNumeric(Value.Type))
    {
        SEMANTIC_TYPE *Original   = Value.Type;
        BOOLEAN        SameFamily = (SemanticIntegral(Original) && SemanticIntegral(Expected)) || (Original->Kind == SEMANTIC_KIND::FLOAT && Expected->Kind == SEMANTIC_KIND::FLOAT);
        if (Original != Expected && !(Value.Literal && SameFamily))
        {
            SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::IMPLICIT_CONVERSION, SemanticName(Original), SemanticName(Expected));
        }
        if (Value.IntegerKnown || Value.FloatKnown)
        {
            long double Number = Value.FloatKnown ? static_cast<long double>(Value.Float) : Original->Unsigned ? static_cast<long double>(Value.Integer & SemanticMask(Original->Bits)) : static_cast<long double>(SemanticSigned(Value.Integer, Original->Bits));
            if (Expected->Kind == SEMANTIC_KIND::FLOAT)
            {
                if (Expected->Bits == 32 && std::abs(Number) > std::numeric_limits<FLOAT32>::max())
                {
                    SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::FLOAT_OVERFLOW, SemanticName(Expected));
                    Value.Float = std::copysign(std::numeric_limits<FLOAT64>::infinity(), static_cast<FLOAT64>(Number));
                }
                else
                {
                    Value.Float = Expected->Bits == 32 ? static_cast<FLOAT64>(static_cast<FLOAT32>(Number)) : static_cast<FLOAT64>(Number);
                }
                Value.FloatKnown   = TRUE;
                Value.IntegerKnown = FALSE;
            }
            else
            {
                if (!std::isfinite(Number))
                {
                    SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_CONVERSION, SemanticName(Original), SemanticName(Expected));
                    Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                    return Value;
                }
                long double Truncated = std::trunc(Number);
                if (Number != Truncated)
                {
                    SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::PRECISION_LOSS, SemanticName(Original), SemanticName(Expected));
                }
                long double Modulus = std::ldexp(static_cast<long double>(1), Expected->Bits);
                long double Minimum = Expected->Unsigned ? 0 : -Modulus / 2;
                long double Maximum = Expected->Unsigned ? Modulus - 1 : Modulus / 2 - 1;
                long double Wrapped = std::fmod(Truncated, Modulus);
                if (Wrapped < 0)
                {
                    Wrapped += Modulus;
                }
                Value.Integer      = static_cast<UINT64>(Wrapped) & SemanticMask(Expected->Bits);
                Value.IntegerKnown = TRUE;
                Value.FloatKnown   = FALSE;
                if (Truncated < Minimum || Truncated > Maximum)
                {
                    STRING Before = Value.Literal && Site != NULLPTR ? STRING(Context->Unit->Lexer.Source.substr(Site->Location.Start, Site->Location.Length)) : std::to_string(static_cast<FLOAT64>(Number));
                    STRING After  = Expected->Unsigned ? std::to_string(Value.Integer) : std::to_string(SemanticSigned(Value.Integer, Expected->Bits));
                    SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::INTEGER_WRAP, Before, SemanticName(Expected), After);
                }
            }
        }
        Value.Type = Expected;
        SemanticRemember(Context, Site, Value);
        return Value;
    }

    if (!SemanticCompatible(Expected, Value.Type))
    {
        SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, SemanticName(Expected), SemanticName(Value.Type));
        Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    else
    {
        Value.Type = Expected;
    }
    return Value;
}

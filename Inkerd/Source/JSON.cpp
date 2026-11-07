// Inkerd/Source/JSON.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <JSON.hpp>
#include <charconv>
#include <stdexcept>

struct JSON_READER
{
    STRING_VIEW Source;
    SIZE        Position = 0;
};

STATIC VOID JSONSkip(JSON_READER *Reader)
{
    while (Reader->Position < Reader->Source.size())
    {
        CHAR8 Character = Reader->Source[Reader->Position];

        if (Character != ' ' && Character != '\t' && Character != '\r' && Character != '\n')
        {
            break;
        }

        Reader->Position++;
    }
}

STATIC CHAR8 JSONTake(JSON_READER *Reader)
{
    if (Reader->Position == Reader->Source.size())
    {
        throw std::invalid_argument("unexpected end of JSON");
    }

    return Reader->Source[Reader->Position++];
}

STATIC VOID JSONExpect(JSON_READER *Reader, CHAR8 Expected)
{
    if (JSONTake(Reader) != Expected)
    {
        throw std::invalid_argument("unexpected JSON character");
    }
}

STATIC UINT32 JSONHex(JSON_READER *Reader)
{
    UINT32 Value = 0;

    for (SIZE Index = 0; Index < 4; Index++)
    {
        CHAR8  Character = JSONTake(Reader);
        UINT32 Digit;

        if (Character >= '0' && Character <= '9')
        {
            Digit = Character - '0';
        }
        else if (Character >= 'a' && Character <= 'f')
        {
            Digit = Character - 'a' + 10;
        }
        else if (Character >= 'A' && Character <= 'F')
        {
            Digit = Character - 'A' + 10;
        }
        else
        {
            throw std::invalid_argument("invalid Unicode escape");
        }

        Value = (Value << 4) | Digit;
    }

    return Value;
}

STATIC VOID JSONUTF8(STRING *Text, UINT32 Value)
{
    if (Value < 0x80)
    {
        Text->push_back(static_cast<CHAR8>(Value));
    }
    else if (Value < 0x800)
    {
        Text->push_back(static_cast<CHAR8>(0xC0 | (Value >> 6)));
        Text->push_back(static_cast<CHAR8>(0x80 | (Value & 63)));
    }
    else if (Value < 0x10000)
    {
        Text->push_back(static_cast<CHAR8>(0xE0 | (Value >> 12)));
        Text->push_back(static_cast<CHAR8>(0x80 | ((Value >> 6) & 63)));
        Text->push_back(static_cast<CHAR8>(0x80 | (Value & 63)));
    }
    else
    {
        Text->push_back(static_cast<CHAR8>(0xF0 | (Value >> 18)));
        Text->push_back(static_cast<CHAR8>(0x80 | ((Value >> 12) & 63)));
        Text->push_back(static_cast<CHAR8>(0x80 | ((Value >> 6) & 63)));
        Text->push_back(static_cast<CHAR8>(0x80 | (Value & 63)));
    }
}

STATIC VOID JSONRawUTF8(JSON_READER *Reader, UINT8 First, STRING *Result)
{
    SIZE   Count;
    UINT32 Value;
    UINT32 Minimum;

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
    else
    {
        throw std::invalid_argument("invalid UTF-8 in JSON");
    }

    Result->push_back(static_cast<CHAR8>(First));

    for (SIZE Index = 1; Index < Count; Index++)
    {
        UINT8 Byte = static_cast<UINT8>(JSONTake(Reader));

        if ((Byte & 0xC0) != 0x80)
        {
            throw std::invalid_argument("invalid UTF-8 continuation");
        }

        Value = (Value << 6) | (Byte & 63);
        Result->push_back(static_cast<CHAR8>(Byte));
    }

    if (Value < Minimum || Value > 0x10FFFF || (Value >= 0xD800 && Value <= 0xDFFF))
    {
        throw std::invalid_argument("invalid UTF-8 code point");
    }
}

STATIC STRING JSONReadString(JSON_READER *Reader)
{
    JSONExpect(Reader, '"');
    STRING Result;

    for (;;)
    {
        CHAR8 Character = JSONTake(Reader);

        if (Character == '"')
        {
            return Result;
        }

        if (static_cast<UINT8>(Character) < 0x20)
        {
            throw std::invalid_argument("control character in JSON string");
        }

        if (static_cast<UINT8>(Character) >= 0x80)
        {
            JSONRawUTF8(Reader, static_cast<UINT8>(Character), &Result);
            continue;
        }

        if (Character != '\\')
        {
            Result.push_back(Character);
            continue;
        }

        Character = JSONTake(Reader);

        switch (Character)
        {
            case '"':
            case '\\':
            case '/':
                Result.push_back(Character);
                break;
            case 'b':
                Result.push_back('\b');
                break;
            case 'f':
                Result.push_back('\f');
                break;
            case 'n':
                Result.push_back('\n');
                break;
            case 'r':
                Result.push_back('\r');
                break;
            case 't':
                Result.push_back('\t');
                break;
            case 'u':
            {
                UINT32 Value = JSONHex(Reader);

                if (Value >= 0xD800 && Value <= 0xDBFF)
                {
                    JSONExpect(Reader, '\\');
                    JSONExpect(Reader, 'u');
                    UINT32 Low = JSONHex(Reader);

                    if (Low < 0xDC00 || Low > 0xDFFF)
                    {
                        throw std::invalid_argument("invalid Unicode surrogate");
                    }

                    Value = 0x10000 + ((Value - 0xD800) << 10) + Low - 0xDC00;
                }
                else if (Value >= 0xDC00 && Value <= 0xDFFF)
                {
                    throw std::invalid_argument("unpaired Unicode surrogate");
                }

                JSONUTF8(&Result, Value);
                break;
            }
            default:
            {
                throw std::invalid_argument("invalid JSON escape");
            }
        }
    }
}

STATIC JSON_VALUE JSONRead(JSON_READER *Reader, SIZE Depth)
{
    if (Depth > 128)
    {
        throw std::invalid_argument("JSON nesting limit");
    }

    JSONSkip(Reader);
    SIZE       Start     = Reader->Position;
    CHAR8      Character = JSONTake(Reader);
    JSON_VALUE Result;

    if (Character == '"')
    {
        Reader->Position--;
        return JSONString(JSONReadString(Reader));
    }

    if (Character == '{' || Character == '[')
    {
        BOOLEAN Object = Character == '{';
        Result.Kind    = Object ? JSON_KIND::OBJECT : JSON_KIND::ARRAY;
        CHAR8 End      = Object ? '}' : ']';
        JSONSkip(Reader);

        if (Reader->Position < Reader->Source.size() && Reader->Source[Reader->Position] == End)
        {
            Reader->Position++;
            return Result;
        }

        for (;;)
        {
            JSONSkip(Reader);
            STRING Key;

            if (Object)
            {
                Key = JSONReadString(Reader);
                JSONSkip(Reader);
                JSONExpect(Reader, ':');
            }

            JSON_VALUE Value = JSONRead(Reader, Depth + 1);

            if (Object)
            {
                Result.Object.insert_or_assign(Key, std::move(Value));
            }
            else
            {
                Result.Array.push_back(std::move(Value));
            }

            JSONSkip(Reader);
            Character = JSONTake(Reader);

            if (Character == End)
            {
                return Result;
            }

            if (Character != ',')
            {
                throw std::invalid_argument("expected JSON separator");
            }
        }
    }

    if (Character == 't' || Character == 'f' || Character == 'n')
    {
        STRING_VIEW Word = Character == 't' ? "true" : Character == 'f' ? "false" : "null";

        if (Reader->Source.substr(Start, Word.size()) != Word)
        {
            throw std::invalid_argument("invalid JSON literal");
        }

        Reader->Position = Start + Word.size();
        return Character == 'n' ? JSON_VALUE{} : JSONBoolean(Character == 't');
    }

    Reader->Position = Start;

    if (Character == '-')
    {
        Reader->Position++;
    }

    Character = JSONTake(Reader);

    if (Character != '0')
    {
        if (Character < '1' || Character > '9')
        {
            throw std::invalid_argument("invalid JSON number");
        }

        while (Reader->Position < Reader->Source.size() && Reader->Source[Reader->Position] >= '0' && Reader->Source[Reader->Position] <= '9')
        {
            Reader->Position++;
        }
    }

    if (Reader->Position < Reader->Source.size() && Reader->Source[Reader->Position] == '.')
    {
        Reader->Position++;
        SIZE Digits = Reader->Position;

        while (Reader->Position < Reader->Source.size() && Reader->Source[Reader->Position] >= '0' && Reader->Source[Reader->Position] <= '9')
        {
            Reader->Position++;
        }

        if (Reader->Position == Digits)
        {
            throw std::invalid_argument("invalid JSON fraction");
        }
    }

    if (Reader->Position < Reader->Source.size() && (Reader->Source[Reader->Position] == 'e' || Reader->Source[Reader->Position] == 'E'))
    {
        Reader->Position++;

        if (Reader->Position < Reader->Source.size() && (Reader->Source[Reader->Position] == '+' || Reader->Source[Reader->Position] == '-'))
        {
            Reader->Position++;
        }

        SIZE Digits = Reader->Position;

        while (Reader->Position < Reader->Source.size() && Reader->Source[Reader->Position] >= '0' && Reader->Source[Reader->Position] <= '9')
        {
            Reader->Position++;
        }

        if (Reader->Position == Digits)
        {
            throw std::invalid_argument("invalid JSON exponent");
        }
    }

    Result.Kind = JSON_KIND::NUMBER;
    Result.Text = Reader->Source.substr(Start, Reader->Position - Start);
    return Result;
}

BOOLEAN JSONParse(STRING_VIEW Source, JSON_VALUE *Value)
{
    try
    {
        JSON_READER Reader{Source};
        JSON_VALUE  Result = JSONRead(&Reader, 0);
        JSONSkip(&Reader);

        if (Reader.Position != Source.size())
        {
            return FALSE;
        }

        *Value = std::move(Result);
        return TRUE;
    }
    catch (CONST std::invalid_argument &)
    {
        return FALSE;
    }
}

STATIC STRING JSONQuote(STRING_VIEW Text)
{
    STRING       Result = "\"";
    CONST CHAR8 *Hex    = "0123456789abcdef";

    for (UINT8 Character : Text)
    {
        if (Character == '"' || Character == '\\')
        {
            Result.push_back('\\');
            Result.push_back(static_cast<CHAR8>(Character));
        }
        else if (Character < 0x20)
        {
            Result += "\\u00";
            Result.push_back(Hex[Character >> 4]);
            Result.push_back(Hex[Character & 15]);
        }
        else
        {
            Result.push_back(static_cast<CHAR8>(Character));
        }
    }

    return Result + '"';
}

STRING JSONSerialize(CONST JSON_VALUE &Value)
{
    switch (Value.Kind)
    {
        case JSON_KIND::NIL:
            return "null";
        case JSON_KIND::BOOLEAN:
            return Value.Boolean ? "true" : "false";
        case JSON_KIND::NUMBER:
            return Value.Text;
        case JSON_KIND::STRING:
            return JSONQuote(Value.Text);
        case JSON_KIND::ARRAY:
        {
            STRING Result = "[";

            for (CONST JSON_VALUE &Item : Value.Array)
            {
                if (Result.size() > 1)
                {
                    Result += ',';
                }

                Result += JSONSerialize(Item);
            }

            return Result + ']';
        }
        case JSON_KIND::OBJECT:
        {
            STRING Result = "{";

            for (CONST auto &[Key, Item] : Value.Object)
            {
                if (Result.size() > 1)
                {
                    Result += ',';
                }

                Result += JSONQuote(Key) + ':' + JSONSerialize(Item);
            }

            return Result + '}';
        }
    }

    return "null";
}

JSON_VALUE JSONObject(std::initializer_list<std::pair<CONST STRING, JSON_VALUE>> Items)
{
    JSON_VALUE Result;
    Result.Kind   = JSON_KIND::OBJECT;
    Result.Object = Items;
    return Result;
}

JSON_VALUE JSONArray(std::vector<JSON_VALUE> Items)
{
    JSON_VALUE Result;
    Result.Kind  = JSON_KIND::ARRAY;
    Result.Array = std::move(Items);
    return Result;
}

JSON_VALUE JSONString(STRING_VIEW Text)
{
    JSON_VALUE Result;
    Result.Kind = JSON_KIND::STRING;
    Result.Text = Text;
    return Result;
}

JSON_VALUE JSONNumber(INT64 Value)
{
    JSON_VALUE Result;
    Result.Kind = JSON_KIND::NUMBER;
    Result.Text = std::to_string(Value);
    return Result;
}

JSON_VALUE JSONBoolean(BOOLEAN Value)
{
    JSON_VALUE Result;
    Result.Kind    = JSON_KIND::BOOLEAN;
    Result.Boolean = Value;
    return Result;
}

CONST JSON_VALUE &JSONGet(CONST JSON_VALUE &Value, STRING_VIEW Key)
{
    STATIC CONST JSON_VALUE Empty;
    auto                    It = Value.Object.find(STRING(Key));
    return It == Value.Object.end() ? Empty : It->second;
}

BOOLEAN JSONInteger(CONST JSON_VALUE &Value, INT64 *Result)
{
    if (Value.Kind != JSON_KIND::NUMBER)
    {
        return FALSE;
    }

    auto Parsed = std::from_chars(Value.Text.data(), Value.Text.data() + Value.Text.size(), *Result);
    return Parsed.ec == std::errc{} && Parsed.ptr == Value.Text.data() + Value.Text.size();
}

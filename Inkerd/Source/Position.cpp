// Inkerd/Source/Position.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Server.hpp>
#include <algorithm>
#include <filesystem>

STATIC INT32 InkerdHex(CHAR8 Character)
{
    if (Character >= '0' && Character <= '9')
    {
        return Character - '0';
    }

    if (Character >= 'a' && Character <= 'f')
    {
        return Character - 'a' + 10;
    }

    if (Character >= 'A' && Character <= 'F')
    {
        return Character - 'A' + 10;
    }

    return -1;
}

STRING InkerdURIPath(STRING_VIEW URI)
{
    if (!URI.starts_with("file://"))
    {
        return {};
    }

    URI.remove_prefix(7);

    if (URI.starts_with("localhost/"))
    {
        URI.remove_prefix(9);
    }

    if (URI.empty() || URI.front() != '/' || URI.find_first_of("?#") != STRING_VIEW::npos)
    {
        return {};
    }

    STRING Path;

    for (SIZE Index = 0; Index < URI.size(); Index++)
    {
        CHAR8 Character = URI[Index];

        if (Character == '%')
        {
            if (Index + 2 >= URI.size())
            {
                return {};
            }

            INT32 High = InkerdHex(URI[Index + 1]);
            INT32 Low  = InkerdHex(URI[Index + 2]);

            if (High < 0 || Low < 0)
            {
                return {};
            }

            Character = static_cast<CHAR8>((High << 4) | Low);
            Index += 2;
        }

        if (Character == '\0')
        {
            return {};
        }

        Path.push_back(Character);
    }

    return std::filesystem::path(Path).lexically_normal().string();
}

STRING InkerdPathURI(STRING_VIEW Path)
{
    STRING       Result = "file://";
    CONST CHAR8 *Hex    = "0123456789ABCDEF";

    for (UINT8 Character : Path)
    {
        if ((Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || (Character >= '0' && Character <= '9') || Character == '/' || Character == '-' || Character == '_' || Character == '.' || Character == '~')
        {
            Result.push_back(static_cast<CHAR8>(Character));
        }
        else
        {
            Result.push_back('%');
            Result.push_back(Hex[Character >> 4]);
            Result.push_back(Hex[Character & 15]);
        }
    }

    return Result;
}

JSON_VALUE InkerdPosition(STRING_VIEW Source, SIZE Offset)
{
    Offset         = std::min(Offset, Source.size());
    SIZE Line      = 0;
    SIZE Character = 0;

    for (SIZE Index = 0; Index < Offset;)
    {
        UINT8 First = static_cast<UINT8>(Source[Index]);

        if (First == '\n')
        {
            Line++;
            Character = 0;
            Index++;
            continue;
        }

        SIZE    Width = First < 0x80 ? 1 : First >= 0xC2 && First <= 0xDF ? 2 : First >= 0xE0 && First <= 0xEF ? 3 : First >= 0xF0 && First <= 0xF4 ? 4 : 1;
        BOOLEAN Valid = Width <= Offset - Index;

        for (SIZE Part = 1; Part < Width && Valid; Part++)
        {
            Valid = (static_cast<UINT8>(Source[Index + Part]) & 0xC0) == 0x80;
        }

        if (!Valid)
        {
            Width = 1;
        }

        Character += Width == 4 ? 2 : 1;
        Index += Width;
    }

    return JSONObject({{"line", JSONNumber(Line)}, {"character", JSONNumber(Character)}});
}

// Inker/Source/Diagnostics/Diagnostics.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Diagnostics/Diagnostics.hpp>
#include <cstdio>

STATIC UINT32 DiagnosticDecodeUTF8(CONST STRING &Source, SIZE Position, SIZE End, SIZE *Length)
{
    UINT8 First = (UINT8) Source[Position];

    if (First < 0x80)
    {
        *Length = 1;
        return First;
    }

    UINT32 Codepoint;
    SIZE   Count;
    UINT32 Minimum;

    if (First >= 0xC2 && First <= 0xDF)
    {
        Codepoint = First & 0x1F;
        Count     = 2;
        Minimum   = 0x80;
    }
    else if (First >= 0xE0 && First <= 0xEF)
    {
        Codepoint = First & 0x0F;
        Count     = 3;
        Minimum   = 0x800;
    }
    else if (First >= 0xF0 && First <= 0xF4)
    {
        Codepoint = First & 0x07;
        Count     = 4;
        Minimum   = 0x10000;
    }
    else
    {
        *Length = 1;
        return 0xFFFD;
    }

    if (Count > End - Position)
    {
        *Length = 1;
        return 0xFFFD;
    }

    for (SIZE Index = 1; Index < Count; Index++)
    {
        UINT8 Next = (UINT8) Source[Position + Index];

        if ((Next & 0xC0) != 0x80)
        {
            *Length = 1;
            return 0xFFFD;
        }

        Codepoint = (Codepoint << 6) | (Next & 0x3F);
    }

    if (Codepoint < Minimum || Codepoint > 0x10FFFF || (Codepoint >= 0xD800 && Codepoint <= 0xDFFF))
    {
        *Length = 1;
        return 0xFFFD;
    }

    *Length = Count;
    return Codepoint;
}

STATIC BOOLEAN DiagnosticIsWide(UINT32 Codepoint)
{
    return (Codepoint >= 0x1100 && Codepoint <= 0x115F) || (Codepoint >= 0x2329 && Codepoint <= 0x232A) || (Codepoint >= 0x2E80 && Codepoint <= 0xA4CF) || (Codepoint >= 0xAC00 && Codepoint <= 0xD7A3) || (Codepoint >= 0xF900 && Codepoint <= 0xFAFF) || (Codepoint >= 0xFE10 && Codepoint <= 0xFE19) || (Codepoint >= 0xFE30 && Codepoint <= 0xFE6F) || (Codepoint >= 0xFF01 && Codepoint <= 0xFF60) || (Codepoint >= 0xFFE0 && Codepoint <= 0xFFE6) || (Codepoint >= 0x1F300 && Codepoint <= 0x1FAFF) || (Codepoint >= 0x20000 && Codepoint <= 0x3FFFD);
}

STATIC BOOLEAN DiagnosticIsCombining(UINT32 Codepoint)
{
    return (Codepoint >= 0x0300 && Codepoint <= 0x036F) || (Codepoint >= 0x1AB0 && Codepoint <= 0x1AFF) || (Codepoint >= 0x1DC0 && Codepoint <= 0x1DFF) || (Codepoint >= 0x20D0 && Codepoint <= 0x20FF) || (Codepoint >= 0xFE20 && Codepoint <= 0xFE2F) || (Codepoint >= 0xFE00 && Codepoint <= 0xFE0F) || Codepoint == 0x200D;
}

STATIC SIZE DiagnosticCharacterWidth(UINT32 Codepoint, SIZE Column)
{
    if (Codepoint == '\t')
    {
        return 4 - (Column % 4);
    }

    if (DiagnosticIsCombining(Codepoint))
    {
        return 0;
    }

    if (DiagnosticIsWide(Codepoint))
    {
        return 2;
    }

    if (Codepoint < 0x20 || (Codepoint >= 0x7F && Codepoint < 0xA0))
    {
        return 0;
    }

    return 1;
}

STATIC SIZE DiagnosticDisplayWidth(CONST STRING &Source, SIZE Start, SIZE End)
{
    SIZE Width = 0;

    while (Start < End)
    {
        SIZE   Length;
        UINT32 Codepoint = DiagnosticDecodeUTF8(Source, Start, End, &Length);

        Width += DiagnosticCharacterWidth(Codepoint, Width);
        Start += Length;
    }

    return Width;
}

STATIC STRING DiagnosticExpandTabs(CONST STRING &Source, SIZE Start, SIZE End)
{
    STRING Result;
    SIZE   Column = 0;

    while (Start < End)
    {
        SIZE   Length;
        UINT32 Codepoint = DiagnosticDecodeUTF8(Source, Start, End, &Length);
        SIZE   Width     = DiagnosticCharacterWidth(Codepoint, Column);

        if (Codepoint == '\t')
        {
            Result.append(Width, ' ');
        }
        else
        {
            Result.append(Source, Start, Length);
        }

        Column += Width;
        Start += Length;
    }

    return Result;
}

STATIC CONST CHAR8 *DiagnosticLevelString(DIAGNOSTIC_LEVEL Level)
{
    switch (Level)
    {
        case DIAGNOSTIC_LEVEL::ERROR:
        {
            return "error";
        }

        case DIAGNOSTIC_LEVEL::WARNING:
        {
            return "warning";
        }

        case DIAGNOSTIC_LEVEL::NOTE:
        {
            return "note";
        }

        default:
        {
            return "unknown";
        }
    }
}

STATIC CONST CHAR8 *DiagnosticLevelColor(DIAGNOSTIC_LEVEL Level)
{
    switch (Level)
    {
        case DIAGNOSTIC_LEVEL::ERROR:
        {
            return "\033[1;31m";
        }

        case DIAGNOSTIC_LEVEL::WARNING:
        {
            return "\033[1;33m";
        }

        case DIAGNOSTIC_LEVEL::NOTE:
        {
            return "\033[1;36m";
        }

        default:
        {
            return "\033[0m";
        }
    }
}

VOID DiagnosticPrint(CONST DIAGNOSTIC *Diagnostic)
{
    if (Diagnostic == NULLPTR || Diagnostic->Source == NULLPTR)
    {
        return;
    }

    CONST STRING &Source = *Diagnostic->Source;

    SIZE Start = Diagnostic->Start;

    if (Start > Source.size())
    {
        Start = Source.size();
    }

    SIZE Line      = 1;
    SIZE LineStart = 0;

    for (SIZE Index = 0; Index < Start; Index++)
    {
        if (Source[Index] == '\n')
        {
            Line++;
            LineStart = Index + 1;
        }
    }

    SIZE LineEnd = Source.find('\n', LineStart);

    if (LineEnd == STRING::npos)
    {
        LineEnd = Source.size();
    }

    if (LineEnd > LineStart && Source[LineEnd - 1] == '\r')
    {
        LineEnd--;
    }

    SIZE HighlightStart = Start;

    if (HighlightStart > LineEnd)
    {
        HighlightStart = LineEnd;
    }

    SIZE HighlightEnd = Start + Diagnostic->Length;

    if (HighlightEnd < Start || HighlightEnd > LineEnd)
    {
        HighlightEnd = LineEnd;
    }

    SIZE Column       = DiagnosticDisplayWidth(Source, LineStart, HighlightStart) + 1;
    SIZE MarkerOffset = Column - 1;
    SIZE MarkerLength = DiagnosticDisplayWidth(Source, HighlightStart, HighlightEnd);

    if (MarkerLength == 0)
    {
        MarkerLength = 1;
    }

    STRING Text = DiagnosticExpandTabs(Source, LineStart, LineEnd);

    CONST CHAR8 *Color = DiagnosticLevelColor(Diagnostic->Level);
    CONST CHAR8 *Level = DiagnosticLevelString(Diagnostic->Level);

    SIZE LineDigits = 1;

    for (SIZE Value = Line; Value >= 10; Value /= 10)
    {
        LineDigits++;
    }

    SIZE GutterWidth = LineDigits < 4 ? 4 : LineDigits;

    std::fprintf(stderr, "%s:%zu:%zu: %s%s\033[0m: %s\n", Diagnostic->Path.c_str(), Line, Column, Color, Level, Diagnostic->Message.c_str());
    std::fprintf(stderr, " %*zu | %s\n", (INT32) GutterWidth, Line, Text.c_str());
    std::fprintf(stderr, " %*s | %*s%s^", (INT32) GutterWidth, "", (INT32) MarkerOffset, "", Color);

    for (SIZE Index = 1; Index < MarkerLength; Index++)
    {
        std::fputc('~', stderr);
    }

    std::fprintf(stderr, "\033[0m\n");

    if (!Diagnostic->FixIt.empty())
    {
        std::fprintf(stderr, " %*s | %*s\033[1;32m%s\033[0m\n", (INT32) GutterWidth, "", (INT32) MarkerOffset, "", Diagnostic->FixIt.c_str());
    }
}

VOID DiagnosticPrintSummary(SIZE ErrorCount, SIZE WarningCount)
{
    if (ErrorCount == 0 && WarningCount == 0)
    {
        return;
    }

    std::fprintf(stderr, "%zu errors, %zu warnings generated.\n", ErrorCount, WarningCount);
}
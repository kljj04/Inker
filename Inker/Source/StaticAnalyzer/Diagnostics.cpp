// Inker/Source/StaticAnalyzer/Diagnostics.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>
#include <algorithm>

AST_LOC AnalyzerLocation(CONST ANALYZER_UNIT *Unit, CONST AST_NODE *Node, STRING_VIEW Name)
{
    AST_LOC       Location = Node == NULLPTR ? AST_LOC{} : Node->Location;
    SIZE          Start    = Location.Start;
    SIZE          Length   = Location.Length;
    CONST STRING &Source   = Unit->Lexer.Source;

    if (!Name.empty())
    {
        UINTN Begin   = reinterpret_cast<UINTN>(Source.data());
        UINTN Address = reinterpret_cast<UINTN>(Name.data());

        if (Address >= Begin && Address - Begin <= Source.size() && Name.size() <= Source.size() - (Address - Begin))
        {
            Start  = Address - Begin;
            Length = Name.size();
        }
        else
        {
            SIZE Found = Source.find(Name, Start);

            if (Found != STRING::npos && Found <= Source.size() && Found + Name.size() <= Location.Start + Location.Length)
            {
                Start  = Found;
                Length = Name.size();
            }
        }
    }

    Start          = std::min(Start, Source.size());
    Length         = std::min(Length, Source.size() - Start);
    auto It        = std::upper_bound(Unit->LineStarts.begin(), Unit->LineStarts.end(), Start);
    SIZE Line      = It == Unit->LineStarts.begin() ? 0 : static_cast<SIZE>(It - Unit->LineStarts.begin() - 1);
    SIZE LineStart = Unit->LineStarts.empty() ? 0 : Unit->LineStarts[Line];
    return {Start, Length, Line + 1, Start - LineStart + 1};
}

VOID AnalyzerReport(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, AST_LOC Location, DIAGNOSTIC_LEVEL Level, DIAGNOSTIC_MESSAGE Message, STRING_VIEW First, STRING_VIEW Second)
{
    DIAGNOSTIC Diagnostic{};
    Diagnostic.Path    = Unit->Path;
    Diagnostic.Source  = &Unit->Lexer.Source;
    Diagnostic.Start   = Location.Start;
    Diagnostic.Length  = Location.Length;
    Diagnostic.Level   = Level;
    Diagnostic.Message = DiagnosticFormatMessage(Message, {First, Second});
    Analyzer->Diagnostics.push_back(std::move(Diagnostic));
}

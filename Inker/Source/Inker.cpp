// Inker/Source/Inker.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Diagnostics/Diagnostics.hpp>
#include <Diagnostics/Messages.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/Parser.hpp>
#include <StaticAnalyzer/StaticAnalyzer.hpp>
#include <cstdio>
#include <fstream>
#include <iterator>

INT32 main(INT32 Argc, CHAR8 *Argv[])
{
    if (Argc < 2)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m no input files\n");
        DiagnosticPrintSummary(1, 0);
        return 1;
    }

    if (Argc > 2)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m too many input files\n");
        DiagnosticPrintSummary(1, 0);
        return 1;
    }

    std::ifstream File(Argv[1], std::ios::binary);

    if (!File)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m cannot open file '%s'\n", Argv[1]);
        DiagnosticPrintSummary(1, 0);
        return 1;
    }

    STRING Source((std::istreambuf_iterator<CHAR8>(File)), std::istreambuf_iterator<CHAR8>());

    if (File.bad())
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m failed to read file '%s'\n", Argv[1]);
        DiagnosticPrintSummary(1, 0);
        return 1;
    }

    STATIC_ANALYZER Analyzer;
    Analyzer.Options                = AnalyzerDefaultOptions(Argv[0]);
    STRING Path                     = AnalyzerCanonicalPath(Argv[1]);
    Analyzer.Options.Overlays[Path] = std::move(Source);
    ANALYZER_UNIT *Entry            = StaticAnalyzerAnalyze(&Analyzer, Path);
    SIZE           ErrorCount       = 0;
    SIZE           WarningCount     = 0;

    for (CONST DIAGNOSTIC &Diagnostic : Analyzer.Diagnostics)
    {
        DiagnosticPrint(&Diagnostic);

        if (Diagnostic.Level == DIAGNOSTIC_LEVEL::ERROR)
        {
            ErrorCount++;
        }
        else if (Diagnostic.Level == DIAGNOSTIC_LEVEL::WARNING)
        {
            WarningCount++;
        }
    }

    if (Entry == NULLPTR && ErrorCount == 0)
    {
        std::fprintf(stderr, "inker: error: failed to load input file\n");
        ErrorCount++;
    }

    DiagnosticPrintSummary(ErrorCount, WarningCount);
    return ErrorCount > 0 ? 1 : 0;
}

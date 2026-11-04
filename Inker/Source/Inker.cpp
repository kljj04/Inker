// Inker/Source/Inker.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Diagnostics/Diagnostics.hpp>
#include <Diagnostics/Messages.hpp>
#include <Lexer/Lexer.hpp>
#include <Parser/Parser.hpp>
#include <cstdio>
#include <fstream>
#include <iterator>

INT32 main(INT32 Argc, CHAR8 *Argv[])
{
    if (Argc < 2)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m no input files\n");
        return 1;
    }

    if (Argc > 2)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m too many input files\n");
        return 1;
    }

    std::ifstream File(Argv[1], std::ios::binary);

    if (!File)
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m cannot open file '%s'\n", Argv[1]);
        return 1;
    }

    STRING Source((std::istreambuf_iterator<CHAR8>(File)), std::istreambuf_iterator<CHAR8>());

    if (File.bad())
    {
        std::fprintf(stderr, "inker: \033[91merror:\033[0m failed to read file '%s'\n", Argv[1]);
        return 1;
    }

    LEXER Lexer;
    LexerInit(&Lexer, Source);

    PARSER Parser;
    ParserInit(&Parser, &Lexer);

    AST_NODE *Root = ParserParse(&Parser);

    if (Parser.ErrorMessage != DIAGNOSTIC_MESSAGE::NONE || Root == NULLPTR)
    {
        DIAGNOSTIC Diagnostic = {.Path = Argv[1], .Source = &Source, .Level = DIAGNOSTIC_LEVEL::ERROR};

        if (Lexer.Error != LEXER_ERROR::NONE)
        {
            Diagnostic.Start   = Lexer.TokenStart;
            Diagnostic.Length  = Lexer.TokenLength;
            Diagnostic.Message = DiagnosticLexerMessage(Lexer.Error);
        }
        else
        {
            Diagnostic.Start   = Parser.ErrorStart;
            Diagnostic.Length  = Parser.ErrorLength;
            Diagnostic.Message = DiagnosticMessageString(Parser.ErrorMessage);
        }

        DiagnosticPrint(&Diagnostic);
        return 1;
    }
    return 0;
}
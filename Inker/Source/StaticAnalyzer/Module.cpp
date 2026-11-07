// Inker/Source/StaticAnalyzer/Module.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>
#include <fstream>
#include <iterator>
#include <cstdlib>

STRING AnalyzerCanonicalPath(CONST std::filesystem::path &Path)
{
    std::error_code Error;
    auto            Absolute = std::filesystem::absolute(Path, Error);

    if (Error)
    {
        return Path.lexically_normal().string();
    }

    auto Canonical = std::filesystem::weakly_canonical(Absolute, Error);
    return (Error ? Absolute.lexically_normal() : Canonical).string();
}

STRING AnalyzerNormalizePath(STRING_VIEW Path)
{
    LEXER Lexer;
    LexerInit(&Lexer, STRING(Path));
    STRING Result;

    for (;;)
    {
        TOKENS Token = LexerNextToken(&Lexer);

        if (Token == TOKENS::TOKEN_EOF || Token == TOKENS::TOKEN_UNKNOWN)
        {
            break;
        }

        Result.append(Lexer.Source, Lexer.TokenStart, Lexer.TokenLength);
    }

    return Result;
}

ANALYZER_OPTIONS AnalyzerDefaultOptions(CONST STRING &Executable)
{
    ANALYZER_OPTIONS Options;
    CONST CHAR8     *Environment = std::getenv("INKER_STDLIB");

    if (Environment != NULLPTR && *Environment != '\0')
    {
        Options.StdlibRoot = AnalyzerCanonicalPath(Environment);
    }
    else
    {
        auto            Directory   = std::filesystem::path(AnalyzerCanonicalPath(Executable)).parent_path();
        auto            Development = Directory.parent_path() / "Std";
        std::error_code Error;
        Options.StdlibRoot = AnalyzerCanonicalPath(std::filesystem::is_directory(Development, Error) ? Development : Directory / ".." / "lib" / "ink");
    }

    return Options;
}

STATIC STRING AnalyzerImportFile(STATIC_ANALYZER *Analyzer, ANALYZER_UNIT *Unit, AST_NODE *Node)
{
    CONST AST_USE_DECLARATION &Use = Node->UseDeclaration;

    std::filesystem::path File      = STRING(Use.File);
    auto                  Directory = std::filesystem::path(Unit->Path).parent_path();

    if (File.is_absolute() || Use.File.starts_with("./") || Use.File.starts_with("../"))
    {
        return AnalyzerCanonicalPath(Directory / File);
    }

    std::vector<STRING> Roots{Directory.string()};
    Roots.insert(Roots.end(), Analyzer->Options.ModuleRoots.begin(), Analyzer->Options.ModuleRoots.end());

    if (!Analyzer->Options.StdlibRoot.empty())
    {
        Roots.push_back(Analyzer->Options.StdlibRoot);
    }

    for (CONST STRING &Root : Roots)
    {
        STRING          Candidate = AnalyzerCanonicalPath(std::filesystem::path(Root) / File);
        std::error_code Error;

        if (Analyzer->Options.Overlays.contains(Candidate) || std::filesystem::is_regular_file(Candidate, Error))
        {
            return Candidate;
        }
    }

    return AnalyzerCanonicalPath(Directory / File);
}

ANALYZER_UNIT *AnalyzerLoad(STATIC_ANALYZER *Analyzer, CONST STRING &Path, ANALYZER_UNIT *Importer, AST_NODE *Import)
{
    STRING Canonical = AnalyzerCanonicalPath(Path);
    auto   Existing  = Analyzer->Loaded.find(Canonical);

    if (Existing != Analyzer->Loaded.end())
    {
        if (Existing->second->Loading && Importer != NULLPTR)
        {
            STRING Chain;

            for (CONST STRING &Item : Analyzer->LoadStack)
            {
                Chain += Item + " -> ";
            }

            Chain += Canonical;
            AnalyzerReport(Analyzer, Importer, Import->Location, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::CIRCULAR_IMPORT, Chain);
        }

        return Existing->second;
    }

    STRING Source;
    auto   Overlay = Analyzer->Options.Overlays.find(Canonical);

    if (Overlay != Analyzer->Options.Overlays.end())
    {
        Source = Overlay->second;
    }
    else
    {
        std::ifstream File(Canonical, std::ios::binary);

        if (!File)
        {
            if (Importer != NULLPTR)
            {
                AnalyzerReport(Analyzer, Importer, Import->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::MODULE_NOT_FOUND, Canonical);
            }

            return NULLPTR;
        }

        Source.assign(std::istreambuf_iterator<CHAR8>(File), std::istreambuf_iterator<CHAR8>());

        if (File.bad())
        {
            if (Importer != NULLPTR)
            {
                AnalyzerReport(Analyzer, Importer, Import->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::MODULE_READ_FAILED, Canonical);
            }

            return NULLPTR;
        }
    }

    if (Analyzer->LoadStack.size() >= 128)
    {
        if (Importer != NULLPTR)
        {
            AnalyzerReport(Analyzer, Importer, Import->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        }

        return NULLPTR;
    }

    auto           Storage = std::make_unique<ANALYZER_UNIT>();
    ANALYZER_UNIT *Unit    = Storage.get();
    Unit->Path             = Canonical;
    LexerInit(&Unit->Lexer, Source);
    Unit->LineStarts.push_back(0);

    for (SIZE Index = 0; Index < Source.size(); Index++)
    {
        if (Source[Index] == '\n')
        {
            Unit->LineStarts.push_back(Index + 1);
        }
    }

    ParserInit(&Unit->Parser, &Unit->Lexer);
    Unit->Root  = ParserParse(&Unit->Parser);
    Unit->Valid = Unit->Root != NULLPTR;
    Analyzer->Loaded.emplace(Canonical, Unit);
    Analyzer->Units.push_back(std::move(Storage));
    Unit->Global = AnalyzerNewScope(Analyzer, Unit);
    Analyzer->Symbols.emplace_back();
    Unit->Module          = &Analyzer->Symbols.back();
    Unit->Module->Kind    = ANALYZER_SYMBOL_KIND::NAMESPACE;
    Unit->Module->Name    = Canonical;
    Unit->Module->Unit    = Unit;
    Unit->Module->Members = Unit->Global;
    Unit->Module->Access  = TOKENS::TOKEN_KW_PUBLIC;
    Analyzer->LoadStack.push_back(Canonical);

    if (Unit->Lexer.Error != LEXER_ERROR::NONE)
    {
        DIAGNOSTIC Diagnostic{};
        Diagnostic.Path    = Canonical;
        Diagnostic.Source  = &Unit->Lexer.Source;
        Diagnostic.Start   = Unit->Lexer.TokenStart;
        Diagnostic.Length  = Unit->Lexer.TokenLength;
        Diagnostic.Level   = DIAGNOSTIC_LEVEL::ERROR;
        Diagnostic.Message = DiagnosticLexerMessage(Unit->Lexer.Error);
        Analyzer->Diagnostics.push_back(std::move(Diagnostic));
    }
    else
    {
        for (CONST PARSER_DIAGNOSTIC &Entry : Unit->Parser.Diagnostics)
        {
            AnalyzerReport(Analyzer, Unit, Entry.Location, Entry.Level, Entry.Message, Entry.Argument);
            Analyzer->Diagnostics.back().FixIt = Entry.FixIt;
        }

        if (Unit->Parser.ErrorMessage != DIAGNOSTIC_MESSAGE::NONE && Unit->Parser.Diagnostics.empty())
        {
            AnalyzerReport(Analyzer, Unit, {Unit->Parser.ErrorStart, Unit->Parser.ErrorLength, 1, 1}, DIAGNOSTIC_LEVEL::ERROR, Unit->Parser.ErrorMessage, Unit->Parser.ErrorArgument);
        }
    }

    if (Unit->Valid)
    {
        AnalyzerWalk(Unit->Root,
                     [&](AST_NODE *Node)
                     {
                         if (Node->Type == AST_NODE_TYPE::USE_DECLARATION)
                         {
                             Unit->Imports.push_back({Node, AnalyzerLoad(Analyzer, AnalyzerImportFile(Analyzer, Unit, Node), Unit, Node)});
                         }
                     });
    }

    Analyzer->LoadStack.pop_back();
    Unit->Loading = FALSE;
    return Unit;
}

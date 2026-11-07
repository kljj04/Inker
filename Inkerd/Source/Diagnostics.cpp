// Inkerd/Source/Diagnostics.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Server.hpp>
#include <StaticAnalyzer/Semantics.hpp>
#include <algorithm>

STATIC JSON_VALUE InkerdDiagnostic(CONST DIAGNOSTIC &Diagnostic)
{
    CONST STRING &Source   = *Diagnostic.Source;
    SIZE          Start    = std::min(Diagnostic.Start, Source.size());
    SIZE          End      = Start + std::min(Diagnostic.Length, Source.size() - Start);
    INT64         Severity = Diagnostic.Level == DIAGNOSTIC_LEVEL::ERROR ? 1 : Diagnostic.Level == DIAGNOSTIC_LEVEL::WARNING ? 2 : 3;
    JSON_VALUE    Result   = JSONObject({{"range", JSONObject({{"start", InkerdPosition(Source, Start)}, {"end", InkerdPosition(Source, End)}})}, {"severity", JSONNumber(Severity)}, {"source", JSONString("inker")}, {"message", JSONString(Diagnostic.Message)}});

    if (!Diagnostic.FixIt.empty())
    {
        Result.Object["data"] = JSONObject({{"fixIt", JSONString(Diagnostic.FixIt)}});
    }

    return Result;
}

VOID InkerdAnalyzeAll(INKERD_SERVER *Server)
{
    STATIC_ANALYZER Analyzer;
    Analyzer.Options = Server->Options;

    for (CONST auto &[URI, Document] : Server->Documents)
    {
        Analyzer.Options.Overlays[Document.Path] = Document.Text;
    }

    for (CONST auto &[URI, Document] : Server->Documents)
    {
        AnalyzerLoad(&Analyzer, Document.Path);
    }

    AnalyzerCollect(&Analyzer);
    AnalyzerBindImports(&Analyzer);
    AnalyzerResolveNames(&Analyzer);
    SemanticCheck(&Analyzer);
    std::map<STRING, std::vector<JSON_VALUE>> Results;
    std::map<STRING, STRING>                  URIs;

    for (CONST auto &[URI, Document] : Server->Documents)
    {
        URIs[Document.Path] = URI;
        Results[URI];
    }

    for (CONST auto &Unit : Analyzer.Units)
    {
        STRING URI = URIs.contains(Unit->Path) ? URIs[Unit->Path] : InkerdPathURI(Unit->Path);
        Results[URI];
    }

    for (CONST DIAGNOSTIC &Diagnostic : Analyzer.Diagnostics)
    {
        STRING URI = URIs.contains(Diagnostic.Path) ? URIs[Diagnostic.Path] : InkerdPathURI(Diagnostic.Path);
        Results[URI].push_back(InkerdDiagnostic(Diagnostic));
    }

    for (CONST STRING &URI : Server->Published)
    {
        Results[URI];
    }

    Server->Published.clear();

    for (auto &[URI, Items] : Results)
    {
        JSON_VALUE Params   = JSONObject({{"uri", JSONString(URI)}, {"diagnostics", JSONArray(std::move(Items))}});
        auto       Document = Server->Documents.find(URI);

        if (Document != Server->Documents.end())
        {
            Params.Object["version"]     = JSONNumber(Document->second.Version);
            Document->second.Diagnostics = JSONGet(Params, "diagnostics");
        }

        if (!JSONGet(Params, "diagnostics").Array.empty() || Document != Server->Documents.end())
        {
            Server->Published.insert(URI);
        }

        InkerdSend(JSONObject({{"jsonrpc", JSONString("2.0")}, {"method", JSONString("textDocument/publishDiagnostics")}, {"params", std::move(Params)}}));
    }
}

STATIC BOOLEAN InkerdPositionLess(CONST JSON_VALUE &Left, CONST JSON_VALUE &Right)
{
    INT64 LeftLine       = 0;
    INT64 RightLine      = 0;
    INT64 LeftCharacter  = 0;
    INT64 RightCharacter = 0;
    JSONInteger(JSONGet(Left, "line"), &LeftLine);
    JSONInteger(JSONGet(Right, "line"), &RightLine);
    JSONInteger(JSONGet(Left, "character"), &LeftCharacter);
    JSONInteger(JSONGet(Right, "character"), &RightCharacter);
    return LeftLine < RightLine || (LeftLine == RightLine && LeftCharacter < RightCharacter);
}

JSON_VALUE InkerdCodeActions(CONST INKERD_DOCUMENT &Document, CONST JSON_VALUE &Range)
{
    JSON_VALUE Result = JSONArray();

    for (CONST JSON_VALUE &Diagnostic : Document.Diagnostics.Array)
    {
        CONST JSON_VALUE &Fix      = JSONGet(JSONGet(Diagnostic, "data"), "fixIt");
        CONST JSON_VALUE &Location = JSONGet(Diagnostic, "range");

        if (Fix.Kind != JSON_KIND::STRING || Fix.Text.empty())
        {
            continue;
        }

        if (Range.Kind == JSON_KIND::OBJECT && (InkerdPositionLess(JSONGet(Location, "end"), JSONGet(Range, "start")) || InkerdPositionLess(JSONGet(Range, "end"), JSONGet(Location, "start"))))
        {
            continue;
        }

        JSON_VALUE Edit = JSONObject({{"textDocument", JSONObject({{"uri", JSONString(Document.URI)}, {"version", JSONNumber(Document.Version)}})}, {"edits", JSONArray({JSONObject({{"range", Location}, {"newText", Fix}})})}});
        Result.Array.push_back(JSONObject({{"title", JSONString("Insert '" + Fix.Text + "'")}, {"kind", JSONString("quickfix")}, {"isPreferred", JSONBoolean(TRUE)}, {"diagnostics", JSONArray({Diagnostic})}, {"edit", JSONObject({{"documentChanges", JSONArray({Edit})}})}}));
    }

    return Result;
}

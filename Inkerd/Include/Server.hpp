// Inkerd/Include/Server.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <JSON.hpp>
#include <map>
#include <set>
#include <StaticAnalyzer/StaticAnalyzer.hpp>

struct INKERD_DOCUMENT
{
    STRING     URI;
    STRING     Path;
    STRING     Text;
    INT64      Version     = 0;
    JSON_VALUE Diagnostics = JSONArray();
};

struct INKERD_SERVER
{
    BOOLEAN                           Initialized = FALSE;
    BOOLEAN                           Shutdown    = FALSE;
    std::map<STRING, INKERD_DOCUMENT> Documents;
    std::set<STRING>                  Published;
    ANALYZER_OPTIONS                  Options;
};

BOOLEAN    InkerdReadFrame(STRING *Body);
VOID       InkerdSend(CONST JSON_VALUE &Message);
VOID       InkerdReply(CONST JSON_VALUE &ID, JSON_VALUE Result);
VOID       InkerdError(CONST JSON_VALUE &ID, INT64 Code, STRING_VIEW Message);
STRING     InkerdURIPath(STRING_VIEW URI);
STRING     InkerdPathURI(STRING_VIEW Path);
JSON_VALUE InkerdPosition(STRING_VIEW Source, SIZE Offset);
VOID       InkerdAnalyzeAll(INKERD_SERVER *Server);
JSON_VALUE InkerdCodeActions(CONST INKERD_DOCUMENT &Document, CONST JSON_VALUE &Range);
INT32      InkerdRun(CONST STRING &Executable);

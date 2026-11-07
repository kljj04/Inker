// Inkerd/Source/Server.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Server.hpp>
#include <iostream>
#include <stdexcept>

STATIC BOOLEAN InkerdDocumentUpdate(INKERD_SERVER *Server, STRING_VIEW Method, CONST JSON_VALUE &Params)
{
    CONST JSON_VALUE &Item     = JSONGet(Params, "textDocument");
    CONST JSON_VALUE &URIValue = JSONGet(Item, "uri");

    if (URIValue.Kind != JSON_KIND::STRING)
    {
        return FALSE;
    }

    STRING URI  = URIValue.Text;
    STRING Path = InkerdURIPath(URI);

    if (Path.empty())
    {
        return FALSE;
    }

    Path = AnalyzerCanonicalPath(Path);

    if (Method == "textDocument/didClose")
    {
        Server->Documents.erase(URI);
        InkerdAnalyzeAll(Server);
        return TRUE;
    }

    INT64 Version;

    if (!JSONInteger(JSONGet(Item, "version"), &Version))
    {
        return FALSE;
    }

    STRING Text;

    if (Method == "textDocument/didOpen")
    {
        CONST JSON_VALUE &Content = JSONGet(Item, "text");

        if (Content.Kind != JSON_KIND::STRING)
        {
            return FALSE;
        }

        Text = Content.Text;
    }
    else
    {
        auto              Existing = Server->Documents.find(URI);
        CONST JSON_VALUE &Changes  = JSONGet(Params, "contentChanges");

        if (Existing == Server->Documents.end() || Version <= Existing->second.Version || Changes.Kind != JSON_KIND::ARRAY || Changes.Array.empty())
        {
            return FALSE;
        }

        for (CONST JSON_VALUE &Change : Changes.Array)
        {
            CONST JSON_VALUE &Content = JSONGet(Change, "text");

            if (Content.Kind != JSON_KIND::STRING || JSONGet(Change, "range").Kind != JSON_KIND::NIL)
            {
                return FALSE;
            }

            Text = Content.Text;
        }
    }

    INKERD_DOCUMENT Document;
    Document.URI     = URI;
    Document.Path    = Path;
    Document.Text    = std::move(Text);
    Document.Version = Version;
    Server->Documents.insert_or_assign(URI, std::move(Document));
    InkerdAnalyzeAll(Server);
    return TRUE;
}

STATIC VOID InkerdInitialize(INKERD_SERVER *Server, CONST JSON_VALUE &ID, CONST JSON_VALUE &Params)
{
    if (Server->Initialized)
    {
        InkerdError(ID, -32600, "server is already initialized");
        return;
    }

    CONST JSON_VALUE &Options = JSONGet(Params, "initializationOptions");
    CONST JSON_VALUE &Stdlib  = JSONGet(Options, "stdlibPath");

    if (Stdlib.Kind == JSON_KIND::STRING && !Stdlib.Text.empty())
    {
        Server->Options.StdlibRoot = AnalyzerCanonicalPath(Stdlib.Text);
    }

    for (CONST JSON_VALUE &Folder : JSONGet(Params, "workspaceFolders").Array)
    {
        STRING Path = InkerdURIPath(JSONGet(Folder, "uri").Text);

        if (!Path.empty())
        {
            Server->Options.ModuleRoots.push_back(AnalyzerCanonicalPath(Path));
        }
    }

    STRING Root = InkerdURIPath(JSONGet(Params, "rootUri").Text);

    if (!Root.empty())
    {
        Server->Options.ModuleRoots.push_back(AnalyzerCanonicalPath(Root));
    }

    Server->Initialized = TRUE;
    InkerdReply(ID, JSONObject({{"capabilities", JSONObject({{"positionEncoding", JSONString("utf-16")}, {"textDocumentSync", JSONObject({{"openClose", JSONBoolean(TRUE)}, {"change", JSONNumber(1)}, {"save", JSONBoolean(TRUE)}})}, {"codeActionProvider", JSONBoolean(TRUE)}})}, {"serverInfo", JSONObject({{"name", JSONString("Inkerd")}, {"version", JSONString("0.1.0")}})}}));
}

INT32 InkerdRun(CONST STRING &Executable)
{
    INKERD_SERVER Server;
    Server.Options = AnalyzerDefaultOptions(Executable);
    STRING Body;

    try
    {
        while (InkerdReadFrame(&Body))
        {
            JSON_VALUE Message;

            if (!JSONParse(Body, &Message))
            {
                InkerdError({}, -32700, "invalid JSON");
                continue;
            }

            CONST JSON_VALUE &ID          = JSONGet(Message, "id");
            BOOLEAN           Request     = Message.Object.contains("id");
            CONST JSON_VALUE &MethodValue = JSONGet(Message, "method");

            if (Message.Kind != JSON_KIND::OBJECT || JSONGet(Message, "jsonrpc").Text != "2.0" || MethodValue.Kind != JSON_KIND::STRING || (Request && ID.Kind != JSON_KIND::NUMBER && ID.Kind != JSON_KIND::STRING && ID.Kind != JSON_KIND::NIL))
            {
                InkerdError({}, -32600, "invalid JSON-RPC request");
                continue;
            }

            STRING_VIEW       Method = MethodValue.Text;
            CONST JSON_VALUE &Params = JSONGet(Message, "params");

            if (Method == "exit")
            {
                return Server.Shutdown ? 0 : 1;
            }

            if (Method == "initialize" && Request)
            {
                InkerdInitialize(&Server, ID, Params);
                continue;
            }

            if (!Server.Initialized)
            {
                if (Request)
                {
                    InkerdError(ID, -32002, "server is not initialized");
                }

                continue;
            }

            if (Method == "shutdown" && Request)
            {
                Server.Shutdown = TRUE;
                InkerdReply(ID, {});
                continue;
            }

            if (Server.Shutdown)
            {
                if (Request)
                {
                    InkerdError(ID, -32600, "server has shut down");
                }

                continue;
            }

            if (Method == "textDocument/didOpen" || Method == "textDocument/didChange" || Method == "textDocument/didClose")
            {
                if (!InkerdDocumentUpdate(&Server, Method, Params))
                {
                    std::cerr << "inkerd: ignored invalid document notification\n";
                }
            }
            else if (Method == "textDocument/didSave" || Method == "workspace/didChangeWatchedFiles")
            {
                InkerdAnalyzeAll(&Server);
            }
            else if (Method == "textDocument/codeAction" && Request)
            {
                auto Document = Server.Documents.find(JSONGet(JSONGet(Params, "textDocument"), "uri").Text);
                InkerdReply(ID, Document == Server.Documents.end() ? JSONArray() : InkerdCodeActions(Document->second, JSONGet(Params, "range")));
            }
            else if (Request)
            {
                InkerdError(ID, -32601, "method not supported");
            }
        }
    }
    catch (CONST std::exception &Error)
    {
        std::cerr << "inkerd: " << Error.what() << '\n';
        return 1;
    }

    return Server.Shutdown ? 0 : 1;
}

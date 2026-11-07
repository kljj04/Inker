// Inkerd/Source/Transport.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Server.hpp>
#include <iostream>
#include <charconv>
#include <stdexcept>
#include <limits>

BOOLEAN InkerdReadFrame(STRING *Body)
{
    STRING  Line;
    SIZE    Length      = 0;
    SIZE    HeaderBytes = 0;
    BOOLEAN Found       = FALSE;

    while (std::getline(std::cin, Line))
    {
        HeaderBytes += Line.size() + 1;

        if (HeaderBytes > 8192)
        {
            throw std::invalid_argument("LSP headers exceed limit");
        }

        if (!Line.empty() && Line.back() == '\r')
        {
            Line.pop_back();
        }

        if (Line.empty())
        {
            if (!Found)
            {
                throw std::invalid_argument("missing Content-Length");
            }

            Body->resize(Length);
            std::cin.read(Body->data(), static_cast<std::streamsize>(Length));

            if (static_cast<SIZE>(std::cin.gcount()) != Length)
            {
                throw std::invalid_argument("truncated LSP body");
            }

            return TRUE;
        }

        SIZE Colon = Line.find(':');

        if (Colon == STRING::npos)
        {
            throw std::invalid_argument("invalid LSP header");
        }

        STRING Key = Line.substr(0, Colon);

        for (CHAR8 &Character : Key)
        {
            if (Character >= 'A' && Character <= 'Z')
            {
                Character += 'a' - 'A';
            }
        }

        if (Key == "content-length")
        {
            if (Found)
            {
                throw std::invalid_argument("duplicate Content-Length");
            }

            STRING_VIEW Value(Line.data() + Colon + 1, Line.size() - Colon - 1);

            while (!Value.empty() && (Value.front() == ' ' || Value.front() == '\t'))
            {
                Value.remove_prefix(1);
            }

            auto Result = std::from_chars(Value.data(), Value.data() + Value.size(), Length);

            if (Result.ec != std::errc{} || Result.ptr != Value.data() + Value.size() || Length > 16 * 1024 * 1024)
            {
                throw std::invalid_argument("invalid Content-Length");
            }

            Found = TRUE;
        }
    }

    if (HeaderBytes != 0)
    {
        throw std::invalid_argument("truncated LSP headers");
    }

    return FALSE;
}

VOID InkerdSend(CONST JSON_VALUE &Message)
{
    STRING Body = JSONSerialize(Message);
    std::cout << "Content-Length: " << Body.size() << "\r\n\r\n" << Body << std::flush;
}

VOID InkerdReply(CONST JSON_VALUE &ID, JSON_VALUE Result)
{
    InkerdSend(JSONObject({{"jsonrpc", JSONString("2.0")}, {"id", ID}, {"result", std::move(Result)}}));
}

VOID InkerdError(CONST JSON_VALUE &ID, INT64 Code, STRING_VIEW Message)
{
    InkerdSend(JSONObject({{"jsonrpc", JSONString("2.0")}, {"id", ID}, {"error", JSONObject({{"code", JSONNumber(Code)}, {"message", JSONString(Message)}})}}));
}

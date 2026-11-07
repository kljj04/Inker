// Inkerd/Include/JSON.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Types.hpp>
#include <map>
#include <vector>
#include <initializer_list>

enum class JSON_KIND : UINT8
{
    NIL,
    BOOLEAN,
    NUMBER,
    STRING,
    ARRAY,
    OBJECT
};

struct JSON_VALUE
{
    JSON_KIND                    Kind = JSON_KIND::NIL;
    STRING                       Text;
    BOOLEAN                      Boolean = FALSE;
    std::vector<JSON_VALUE>      Array;
    std::map<STRING, JSON_VALUE> Object;
};

BOOLEAN           JSONParse(STRING_VIEW Source, JSON_VALUE *Value);
STRING            JSONSerialize(CONST JSON_VALUE &Value);
JSON_VALUE        JSONObject(std::initializer_list<std::pair<CONST STRING, JSON_VALUE>> Items = {});
JSON_VALUE        JSONArray(std::vector<JSON_VALUE> Items = {});
JSON_VALUE        JSONString(STRING_VIEW Text);
JSON_VALUE        JSONNumber(INT64 Value);
JSON_VALUE        JSONBoolean(BOOLEAN Value);
CONST JSON_VALUE &JSONGet(CONST JSON_VALUE &Value, STRING_VIEW Key);
BOOLEAN           JSONInteger(CONST JSON_VALUE &Value, INT64 *Result);

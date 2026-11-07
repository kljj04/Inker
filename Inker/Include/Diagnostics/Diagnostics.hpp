// Inker/Include/Diagnostics/Diagnostic.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Types.hpp>

enum class DIAGNOSTIC_LEVEL : UINT8
{
    ERROR,
    WARNING,
    NOTE
};

struct DIAGNOSTIC
{
    STRING           Path;
    CONST STRING    *Source;
    SIZE             Start;
    SIZE             Length;
    DIAGNOSTIC_LEVEL Level;
    STRING           Message;
    STRING           FixIt{};
};

VOID DiagnosticPrint(CONST DIAGNOSTIC *Diagnostic);
VOID DiagnosticPrintSummary(SIZE ErrorCount, SIZE WarningCount);
// Inker/Source/StaticAnalyzer/StaticAnalyzer.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>

ANALYZER_UNIT *StaticAnalyzerAnalyze(STATIC_ANALYZER *Analyzer, CONST STRING &Path)
{
    StaticAnalyzerReset(Analyzer);
    ANALYZER_UNIT *Entry = AnalyzerLoad(Analyzer, Path);

    if (Entry == NULLPTR)
    {
        return NULLPTR;
    }

    AnalyzerCollect(Analyzer);
    AnalyzerBindImports(Analyzer);
    AnalyzerResolveNames(Analyzer);
    SemanticCheck(Analyzer);
    return Entry;
}

VOID StaticAnalyzerReset(STATIC_ANALYZER *Analyzer)
{
    Analyzer->Variants.clear();
    Analyzer->VariantCache.clear();
    Analyzer->Types.clear();
    Analyzer->TypeCache.clear();
    Analyzer->DeclaredTypes.clear();
    Analyzer->ExpressionTypes.clear();
    Analyzer->Conversions.clear();
    Analyzer->CharacterArrays.clear();
    Analyzer->SemanticDiagnostics.clear();
    Analyzer->CallDepth = 0;
    Analyzer->Diagnostics.clear();
    Analyzer->Bindings.clear();
    Analyzer->Declarations.clear();
    Analyzer->NodeScopes.clear();
    Analyzer->ImportAccess.clear();
    Analyzer->Loaded.clear();
    Analyzer->LoadStack.clear();
    Analyzer->Symbols.clear();
    Analyzer->Scopes.clear();
    Analyzer->Units.clear();
}

// Inkerd/Source/Inkerd.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Server.hpp>
#include <iostream>

INT32 main(INT32 Argc, CHAR8 *Argv[])
{
    if (Argc == 2 && STRING_VIEW(Argv[1]) == "--version")
    {
        std::cout << "Inkerd 0.1.0\n";
        return 0;
    }

    if (Argc > 2 || (Argc == 2 && STRING_VIEW(Argv[1]) != "--stdio"))
    {
        std::cerr << "usage: inkerd [--stdio | --version]\n";
        return 1;
    }

    return InkerdRun(Argv[0]);
}

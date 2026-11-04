# Inker

**Inker** is a C++20 interpreter for the **INK programming language**.

The project aims to provide a lightweight interpreter with a custom lexer, parser, abstract syntax tree (AST), and diagnostic system.

> **Development Status:** Inker is currently under active development and is not yet capable of executing INK programs.

## Overview

Inker is designed and implemented primarily using C-style structures and functions while taking advantage of selected C++23 features.

The interpreter currently uses a tree-walking architecture.

```text
Source Code (.ink)
       |
       v
     Lexer
       |
       v
     Parser
       |
       v
      AST
       |
       v
    Evaluator
```

The evaluator has not yet been implemented.

## Features

### Implemented

- Tokenization of INK source code
- Integer literals and identifiers
- String tokenization and escape handling
- Lexer support for string interpolation
- Unary and binary expression parsing
- Variable declaration parsing
- Expression statements
- Abstract syntax tree construction
- Source-aware diagnostics with line and column information

### Planned

- String expressions and interpolation parsing
- AST evaluation
- Variable storage and assignment
- Boolean and floating-point values
- Conditional statements and loops
- Functions and function calls
- Arrays and additional language features

## Requirements

- C++23-compatible compiler
- Linux development environment

## Usage

Inker accepts an INK source file as a command-line argument.

```bash
inker Test.ink
```

Example INK source:

```text
set A = 10;
set B = 20;
set C = -(A * B);
```

The current implementation parses the source and reports syntax errors. Program execution is not yet implemented.

## Diagnostics

Inker provides source-aware error messages.

Example:

```text
Test.ink:3:17: error: expected ';'
    3 | set C = -(A * B)
      |                 ^
```

Diagnostics identify the source file, line, column, and relevant source range.

## Implementation

Inker is written in C++23 with an emphasis on:

- Explicit data structures
- Custom integer and platform-related type aliases
- PascalCase function and variable naming
- Minimal unnecessary abstractions
- Modular compiler frontend components

## Development Status

| Component | Status |
|-----------|--------|
| Lexer | Implemented |
| Diagnostics | Implemented |
| Parser | In progress |
| AST | Basic implementation |
| Evaluator | Planned |
| Runtime | Planned |

Features and language syntax may change during development.

## License

Inker is licensed under the **Preservation of Origin License (POL) 1.0**.

Use, Modify, Distribute, including in Closed-Source Form.  
Origin Must Be Preserved.

See [LICENSE](LICENSE) for the complete license terms.

Original Author: kljj04  
Original Project: Inker  
Original Repository: https://github.com/kljj04/Inker

Copyright (C) 2026 kljj04.
// Inker/Include/Parser/AST.hpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#pragma once

#include <Types.hpp>
#include <Lexer/Tokens.hpp>

enum class AST_NODE_TYPE : UINT8
{
    PROGRAM,
    INTEGER,
    FLOAT,
    STRING,
    BOOLEAN,
    NULL_LITERAL,
    IDENTIFIER,
    ARRAY,
    ARRAY_ACCESS,
    MEMBER_ACCESS,
    UNARY,
    BINARY,
    TERNARY,
    ASSIGNMENT,
    FUNCTION_CALL,
    ARGUMENT,
    LAMBDA,
    VARIABLE_DECLARATION,
    EXPRESSION_STATEMENT,
    BLOCK,
    IF,
    WHILE,
    FOR,
    BREAK,
    CONTINUE,
    RETURN,
    FUNCTION_DECLARATION,
    PARAMETER,
    NAMESPACE_DECLARATION,
    USE_DECLARATION,
    STRUCT_DECLARATION,
    CLASS_DECLARATION,
    ENUM_DECLARATION,
    ENUM_MEMBER,
    FIELD_DECLARATION,
    METHOD_DECLARATION,
    ACCESS_MODIFIER,
    TYPE,
    POINTER_TYPE,
    ARRAY_TYPE,
    FUNCTION_TYPE,
    STRING_INTERPOLATION,
    INTERPOLATION_EXPRESSION
};

struct AST_NODE;

struct AST_NODE_LIST
{
    AST_NODE **Items;
    SIZE       Count;
    SIZE       Capacity;
};

struct AST_LOC
{
    UINT64 Start;
    UINT64 Length;
    UINT64 Line;
    UINT64 Column;
};

struct AST_EMPTY
{
};

struct AST_INTEGER
{
    INT64 Value;
};

struct AST_FLOAT
{
    FLOAT64 Value;
};

struct AST_STRING
{
    STRING_VIEW Value;
};

struct AST_BOOLEAN
{
    BOOLEAN Value;
};

struct AST_IDENTIFIER
{
    STRING_VIEW Name;
};

struct AST_ARRAY
{
    AST_NODE_LIST Elements;
};

struct AST_ARRAY_ACCESS
{
    AST_NODE *Array;
    AST_NODE *Index;
};

struct AST_MEMBER_ACCESS
{
    AST_NODE   *Object;
    STRING_VIEW Member;
};

struct AST_UNARY
{
    TOKENS    Operator;
    AST_NODE *Operand;
    BOOLEAN   Postfix;
};

struct AST_BINARY
{
    TOKENS    Operator;
    AST_NODE *Left;
    AST_NODE *Right;
};

struct AST_TERNARY
{
    AST_NODE *Condition;
    AST_NODE *ThenExpression;
    AST_NODE *ElseExpression;
};

struct AST_ASSIGNMENT
{
    TOKENS    Operator;
    AST_NODE *Target;
    AST_NODE *Value;
};

struct AST_FUNCTION_CALL
{
    AST_NODE     *Callee;
    AST_NODE_LIST Arguments;
};

struct AST_ARGUMENT
{
    STRING_VIEW Name;
    AST_NODE   *Value;
};

struct AST_LAMBDA
{
    AST_NODE_LIST Parameters;
    AST_NODE     *ReturnType;
    AST_NODE     *Body;
};

struct AST_VARIABLE_DECLARATION
{
    STRING_VIEW Name;
    AST_NODE   *VariableType;
    AST_NODE   *Initializer;
};

struct AST_EXPRESSION_STATEMENT
{
    AST_NODE *Expression;
};

struct AST_BLOCK
{
    AST_NODE_LIST Statements;
};

struct AST_IF
{
    AST_NODE *Condition;
    AST_NODE *ThenBranch;
    AST_NODE *ElseBranch;
};

struct AST_WHILE
{
    AST_NODE *Condition;
    AST_NODE *Body;
};

struct AST_FOR
{
    AST_NODE *Initializer;
    AST_NODE *Condition;
    AST_NODE *Increment;
    AST_NODE *Body;
};

struct AST_RETURN
{
    AST_NODE *Value;
};

struct AST_FUNCTION_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE_LIST Parameters;
    AST_NODE     *ReturnType;
    AST_NODE     *Body;
};

struct AST_PARAMETER
{
    STRING_VIEW Name;
    AST_NODE   *ParameterType;
    AST_NODE   *DefaultValue;
};

struct AST_NAMESPACE_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE_LIST Declarations;
};

struct AST_USE_DECLARATION
{
    STRING_VIEW Path;
    STRING_VIEW Alias;
    STRING_VIEW File{};
};

struct AST_STRUCT_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE_LIST Members;
};

struct AST_CLASS_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE_LIST Members;
};

struct AST_ENUM_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE     *UnderlyingType;
    AST_NODE_LIST Members;
};

struct AST_ENUM_MEMBER
{
    STRING_VIEW Name;
    AST_NODE   *Value;
};

struct AST_FIELD_DECLARATION
{
    STRING_VIEW Name;
    AST_NODE   *FieldType;
    AST_NODE   *Initializer;
};

struct AST_METHOD_DECLARATION
{
    STRING_VIEW   Name;
    AST_NODE_LIST Parameters;
    AST_NODE     *ReturnType;
    AST_NODE     *Body;
};

struct AST_ACCESS_MODIFIER
{
    TOKENS Modifier;
};

struct AST_TYPE
{
    TOKENS      Token;
    STRING_VIEW Name;
};

struct AST_POINTER_TYPE
{
    AST_NODE *BaseType;
};

struct AST_ARRAY_TYPE
{
    AST_NODE *ElementType;
    AST_NODE *Size;
};

struct AST_FUNCTION_TYPE
{
    AST_NODE_LIST Parameters;
    AST_NODE     *ReturnType;
};

struct AST_STRING_INTERPOLATION
{
    AST_NODE_LIST Segments;
};

struct AST_INTERPOLATION_EXPRESSION
{
    AST_NODE *Expression;
};

struct AST_NODE
{
    AST_NODE_TYPE Type;
    AST_LOC       Location;

    union
    {
        AST_EMPTY                    Empty;
        AST_NODE_LIST                Program;
        AST_INTEGER                  Integer;
        AST_FLOAT                    Float;
        AST_STRING                   String;
        AST_BOOLEAN                  Boolean;
        AST_IDENTIFIER               Identifier;
        AST_ARRAY                    Array;
        AST_ARRAY_ACCESS             ArrayAccess;
        AST_MEMBER_ACCESS            MemberAccess;
        AST_UNARY                    Unary;
        AST_BINARY                   Binary;
        AST_TERNARY                  Ternary;
        AST_ASSIGNMENT               Assignment;
        AST_FUNCTION_CALL            FunctionCall;
        AST_ARGUMENT                 Argument;
        AST_LAMBDA                   Lambda;
        AST_VARIABLE_DECLARATION     VariableDeclaration;
        AST_EXPRESSION_STATEMENT     ExpressionStatement;
        AST_BLOCK                    Block;
        AST_IF                       If;
        AST_WHILE                    While;
        AST_FOR                      For;
        AST_RETURN                   Return;
        AST_FUNCTION_DECLARATION     FunctionDeclaration;
        AST_PARAMETER                Parameter;
        AST_NAMESPACE_DECLARATION    NamespaceDeclaration;
        AST_USE_DECLARATION          UseDeclaration;
        AST_STRUCT_DECLARATION       StructDeclaration;
        AST_CLASS_DECLARATION        ClassDeclaration;
        AST_ENUM_DECLARATION         EnumDeclaration;
        AST_ENUM_MEMBER              EnumMember;
        AST_FIELD_DECLARATION        FieldDeclaration;
        AST_METHOD_DECLARATION       MethodDeclaration;
        AST_ACCESS_MODIFIER          AccessModifier;
        AST_TYPE                     TypeInfo;
        AST_POINTER_TYPE             PointerType;
        AST_ARRAY_TYPE               ArrayType;
        AST_FUNCTION_TYPE            FunctionType;
        AST_STRING_INTERPOLATION     StringInterpolation;
        AST_INTERPOLATION_EXPRESSION InterpolationExpression;
    };
};

struct AST_ARENA_BLOCK;

struct AST_ARENA
{
    AST_ARENA_BLOCK *Head            = NULLPTR;
    SIZE             InitialCapacity = 4096;
    SIZE             NextCapacity    = 4096;

    AST_ARENA(VOID)                         = default;
    AST_ARENA(CONST AST_ARENA &)            = delete;
    AST_ARENA &operator=(CONST AST_ARENA &) = delete;
};

VOID      ASTArenaInit(AST_ARENA *Arena, SIZE InitialCapacity = 4096) NOEXCEPT;
VOID     *ASTArenaAllocate(AST_ARENA *Arena, SIZE Size, SIZE Alignment = ALIGNOF(std::max_align_t)) NOEXCEPT;
VOID      ASTArenaReset(AST_ARENA *Arena) NOEXCEPT;
VOID      ASTArenaDestroy(AST_ARENA *Arena) NOEXCEPT;
AST_NODE *ASTCreateNode(AST_ARENA *Arena, AST_NODE_TYPE Type, AST_LOC Location = {}) NOEXCEPT;
VOID      ASTNodeListInit(AST_NODE_LIST *List) NOEXCEPT;
BOOLEAN   ASTNodeListAppend(AST_ARENA *Arena, AST_NODE_LIST *List, AST_NODE *Node) NOEXCEPT;

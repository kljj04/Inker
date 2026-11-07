// Inker/Source/Parser/AST.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/AST.hpp>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

struct AST_ARENA_BLOCK
{
    AST_ARENA_BLOCK *Next;
    VOID            *Memory;
    SIZE             Capacity;
    SIZE             Used;
};

static_assert(std::is_trivially_destructible_v<AST_NODE>);
static_assert(std::is_trivially_copyable_v<AST_NODE>);

VOID ASTArenaInit(AST_ARENA *Arena, SIZE InitialCapacity) NOEXCEPT
{
    if (Arena == NULLPTR)
    {
        return;
    }
    ASTArenaDestroy(Arena);
    Arena->InitialCapacity = InitialCapacity == 0 ? 4096 : InitialCapacity;
    Arena->NextCapacity    = Arena->InitialCapacity;
}

VOID *ASTArenaAllocate(AST_ARENA *Arena, SIZE Size, SIZE Alignment) NOEXCEPT
{
    CONST SIZE Max = std::numeric_limits<SIZE>::max();
    if (Arena == NULLPTR || Size == 0 || Alignment == 0 || (Alignment & (Alignment - 1)) != 0 || Size > Max - (Alignment - 1))
    {
        return NULLPTR;
    }
    AST_ARENA_BLOCK *Block = Arena->Head;
    if (Block != NULLPTR)
    {
        VOID *Memory = static_cast<UINT8 *>(Block->Memory) + Block->Used;
        SIZE  Space  = Block->Capacity - Block->Used;
        if (std::align(Alignment, Size, Memory, Space) != NULLPTR)
        {
            Block->Used = Block->Capacity - Space + Size;
            return Memory;
        }
    }
    CONST SIZE Required = Size + Alignment - 1;
    CONST SIZE Capacity = Required > Arena->NextCapacity ? Required : Arena->NextCapacity;
    if (Capacity > Max - sizeof(AST_ARENA_BLOCK))
    {
        return NULLPTR;
    }
    VOID *Storage = std::malloc(sizeof(AST_ARENA_BLOCK) + Capacity);
    if (Storage == NULLPTR)
    {
        return NULLPTR;
    }
    Block        = ::new (Storage) AST_ARENA_BLOCK{Arena->Head, static_cast<UINT8 *>(Storage) + sizeof(AST_ARENA_BLOCK), Capacity, 0};
    VOID *Memory = Block->Memory;
    SIZE  Space  = Capacity;
    if (std::align(Alignment, Size, Memory, Space) == NULLPTR)
    {
        std::free(Storage);
        return NULLPTR;
    }
    Block->Used         = Capacity - Space + Size;
    Arena->Head         = Block;
    Arena->NextCapacity = Capacity > Max / 2 ? Capacity : Capacity * 2;
    return Memory;
}

VOID ASTArenaReset(AST_ARENA *Arena) NOEXCEPT
{
    if (Arena == NULLPTR)
    {
        return;
    }
    AST_ARENA_BLOCK *Block = Arena->Head;
    while (Block != NULLPTR)
    {
        AST_ARENA_BLOCK *Next = Block->Next;
        std::free(Block);
        Block = Next;
    }
    Arena->Head         = NULLPTR;
    Arena->NextCapacity = Arena->InitialCapacity;
}

VOID ASTArenaDestroy(AST_ARENA *Arena) NOEXCEPT
{
    ASTArenaReset(Arena);
}

AST_NODE *ASTCreateNode(AST_ARENA *Arena, AST_NODE_TYPE Type, AST_LOC Location) NOEXCEPT
{
    if (static_cast<UINT8>(Type) > static_cast<UINT8>(AST_NODE_TYPE::INTERPOLATION_EXPRESSION))
    {
        return NULLPTR;
    }
    VOID *Memory = ASTArenaAllocate(Arena, sizeof(AST_NODE), ALIGNOF(AST_NODE));
    if (Memory == NULLPTR)
    {
        return NULLPTR;
    }
    AST_NODE *Node = ::new (Memory) AST_NODE{Type, Location, {AST_EMPTY{}}};
    switch (Type)
    {
        case AST_NODE_TYPE::PROGRAM:
            ::new (static_cast<VOID *>(&Node->Program)) AST_NODE_LIST{};
            break;
        case AST_NODE_TYPE::INTEGER:
            ::new (static_cast<VOID *>(&Node->Integer)) AST_INTEGER{};
            break;
        case AST_NODE_TYPE::FLOAT:
            ::new (static_cast<VOID *>(&Node->Float)) AST_FLOAT{};
            break;
        case AST_NODE_TYPE::STRING:
            ::new (static_cast<VOID *>(&Node->String)) AST_STRING{};
            break;
        case AST_NODE_TYPE::BOOLEAN:
            ::new (static_cast<VOID *>(&Node->Boolean)) AST_BOOLEAN{};
            break;
        case AST_NODE_TYPE::NULL_LITERAL:
            ::new (static_cast<VOID *>(&Node->Empty)) AST_EMPTY{};
            break;
        case AST_NODE_TYPE::IDENTIFIER:
            ::new (static_cast<VOID *>(&Node->Identifier)) AST_IDENTIFIER{};
            break;
        case AST_NODE_TYPE::ARRAY:
            ::new (static_cast<VOID *>(&Node->Array)) AST_ARRAY{};
            break;
        case AST_NODE_TYPE::ARRAY_ACCESS:
            ::new (static_cast<VOID *>(&Node->ArrayAccess)) AST_ARRAY_ACCESS{};
            break;
        case AST_NODE_TYPE::MEMBER_ACCESS:
            ::new (static_cast<VOID *>(&Node->MemberAccess)) AST_MEMBER_ACCESS{};
            break;
        case AST_NODE_TYPE::UNARY:
            ::new (static_cast<VOID *>(&Node->Unary)) AST_UNARY{};
            break;
        case AST_NODE_TYPE::BINARY:
            ::new (static_cast<VOID *>(&Node->Binary)) AST_BINARY{};
            break;
        case AST_NODE_TYPE::TERNARY:
            ::new (static_cast<VOID *>(&Node->Ternary)) AST_TERNARY{};
            break;
        case AST_NODE_TYPE::ASSIGNMENT:
            ::new (static_cast<VOID *>(&Node->Assignment)) AST_ASSIGNMENT{};
            break;
        case AST_NODE_TYPE::FUNCTION_CALL:
            ::new (static_cast<VOID *>(&Node->FunctionCall)) AST_FUNCTION_CALL{};
            break;
        case AST_NODE_TYPE::ARGUMENT:
            ::new (static_cast<VOID *>(&Node->Argument)) AST_ARGUMENT{};
            break;
        case AST_NODE_TYPE::LAMBDA:
            ::new (static_cast<VOID *>(&Node->Lambda)) AST_LAMBDA{};
            break;
        case AST_NODE_TYPE::VARIABLE_DECLARATION:
            ::new (static_cast<VOID *>(&Node->VariableDeclaration)) AST_VARIABLE_DECLARATION{};
            break;
        case AST_NODE_TYPE::EXPRESSION_STATEMENT:
            ::new (static_cast<VOID *>(&Node->ExpressionStatement)) AST_EXPRESSION_STATEMENT{};
            break;
        case AST_NODE_TYPE::BLOCK:
            ::new (static_cast<VOID *>(&Node->Block)) AST_BLOCK{};
            break;
        case AST_NODE_TYPE::IF:
            ::new (static_cast<VOID *>(&Node->If)) AST_IF{};
            break;
        case AST_NODE_TYPE::WHILE:
            ::new (static_cast<VOID *>(&Node->While)) AST_WHILE{};
            break;
        case AST_NODE_TYPE::FOR:
            ::new (static_cast<VOID *>(&Node->For)) AST_FOR{};
            break;
        case AST_NODE_TYPE::BREAK:
            ::new (static_cast<VOID *>(&Node->Empty)) AST_EMPTY{};
            break;
        case AST_NODE_TYPE::CONTINUE:
            ::new (static_cast<VOID *>(&Node->Empty)) AST_EMPTY{};
            break;
        case AST_NODE_TYPE::RETURN:
            ::new (static_cast<VOID *>(&Node->Return)) AST_RETURN{};
            break;
        case AST_NODE_TYPE::FUNCTION_DECLARATION:
            ::new (static_cast<VOID *>(&Node->FunctionDeclaration)) AST_FUNCTION_DECLARATION{};
            break;
        case AST_NODE_TYPE::PARAMETER:
            ::new (static_cast<VOID *>(&Node->Parameter)) AST_PARAMETER{};
            break;
        case AST_NODE_TYPE::NAMESPACE_DECLARATION:
            ::new (static_cast<VOID *>(&Node->NamespaceDeclaration)) AST_NAMESPACE_DECLARATION{};
            break;
        case AST_NODE_TYPE::USE_DECLARATION:
            ::new (static_cast<VOID *>(&Node->UseDeclaration)) AST_USE_DECLARATION{};
            break;
        case AST_NODE_TYPE::STRUCT_DECLARATION:
            ::new (static_cast<VOID *>(&Node->StructDeclaration)) AST_STRUCT_DECLARATION{};
            break;
        case AST_NODE_TYPE::CLASS_DECLARATION:
            ::new (static_cast<VOID *>(&Node->ClassDeclaration)) AST_CLASS_DECLARATION{};
            break;
        case AST_NODE_TYPE::ENUM_DECLARATION:
            ::new (static_cast<VOID *>(&Node->EnumDeclaration)) AST_ENUM_DECLARATION{};
            break;
        case AST_NODE_TYPE::ENUM_MEMBER:
            ::new (static_cast<VOID *>(&Node->EnumMember)) AST_ENUM_MEMBER{};
            break;
        case AST_NODE_TYPE::FIELD_DECLARATION:
            ::new (static_cast<VOID *>(&Node->FieldDeclaration)) AST_FIELD_DECLARATION{};
            break;
        case AST_NODE_TYPE::METHOD_DECLARATION:
            ::new (static_cast<VOID *>(&Node->MethodDeclaration)) AST_METHOD_DECLARATION{};
            break;
        case AST_NODE_TYPE::ACCESS_MODIFIER:
            ::new (static_cast<VOID *>(&Node->AccessModifier)) AST_ACCESS_MODIFIER{};
            break;
        case AST_NODE_TYPE::TYPE:
            ::new (static_cast<VOID *>(&Node->TypeInfo)) AST_TYPE{};
            break;
        case AST_NODE_TYPE::POINTER_TYPE:
            ::new (static_cast<VOID *>(&Node->PointerType)) AST_POINTER_TYPE{};
            break;
        case AST_NODE_TYPE::ARRAY_TYPE:
            ::new (static_cast<VOID *>(&Node->ArrayType)) AST_ARRAY_TYPE{};
            break;
        case AST_NODE_TYPE::FUNCTION_TYPE:
            ::new (static_cast<VOID *>(&Node->FunctionType)) AST_FUNCTION_TYPE{};
            break;
        case AST_NODE_TYPE::STRING_INTERPOLATION:
            ::new (static_cast<VOID *>(&Node->StringInterpolation)) AST_STRING_INTERPOLATION{};
            break;
        case AST_NODE_TYPE::INTERPOLATION_EXPRESSION:
            ::new (static_cast<VOID *>(&Node->InterpolationExpression)) AST_INTERPOLATION_EXPRESSION{};
            break;
    }
    return Node;
}

VOID ASTNodeListInit(AST_NODE_LIST *List) NOEXCEPT
{
    if (List != NULLPTR)
    {
        *List = {};
    }
}

BOOLEAN ASTNodeListAppend(AST_ARENA *Arena, AST_NODE_LIST *List, AST_NODE *Node) NOEXCEPT
{
    CONST SIZE MaxCount = std::numeric_limits<SIZE>::max() / sizeof(AST_NODE *);
    if (Arena == NULLPTR || List == NULLPTR || Node == NULLPTR || List->Count > List->Capacity || List->Capacity > MaxCount || (List->Capacity != 0 && List->Items == NULLPTR))
    {
        return FALSE;
    }
    if (List->Count == List->Capacity)
    {
        if (List->Capacity == MaxCount)
        {
            return FALSE;
        }
        SIZE       Capacity = List->Capacity == 0 ? 8 : (List->Capacity > MaxCount / 2 ? MaxCount : List->Capacity * 2);
        AST_NODE **Items    = static_cast<AST_NODE **>(ASTArenaAllocate(Arena, Capacity * sizeof(AST_NODE *), ALIGNOF(AST_NODE *)));
        if (Items == NULLPTR)
        {
            return FALSE;
        }
        for (SIZE Index = 0; Index < List->Count; ++Index)
        {
            std::construct_at(Items + Index, List->Items[Index]);
        }
        List->Items    = Items;
        List->Capacity = Capacity;
    }
    std::construct_at(List->Items + List->Count, Node);
    ++List->Count;
    return TRUE;
}

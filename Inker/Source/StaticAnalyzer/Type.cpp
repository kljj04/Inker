// Inker/Source/StaticAnalyzer/Type.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>
#include <bit>
#include <algorithm>

SEMANTIC_TYPE *SemanticType(STATIC_ANALYZER *Analyzer, SEMANTIC_KIND Kind, UINT16 Bits, BOOLEAN Unsigned, SEMANTIC_TYPE *Element, SIZE Count, BOOLEAN Sized, ANALYZER_SYMBOL *Owner)
{
    STRING Key      = std::to_string(static_cast<UINT8>(Kind)) + ":" + std::to_string(Bits) + ":" + std::to_string(Unsigned) + ":" + std::to_string(Element == NULLPTR ? 0 : Element->ID) + ":" + std::to_string(Count) + ":" + std::to_string(Sized) + ":" + std::to_string(reinterpret_cast<UINTN>(Owner));
    auto   Existing = Analyzer->TypeCache.find(Key);
    if (Existing != Analyzer->TypeCache.end())
    {
        return Existing->second;
    }
    Analyzer->Types.emplace_back();
    SEMANTIC_TYPE *Type = &Analyzer->Types.back();
    Type->ID            = Analyzer->Types.size();
    Type->Kind          = Kind;
    Type->Bits          = Bits;
    Type->Unsigned      = Unsigned;
    Type->Element       = Element;
    Type->Count         = Count;
    Type->Sized         = Sized;
    Type->Owner         = Owner;
    Analyzer->TypeCache.emplace(Key, Type);
    return Type;
}

STRING SemanticName(CONST SEMANTIC_TYPE *Type)
{
    if (Type == NULLPTR)
    {
        return "<error>";
    }
    switch (Type->Kind)
    {
        case SEMANTIC_KIND::VOID:
            return "VOID";
        case SEMANTIC_KIND::BOOLEAN:
            return "BOOLEAN";
        case SEMANTIC_KIND::STRING:
            return "STRING";
        case SEMANTIC_KIND::INTEGER:
            return STRING(Type->Unsigned ? "UINT" : "INT") + std::to_string(Type->Bits);
        case SEMANTIC_KIND::FLOAT:
            return "FLOAT" + std::to_string(Type->Bits);
        case SEMANTIC_KIND::CHARACTER:
            return "CHAR" + std::to_string(Type->Bits);
        case SEMANTIC_KIND::AUTO:
            return "AUTO" + std::to_string(Type->Bits);
        case SEMANTIC_KIND::POINTER:
            return SemanticName(Type->Element) + "*";
        case SEMANTIC_KIND::ARRAY:
            return SemanticName(Type->Element) + "[" + (Type->Sized ? std::to_string(Type->Count) : STRING{}) + "]";
        case SEMANTIC_KIND::RECORD:
        case SEMANTIC_KIND::ENUM:
        case SEMANTIC_KIND::FUNCTION:
        case SEMANTIC_KIND::NAMESPACE:
        case SEMANTIC_KIND::TYPE_OBJECT:
            return Type->Owner == NULLPTR ? "<unknown>" : Type->Owner->Name;
        case SEMANTIC_KIND::NULL_VALUE:
            return "NULL";
        default:
            return "<error>";
    }
}

BOOLEAN SemanticNumeric(CONST SEMANTIC_TYPE *Type)
{
    return Type != NULLPTR && (Type->Kind == SEMANTIC_KIND::INTEGER || Type->Kind == SEMANTIC_KIND::FLOAT || Type->Kind == SEMANTIC_KIND::CHARACTER);
}

BOOLEAN SemanticIntegral(CONST SEMANTIC_TYPE *Type)
{
    return Type != NULLPTR && (Type->Kind == SEMANTIC_KIND::INTEGER || Type->Kind == SEMANTIC_KIND::CHARACTER);
}

BOOLEAN SemanticHasAuto(CONST SEMANTIC_TYPE *Type)
{
    while (Type != NULLPTR && (Type->Kind == SEMANTIC_KIND::ARRAY || Type->Kind == SEMANTIC_KIND::POINTER))
    {
        Type = Type->Element;
    }
    return Type != NULLPTR && Type->Kind == SEMANTIC_KIND::AUTO;
}

BOOLEAN SemanticCompatible(CONST SEMANTIC_TYPE *Expected, CONST SEMANTIC_TYPE *Actual)
{
    if (Expected == Actual)
    {
        return TRUE;
    }
    if (Expected == NULLPTR || Actual == NULLPTR || Expected->Kind == SEMANTIC_KIND::ERROR || Actual->Kind == SEMANTIC_KIND::ERROR)
    {
        return TRUE;
    }
    if (Expected->Kind == SEMANTIC_KIND::AUTO || Actual->Kind == SEMANTIC_KIND::AUTO)
    {
        return TRUE;
    }
    if (Expected->Kind == SEMANTIC_KIND::ARRAY && Actual->Kind == SEMANTIC_KIND::ARRAY)
    {
        return (!Expected->Sized || (Actual->Sized && Expected->Count == Actual->Count)) && SemanticCompatible(Expected->Element, Actual->Element);
    }
    if (Expected->Kind == SEMANTIC_KIND::POINTER && Actual->Kind == SEMANTIC_KIND::NULL_VALUE)
    {
        return TRUE;
    }
    return FALSE;
}

UINT64 SemanticMask(UINT16 Bits)
{
    return Bits >= 64 ? UINT64(-1) : (UINT64(1) << Bits) - 1;
}

INT64 SemanticSigned(UINT64 Value, UINT16 Bits)
{
    UINT64 Mask = SemanticMask(Bits);
    Value &= Mask;
    if (Bits < 64 && (Value & (UINT64(1) << (Bits - 1))) != 0)
    {
        Value |= ~Mask;
    }
    return std::bit_cast<INT64>(Value);
}

VOID SemanticReport(SEMANTIC_CONTEXT *Context, AST_NODE *Node, DIAGNOSTIC_LEVEL Level, DIAGNOSTIC_MESSAGE Message, STRING_VIEW First, STRING_VIEW Second, STRING_VIEW Third)
{
    AST_LOC Location = Node == NULLPTR ? AST_LOC{} : Node->Location;
    if (Node != NULLPTR && (Message == DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL || Message == DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL || Message == DIAGNOSTIC_MESSAGE::UNINITIALIZED_VALUE))
    {
        if (Node->Type == AST_NODE_TYPE::IDENTIFIER)
        {
            Location = AnalyzerLocation(Context->Unit, Node, Node->Identifier.Name);
        }
        else if (Node->Type == AST_NODE_TYPE::MEMBER_ACCESS)
        {
            Location = AnalyzerLocation(Context->Unit, Node, Node->MemberAccess.Member);
        }
    }
    STRING Text = DiagnosticFormatMessage(Message, {First, Second, Third});
    STRING Key  = Context->Unit->Path + ":" + std::to_string(Location.Start) + ":" + std::to_string(Location.Length) + ":" + std::to_string(static_cast<UINT8>(Level)) + ":" + Text;
    if (!Context->Analyzer->SemanticDiagnostics.insert(Key).second)
    {
        return;
    }
    DIAGNOSTIC Diagnostic{};
    Diagnostic.Path    = Context->Unit->Path;
    Diagnostic.Source  = &Context->Unit->Lexer.Source;
    Diagnostic.Start   = Location.Start;
    Diagnostic.Length  = Location.Length;
    Diagnostic.Level   = Level;
    Diagnostic.Message = std::move(Text);
    Context->Analyzer->Diagnostics.push_back(std::move(Diagnostic));
}

SEMANTIC_TYPE *SemanticDeclared(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node)
{
    if (Node == NULLPTR)
    {
        return SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    auto Cached = Analyzer->DeclaredTypes.find(Node);
    if (Cached != Analyzer->DeclaredTypes.end())
    {
        return Cached->second;
    }
    SEMANTIC_CONTEXT Context;
    Context.Analyzer     = Analyzer;
    Context.Unit         = Scope->Unit;
    Context.Scope        = Scope;
    Context.CheckingType = TRUE;
    SEMANTIC_TYPE *Type  = NULLPTR;
    if (Node->Type == AST_NODE_TYPE::POINTER_TYPE)
    {
        Type = SemanticType(Analyzer, SEMANTIC_KIND::POINTER, 64, TRUE, SemanticDeclared(Analyzer, Scope, Node->PointerType.BaseType));
    }
    else if (Node->Type == AST_NODE_TYPE::ARRAY_TYPE)
    {
        SEMANTIC_TYPE *Element = SemanticDeclared(Analyzer, Scope, Node->ArrayType.ElementType);
        SEMANTIC_VALUE Size;
        if (Node->ArrayType.Size != NULLPTR)
        {
            Size = SemanticExpression(&Context, Node->ArrayType.Size);
            if (!SemanticIntegral(Size.Type) && Size.Type->Kind != SEMANTIC_KIND::ERROR && Size.Type->Kind != SEMANTIC_KIND::AUTO)
            {
                SemanticReport(&Context, Node->ArrayType.Size, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, "integer array size", SemanticName(Size.Type));
            }
            if (Size.IntegerKnown && (!Size.Type->Unsigned && SemanticSigned(Size.Integer, Size.Type->Bits) <= 0))
            {
                SemanticReport(&Context, Node->ArrayType.Size, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_ARRAY_SIZE);
            }
        }
        if (Element->Kind == SEMANTIC_KIND::VOID)
        {
            SemanticReport(&Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_TYPE, "VOID array element");
        }
        Type = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Element, Size.IntegerKnown ? Size.Integer : 0, Size.IntegerKnown);
    }
    else if (Node->Type == AST_NODE_TYPE::TYPE)
    {
        TOKENS Token = Node->TypeInfo.Token;
        if (Token >= TOKENS::TOKEN_TYPE_INT8 && Token <= TOKENS::TOKEN_TYPE_INT64)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 8u << (static_cast<UINT8>(Token) - static_cast<UINT8>(TOKENS::TOKEN_TYPE_INT8)));
        }
        else if (Token >= TOKENS::TOKEN_TYPE_UINT8 && Token <= TOKENS::TOKEN_TYPE_UINT64)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, 8u << (static_cast<UINT8>(Token) - static_cast<UINT8>(TOKENS::TOKEN_TYPE_UINT8)), TRUE);
        }
        else if (Token >= TOKENS::TOKEN_TYPE_AUTO8 && Token <= TOKENS::TOKEN_TYPE_AUTO64)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::AUTO, 8u << (static_cast<UINT8>(Token) - static_cast<UINT8>(TOKENS::TOKEN_TYPE_AUTO8)));
        }
        else if (Token == TOKENS::TOKEN_TYPE_FLOAT32 || Token == TOKENS::TOKEN_TYPE_FLOAT64)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::FLOAT, Token == TOKENS::TOKEN_TYPE_FLOAT32 ? 32 : 64);
        }
        else if (Token == TOKENS::TOKEN_TYPE_CHAR8 || Token == TOKENS::TOKEN_TYPE_CHAR16)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::CHARACTER, Token == TOKENS::TOKEN_TYPE_CHAR8 ? 8 : 16, TRUE);
        }
        else if (Token == TOKENS::TOKEN_TYPE_VOID)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::VOID);
        }
        else if (Token == TOKENS::TOKEN_TYPE_BOOLEAN)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1);
        }
        else if (Token == TOKENS::TOKEN_TYPE_STRING)
        {
            Type = SemanticType(Analyzer, SEMANTIC_KIND::STRING);
        }
        else
        {
            auto Binding = Analyzer->Bindings.find(Node);
            if (Binding != Analyzer->Bindings.end() && Binding->second != NULLPTR && Binding->second->Kind == ANALYZER_SYMBOL_KIND::TYPE)
            {
                Type = SemanticSymbolType(Analyzer, Binding->second);
            }
        }
    }
    if (Type == NULLPTR)
    {
        Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    Analyzer->DeclaredTypes[Node] = Type;
    return Type;
}

SEMANTIC_TYPE *SemanticSymbolType(STATIC_ANALYZER *Analyzer, ANALYZER_SYMBOL *Symbol)
{
    Symbol = AnalyzerUnalias(Symbol);
    if (Symbol == NULLPTR || Symbol->Kind == ANALYZER_SYMBOL_KIND::ERROR)
    {
        return SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    if (Symbol->TypeChecked)
    {
        return Symbol->Type;
    }
    if (Symbol->TypeChecking)
    {
        return Symbol->Type == NULLPTR ? SemanticType(Analyzer, SEMANTIC_KIND::ERROR) : Symbol->Type;
    }
    Symbol->TypeChecking = TRUE;
    AST_NODE *Node       = Symbol->Node;
    if (Symbol->Kind == ANALYZER_SYMBOL_KIND::NAMESPACE)
    {
        Symbol->Type = SemanticType(Analyzer, SEMANTIC_KIND::NAMESPACE, 0, FALSE, NULLPTR, 0, FALSE, Symbol);
    }
    else if (Symbol->Kind == ANALYZER_SYMBOL_KIND::TYPE)
    {
        Symbol->Type = SemanticType(Analyzer, Node->Type == AST_NODE_TYPE::ENUM_DECLARATION ? SEMANTIC_KIND::ENUM : SEMANTIC_KIND::RECORD, Node->Type == AST_NODE_TYPE::ENUM_DECLARATION ? 32 : 0, FALSE, NULLPTR, 0, FALSE, Symbol);
    }
    else if (Symbol->Kind == ANALYZER_SYMBOL_KIND::FUNCTION)
    {
        BOOLEAN Method                  = Node->Type == AST_NODE_TYPE::METHOD_DECLARATION;
        Symbol->Type                    = SemanticType(Analyzer, SEMANTIC_KIND::FUNCTION, 0, FALSE, NULLPTR, 0, FALSE, Symbol);
        Symbol->Type->Return            = SemanticDeclared(Analyzer, Symbol->Scope, Method ? Node->MethodDeclaration.ReturnType : Node->FunctionDeclaration.ReturnType);
        CONST AST_NODE_LIST &Parameters = Method ? Node->MethodDeclaration.Parameters : Node->FunctionDeclaration.Parameters;
        for (SIZE Index = 0; Index < Parameters.Count; Index++)
        {
            Symbol->Type->Parameters.push_back(SemanticDeclared(Analyzer, Symbol->Members, Parameters.Items[Index]->Parameter.ParameterType));
        }
    }
    else if (Symbol->Kind == ANALYZER_SYMBOL_KIND::ENUM_MEMBER)
    {
        Symbol->Type = SemanticSymbolType(Analyzer, Symbol->Scope->Owner);
    }
    else
    {
        AST_NODE *Declared = Symbol->Kind == ANALYZER_SYMBOL_KIND::PARAMETER ? Node->Parameter.ParameterType : Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD ? Node->FieldDeclaration.FieldType : Node->VariableDeclaration.VariableType;
        Symbol->Type       = SemanticDeclared(Analyzer, Symbol->Scope, Declared);
        BOOLEAN Global     = Symbol->Scope->Owner == NULLPTR || Symbol->Scope->Owner->Kind == ANALYZER_SYMBOL_KIND::NAMESPACE;
        if (Symbol->Kind != ANALYZER_SYMBOL_KIND::PARAMETER && (Global || Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD))
        {
            AST_NODE *Initializer = Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD ? Node->FieldDeclaration.Initializer : Node->VariableDeclaration.Initializer;
            if (Initializer != NULLPTR)
            {
                SEMANTIC_CONTEXT Context;
                Context.Analyzer     = Analyzer;
                Context.Unit         = Symbol->Unit;
                Context.Scope        = Symbol->Scope;
                SEMANTIC_VALUE Value = SemanticExpression(&Context, Initializer, Symbol->Type);
                Value                = SemanticConvert(&Context, Value, Symbol->Type, Initializer);
                if (SemanticHasAuto(Value.Type))
                {
                    SemanticReport(&Context, Initializer, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::CANNOT_INFER_TYPE, Symbol->Name);
                    Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                }
                Symbol->Type        = Value.Type;
                Symbol->Value       = Value;
                Symbol->Initialized = TRUE;
            }
        }
    }
    if (Symbol->Type == NULLPTR)
    {
        Symbol->Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
    }
    Symbol->TypeChecking = FALSE;
    Symbol->TypeChecked  = TRUE;
    return Symbol->Type;
}

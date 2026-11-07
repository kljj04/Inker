// Inker/Source/StaticAnalyzer/Name.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>

STATIC BOOLEAN AnalyzerAccessible(ANALYZER_SYMBOL *Symbol, ANALYZER_SCOPE *Scope)
{
    if (Symbol->Access == TOKENS::TOKEN_KW_PUBLIC || Symbol->Access == TOKENS::TOKEN_UNKNOWN)
    {
        return TRUE;
    }

    if (Symbol->Unit != Scope->Unit)
    {
        return FALSE;
    }

    ANALYZER_SYMBOL *Owner = Symbol->Scope == NULLPTR ? NULLPTR : Symbol->Scope->Owner;

    if (Owner == NULLPTR || Owner->Kind != ANALYZER_SYMBOL_KIND::TYPE)
    {
        return TRUE;
    }

    while (Scope != NULLPTR)
    {
        if (Scope->Owner == Owner)
        {
            return TRUE;
        }

        Scope = Scope->Parent;
    }

    return FALSE;
}

STATIC ANALYZER_SYMBOL *AnalyzerTypeName(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node)
{
    if (Node == NULLPTR)
    {
        return NULLPTR;
    }

    while (Node->Type == AST_NODE_TYPE::POINTER_TYPE || Node->Type == AST_NODE_TYPE::ARRAY_TYPE)
    {
        Node = Node->Type == AST_NODE_TYPE::POINTER_TYPE ? Node->PointerType.BaseType : Node->ArrayType.ElementType;

        if (Node == NULLPTR)
        {
            return NULLPTR;
        }
    }

    if (Node->Type != AST_NODE_TYPE::TYPE || Node->TypeInfo.Token != TOKENS::TOKEN_IDENTIFIER)
    {
        return NULLPTR;
    }

    STRING           Name   = AnalyzerNormalizePath(Node->TypeInfo.Name);
    SIZE             Dot    = Name.find('.');
    ANALYZER_SYMBOL *Symbol = AnalyzerUnalias(AnalyzerFind(Scope, Name.substr(0, Dot)));

    while (Symbol != NULLPTR && Dot != STRING::npos)
    {
        SIZE   Next = Name.find('.', Dot + 1);
        STRING Part = Name.substr(Dot + 1, Next == STRING::npos ? STRING::npos : Next - Dot - 1);
        Symbol      = Symbol->Members == NULLPTR ? NULLPTR : AnalyzerQualified(Symbol->Members, Part);
        Dot         = Next;
    }

    if (Symbol == NULLPTR)
    {
        AnalyzerReport(Analyzer, Scope->Unit, Node->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL, Name);
    }
    else if (Symbol->Kind != ANALYZER_SYMBOL_KIND::TYPE && Symbol->Kind != ANALYZER_SYMBOL_KIND::ERROR)
    {
        AnalyzerReport(Analyzer, Scope->Unit, Node->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_TYPE, Name);
        Symbol = NULLPTR;
    }
    else if (!AnalyzerAccessible(Symbol, Scope))
    {
        AnalyzerReport(Analyzer, Scope->Unit, Node->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL, Name);
        Symbol = NULLPTR;
    }

    Analyzer->Bindings[Node] = Symbol;
    return Symbol;
}

STATIC ANALYZER_SYMBOL *AnalyzerExpressionName(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node, SIZE Depth, BOOLEAN Read = TRUE)
{
    if (Node == NULLPTR)
    {
        return NULLPTR;
    }

    if (Depth > 256)
    {
        AnalyzerReport(Analyzer, Scope->Unit, Node->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        return NULLPTR;
    }

    ANALYZER_SYMBOL *Symbol     = NULLPTR;
    auto             Expression = [&](AST_NODE *Child) { return AnalyzerExpressionName(Analyzer, Scope, Child, Depth + 1); };

    switch (Node->Type)
    {
        case AST_NODE_TYPE::IDENTIFIER:
        {
            Symbol = AnalyzerFind(Scope, Node->Identifier.Name);

            if (Symbol == NULLPTR)
            {
                AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Node->Identifier.Name), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL, Node->Identifier.Name);
            }
            else if (Symbol->Kind == ANALYZER_SYMBOL_KIND::VARIABLE && Symbol->Unit == Scope->Unit && Node->Location.Start < Symbol->Location.Start)
            {
                AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Node->Identifier.Name), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::USE_BEFORE_DECLARATION, Symbol->Name);
                AnalyzerReport(Analyzer, Symbol->Unit, Symbol->Location, DIAGNOSTIC_LEVEL::NOTE, DIAGNOSTIC_MESSAGE::DECLARED_HERE, Symbol->Name);
                Symbol = NULLPTR;
            }
            else
            {
                Symbol->Used = Symbol->Used || Read;
                Symbol       = AnalyzerUnalias(Symbol);

                if (Symbol != NULLPTR)
                {
                    Symbol->Used = Symbol->Used || Read;
                }
            }

            break;
        }
        case AST_NODE_TYPE::MEMBER_ACCESS:
        {
            ANALYZER_SYMBOL *Object  = Expression(Node->MemberAccess.Object);
            ANALYZER_SCOPE  *Members = Object == NULLPTR ? NULLPTR : Object->Members;

            if (Object != NULLPTR && Object->Node != NULLPTR && (Object->Kind == ANALYZER_SYMBOL_KIND::VARIABLE || Object->Kind == ANALYZER_SYMBOL_KIND::FIELD || Object->Kind == ANALYZER_SYMBOL_KIND::PARAMETER))
            {
                AST_NODE *Type = Object->Kind == ANALYZER_SYMBOL_KIND::PARAMETER ? Object->Node->Parameter.ParameterType : Object->Kind == ANALYZER_SYMBOL_KIND::FIELD ? Object->Node->FieldDeclaration.FieldType : Object->Node->VariableDeclaration.VariableType;

                while (Type != NULLPTR && (Type->Type == AST_NODE_TYPE::POINTER_TYPE || Type->Type == AST_NODE_TYPE::ARRAY_TYPE))
                {
                    Type = Type->Type == AST_NODE_TYPE::POINTER_TYPE ? Type->PointerType.BaseType : Type->ArrayType.ElementType;
                }

                auto             It    = Analyzer->Bindings.find(Type);
                ANALYZER_SYMBOL *Class = It == Analyzer->Bindings.end() ? AnalyzerTypeName(Analyzer, Object->Scope, Type) : It->second;
                Members                = Class == NULLPTR ? NULLPTR : Class->Members;
            }

            if (Members != NULLPTR)
            {
                auto It = Members->Symbols.find(STRING(Node->MemberAccess.Member));

                if (It == Members->Symbols.end())
                {
                    AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Node->MemberAccess.Member), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL, Node->MemberAccess.Member);
                }
                else if (!AnalyzerAccessible(It->second, Scope))
                {
                    AnalyzerReport(Analyzer, Scope->Unit, AnalyzerLocation(Scope->Unit, Node, Node->MemberAccess.Member), DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL, Node->MemberAccess.Member);
                }
                else
                {
                    It->second->Used = TRUE;
                    Symbol           = AnalyzerUnalias(It->second);

                    if (Symbol != NULLPTR)
                    {
                        Symbol->Used = Symbol->Used || Read;
                    }
                }
            }

            break;
        }
        case AST_NODE_TYPE::FUNCTION_CALL:
        {
            ANALYZER_SYMBOL *Callee = Expression(Node->FunctionCall.Callee);

            for (SIZE Index = 0; Index < Node->FunctionCall.Arguments.Count; Index++)
            {
                Expression(Node->FunctionCall.Arguments.Items[Index]);
            }

            if (Callee != NULLPTR && Callee->Kind == ANALYZER_SYMBOL_KIND::FUNCTION && Callee->Node != NULLPTR)
            {
                AST_NODE *Declaration = Callee->Node;
                SIZE      Count       = Declaration->Type == AST_NODE_TYPE::METHOD_DECLARATION ? Declaration->MethodDeclaration.Parameters.Count : Declaration->FunctionDeclaration.Parameters.Count;

                if (Count != Node->FunctionCall.Arguments.Count)
                {
                    AnalyzerReport(Analyzer, Scope->Unit, Node->Location, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ARGUMENT_COUNT_MISMATCH, std::to_string(Count), std::to_string(Node->FunctionCall.Arguments.Count));
                }
            }

            break;
        }
        case AST_NODE_TYPE::ARGUMENT:
            Expression(Node->Argument.Value);
            break;
        case AST_NODE_TYPE::ARRAY:
            for (SIZE Index = 0; Index < Node->Array.Elements.Count; Index++)
            {
                Expression(Node->Array.Elements.Items[Index]);
            }
            break;
        case AST_NODE_TYPE::ARRAY_ACCESS:
            Expression(Node->ArrayAccess.Array);
            Expression(Node->ArrayAccess.Index);
            break;
        case AST_NODE_TYPE::UNARY:
            Expression(Node->Unary.Operand);
            break;
        case AST_NODE_TYPE::BINARY:
            Expression(Node->Binary.Left);
            Expression(Node->Binary.Right);
            break;
        case AST_NODE_TYPE::ASSIGNMENT:
            AnalyzerExpressionName(Analyzer, Scope, Node->Assignment.Target, Depth + 1, Node->Assignment.Operator != TOKENS::TOKEN_OP_ASSIGN);
            Expression(Node->Assignment.Value);
            break;
        case AST_NODE_TYPE::TERNARY:
            Expression(Node->Ternary.Condition);
            Expression(Node->Ternary.ThenExpression);
            Expression(Node->Ternary.ElseExpression);
            break;
        case AST_NODE_TYPE::STRING_INTERPOLATION:
            for (SIZE Index = 0; Index < Node->StringInterpolation.Segments.Count; Index++)
            {
                Expression(Node->StringInterpolation.Segments.Items[Index]);
            }
            break;
        case AST_NODE_TYPE::INTERPOLATION_EXPRESSION:
            Expression(Node->InterpolationExpression.Expression);
            break;
        default:
            break;
    }

    Analyzer->Bindings[Node] = Symbol;

    return Symbol;
}

STATIC VOID AnalyzerStatementNames(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node, SIZE Depth)
{
    if (Node == NULLPTR || Depth > 256)
    {
        return;
    }

    auto            It         = Analyzer->NodeScopes.find(Node);
    ANALYZER_SCOPE *Own        = It == Analyzer->NodeScopes.end() ? Scope : It->second;
    auto            Expression = [&](AST_NODE *Child) { AnalyzerExpressionName(Analyzer, Scope, Child, 0); };
    auto            Statement  = [&](AST_NODE *Child) { AnalyzerStatementNames(Analyzer, Own, Child, Depth + 1); };
    auto            List       = [&](CONST AST_NODE_LIST &Items)
    {
        for (SIZE Index = 0; Index < Items.Count; Index++)
        {
            Statement(Items.Items[Index]);
        }
    };

    switch (Node->Type)
    {
        case AST_NODE_TYPE::PROGRAM:
            List(Node->Program);
            break;
        case AST_NODE_TYPE::BLOCK:
            List(Node->Block.Statements);
            break;
        case AST_NODE_TYPE::NAMESPACE_DECLARATION:
            List(Node->NamespaceDeclaration.Declarations);
            break;
        case AST_NODE_TYPE::CLASS_DECLARATION:
            List(Node->ClassDeclaration.Members);
            break;
        case AST_NODE_TYPE::STRUCT_DECLARATION:
            List(Node->StructDeclaration.Members);
            break;
        case AST_NODE_TYPE::ENUM_DECLARATION:
            List(Node->EnumDeclaration.Members);
            break;
        case AST_NODE_TYPE::FUNCTION_DECLARATION:
            AnalyzerTypeName(Analyzer, Scope, Node->FunctionDeclaration.ReturnType);
            List(Node->FunctionDeclaration.Parameters);
            Statement(Node->FunctionDeclaration.Body);
            break;
        case AST_NODE_TYPE::METHOD_DECLARATION:
            AnalyzerTypeName(Analyzer, Scope, Node->MethodDeclaration.ReturnType);
            List(Node->MethodDeclaration.Parameters);
            Statement(Node->MethodDeclaration.Body);
            break;
        case AST_NODE_TYPE::PARAMETER:
            AnalyzerTypeName(Analyzer, Scope, Node->Parameter.ParameterType);
            break;
        case AST_NODE_TYPE::VARIABLE_DECLARATION:
            AnalyzerTypeName(Analyzer, Scope, Node->VariableDeclaration.VariableType);
            Expression(Node->VariableDeclaration.Initializer);
            break;
        case AST_NODE_TYPE::FIELD_DECLARATION:
            AnalyzerTypeName(Analyzer, Scope, Node->FieldDeclaration.FieldType);
            Expression(Node->FieldDeclaration.Initializer);
            break;
        case AST_NODE_TYPE::EXPRESSION_STATEMENT:
            Expression(Node->ExpressionStatement.Expression);
            break;
        case AST_NODE_TYPE::RETURN:
            Expression(Node->Return.Value);
            break;
        case AST_NODE_TYPE::ENUM_MEMBER:
            Expression(Node->EnumMember.Value);
            break;
        case AST_NODE_TYPE::IF:
            Expression(Node->If.Condition);
            Statement(Node->If.ThenBranch);
            Statement(Node->If.ElseBranch);
            break;
        case AST_NODE_TYPE::WHILE:
            Expression(Node->While.Condition);
            Statement(Node->While.Body);
            break;
        case AST_NODE_TYPE::FOR:
            Statement(Node->For.Initializer);
            AnalyzerExpressionName(Analyzer, Own, Node->For.Condition, 0);
            AnalyzerExpressionName(Analyzer, Own, Node->For.Increment, 0);
            Statement(Node->For.Body);
            break;
        default:
            break;
    }
}

VOID AnalyzerResolveNames(STATIC_ANALYZER *Analyzer)
{
    for (CONST auto &Unit : Analyzer->Units)
    {
        if (Unit->Valid)
        {
            AnalyzerStatementNames(Analyzer, Unit->Global, Unit->Root, 0);
        }
    }
}

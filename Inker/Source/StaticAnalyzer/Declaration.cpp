// Inker/Source/StaticAnalyzer/Declaration.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>

STATIC VOID AnalyzerCollectList(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, CONST AST_NODE_LIST &List, BOOLEAN Members);

STATIC VOID AnalyzerCollectStatement(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, AST_NODE *Node)
{
    if (Node == NULLPTR)
    {
        return;
    }

    Analyzer->NodeScopes[Node] = Scope;

    switch (Node->Type)
    {
        case AST_NODE_TYPE::BLOCK:
        {
            ANALYZER_SCOPE *Block      = AnalyzerNewScope(Analyzer, Scope->Unit, Scope);
            Analyzer->NodeScopes[Node] = Block;
            AnalyzerCollectList(Analyzer, Block, Node->Block.Statements, FALSE);
            break;
        }
        case AST_NODE_TYPE::IF:
            AnalyzerCollectStatement(Analyzer, Scope, Node->If.ThenBranch);
            AnalyzerCollectStatement(Analyzer, Scope, Node->If.ElseBranch);
            break;
        case AST_NODE_TYPE::WHILE:
            AnalyzerCollectStatement(Analyzer, Scope, Node->While.Body);
            break;
        case AST_NODE_TYPE::FOR:
        {
            ANALYZER_SCOPE *Loop       = AnalyzerNewScope(Analyzer, Scope->Unit, Scope);
            Analyzer->NodeScopes[Node] = Loop;

            if (Node->For.Initializer != NULLPTR)
            {
                AST_NODE *Initializer             = Node->For.Initializer;
                Analyzer->NodeScopes[Initializer] = Loop;
                AnalyzerDeclare(Analyzer, Loop, ANALYZER_SYMBOL_KIND::VARIABLE, Initializer->VariableDeclaration.Name, Initializer, TOKENS::TOKEN_KW_PRIVATE);
            }

            AnalyzerCollectStatement(Analyzer, Loop, Node->For.Body);
            break;
        }
        default:
            break;
    }
}

STATIC VOID AnalyzerCollectList(STATIC_ANALYZER *Analyzer, ANALYZER_SCOPE *Scope, CONST AST_NODE_LIST &List, BOOLEAN Members)
{
    TOKENS  DefaultAccess = Members && Scope->Owner != NULLPTR && Scope->Owner->Node->Type == AST_NODE_TYPE::STRUCT_DECLARATION ? TOKENS::TOKEN_KW_PUBLIC : TOKENS::TOKEN_KW_PRIVATE;
    TOKENS  Access        = DefaultAccess;
    BOOLEAN Explicit      = FALSE;

    for (SIZE Index = 0; Index < List.Count; Index++)
    {
        AST_NODE *Node             = List.Items[Index];
        Analyzer->NodeScopes[Node] = Scope;

        if (Node->Type == AST_NODE_TYPE::ACCESS_MODIFIER)
        {
            Access   = Node->AccessModifier.Modifier;
            Explicit = TRUE;
            continue;
        }

        ANALYZER_SYMBOL *Symbol = NULLPTR;

        switch (Node->Type)
        {
            case AST_NODE_TYPE::NAMESPACE_DECLARATION:
            {
                STRING          Path    = AnalyzerNormalizePath(Node->NamespaceDeclaration.Name);
                ANALYZER_SCOPE *Current = Scope;

                while (!Path.empty())
                {
                    SIZE   Dot  = Path.find('.');
                    STRING Name = Path.substr(0, Dot);
                    Symbol      = AnalyzerDeclare(Analyzer, Current, ANALYZER_SYMBOL_KIND::NAMESPACE, Name, Node, Explicit && Dot == STRING::npos ? Access : TOKENS::TOKEN_KW_PUBLIC);

                    if (Symbol == NULLPTR)
                    {
                        break;
                    }

                    if (Symbol->Members == NULLPTR)
                    {
                        Symbol->Members = AnalyzerNewScope(Analyzer, Scope->Unit, Current, Symbol);
                    }

                    Current = Symbol->Members;

                    if (Dot == STRING::npos)
                    {
                        Analyzer->NodeScopes[Node] = Current;
                        AnalyzerCollectList(Analyzer, Current, Node->NamespaceDeclaration.Declarations, FALSE);
                        break;
                    }

                    Path.erase(0, Dot + 1);
                }

                break;
            }
            case AST_NODE_TYPE::CLASS_DECLARATION:
            case AST_NODE_TYPE::STRUCT_DECLARATION:
            case AST_NODE_TYPE::ENUM_DECLARATION:
            {
                STRING_VIEW Name = Node->Type == AST_NODE_TYPE::CLASS_DECLARATION ? Node->ClassDeclaration.Name : Node->Type == AST_NODE_TYPE::STRUCT_DECLARATION ? Node->StructDeclaration.Name : Node->EnumDeclaration.Name;
                Symbol           = AnalyzerDeclare(Analyzer, Scope, ANALYZER_SYMBOL_KIND::TYPE, Name, Node, Access);

                if (Symbol != NULLPTR)
                {
                    Symbol->Members            = AnalyzerNewScope(Analyzer, Scope->Unit, Scope, Symbol);
                    Analyzer->NodeScopes[Node] = Symbol->Members;

                    if (Node->Type != AST_NODE_TYPE::ENUM_DECLARATION)
                    {
                        AnalyzerCollectList(Analyzer, Symbol->Members, Node->Type == AST_NODE_TYPE::CLASS_DECLARATION ? Node->ClassDeclaration.Members : Node->StructDeclaration.Members, TRUE);
                    }
                    else
                    {
                        for (SIZE MemberIndex = 0; MemberIndex < Node->EnumDeclaration.Members.Count; MemberIndex++)
                        {
                            AST_NODE *Member             = Node->EnumDeclaration.Members.Items[MemberIndex];
                            Analyzer->NodeScopes[Member] = Symbol->Members;
                            AnalyzerDeclare(Analyzer, Symbol->Members, ANALYZER_SYMBOL_KIND::ENUM_MEMBER, Member->EnumMember.Name, Member, TOKENS::TOKEN_KW_PUBLIC);
                        }
                    }
                }

                break;
            }
            case AST_NODE_TYPE::FUNCTION_DECLARATION:
            case AST_NODE_TYPE::METHOD_DECLARATION:
            {
                BOOLEAN     Method = Node->Type == AST_NODE_TYPE::METHOD_DECLARATION;
                STRING_VIEW Name   = Method ? Node->MethodDeclaration.Name : Node->FunctionDeclaration.Name;
                Symbol             = AnalyzerDeclare(Analyzer, Scope, ANALYZER_SYMBOL_KIND::FUNCTION, Name, Node, Access);

                if (Symbol != NULLPTR)
                {
                    Symbol->Members            = AnalyzerNewScope(Analyzer, Scope->Unit, Scope, Symbol);
                    AST_NODE_LIST &Parameters  = Method ? Node->MethodDeclaration.Parameters : Node->FunctionDeclaration.Parameters;
                    AST_NODE      *Body        = Method ? Node->MethodDeclaration.Body : Node->FunctionDeclaration.Body;
                    Analyzer->NodeScopes[Node] = Symbol->Members;

                    for (SIZE ParameterIndex = 0; ParameterIndex < Parameters.Count; ParameterIndex++)
                    {
                        AST_NODE *Parameter             = Parameters.Items[ParameterIndex];
                        Analyzer->NodeScopes[Parameter] = Symbol->Members;
                        ANALYZER_SYMBOL *Argument       = AnalyzerDeclare(Analyzer, Symbol->Members, ANALYZER_SYMBOL_KIND::PARAMETER, Parameter->Parameter.Name, Parameter, TOKENS::TOKEN_KW_PRIVATE);

                        if (Argument != NULLPTR)
                        {
                            Argument->Initialized = TRUE;
                        }
                    }

                    if (Body != NULLPTR)
                    {
                        Analyzer->NodeScopes[Body] = Symbol->Members;
                        AnalyzerCollectList(Analyzer, Symbol->Members, Body->Block.Statements, FALSE);
                    }
                }

                break;
            }
            case AST_NODE_TYPE::USE_DECLARATION:
                Analyzer->ImportAccess[Node] = Access;
                break;
            case AST_NODE_TYPE::VARIABLE_DECLARATION:
                AnalyzerDeclare(Analyzer, Scope, ANALYZER_SYMBOL_KIND::VARIABLE, Node->VariableDeclaration.Name, Node, Access);
                break;
            case AST_NODE_TYPE::FIELD_DECLARATION:
                AnalyzerDeclare(Analyzer, Scope, ANALYZER_SYMBOL_KIND::FIELD, Node->FieldDeclaration.Name, Node, Access);
                break;
            default:
                AnalyzerCollectStatement(Analyzer, Scope, Node);
                break;
        }

        Access   = Members ? TOKENS::TOKEN_UNKNOWN : TOKENS::TOKEN_KW_PRIVATE;
        Explicit = FALSE;
    }
}

VOID AnalyzerCollect(STATIC_ANALYZER *Analyzer)
{
    for (CONST auto &Unit : Analyzer->Units)
    {
        if (Unit->Valid)
        {
            AnalyzerCollectList(Analyzer, Unit->Global, Unit->Root->Program, FALSE);
        }
    }
}

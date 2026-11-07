// Inker/Source/StaticAnalyzer/Traversal.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/StaticAnalyzer.hpp>

STATIC VOID AnalyzerPush(std::vector<AST_NODE *> *Pending, AST_NODE *Node)
{
    if (Node != NULLPTR)
    {
        Pending->push_back(Node);
    }
}

STATIC VOID AnalyzerPushList(std::vector<AST_NODE *> *Pending, CONST AST_NODE_LIST &List)
{
    for (SIZE Index = List.Count; Index > 0; Index--)
    {
        AnalyzerPush(Pending, List.Items[Index - 1]);
    }
}

VOID AnalyzerWalk(AST_NODE *Root, CONST std::function<VOID(AST_NODE *)> &Visitor)
{
    std::vector<AST_NODE *> Pending;
    AnalyzerPush(&Pending, Root);

    while (!Pending.empty())
    {
        AST_NODE *Node = Pending.back();
        Pending.pop_back();
        Visitor(Node);

        switch (Node->Type)
        {
            case AST_NODE_TYPE::PROGRAM:
                AnalyzerPushList(&Pending, Node->Program);
                break;
            case AST_NODE_TYPE::ARRAY:
                AnalyzerPushList(&Pending, Node->Array.Elements);
                break;
            case AST_NODE_TYPE::BLOCK:
                AnalyzerPushList(&Pending, Node->Block.Statements);
                break;
            case AST_NODE_TYPE::NAMESPACE_DECLARATION:
                AnalyzerPushList(&Pending, Node->NamespaceDeclaration.Declarations);
                break;
            case AST_NODE_TYPE::CLASS_DECLARATION:
                AnalyzerPushList(&Pending, Node->ClassDeclaration.Members);
                break;
            case AST_NODE_TYPE::STRUCT_DECLARATION:
                AnalyzerPushList(&Pending, Node->StructDeclaration.Members);
                break;
            case AST_NODE_TYPE::ENUM_DECLARATION:
                AnalyzerPushList(&Pending, Node->EnumDeclaration.Members);
                AnalyzerPush(&Pending, Node->EnumDeclaration.UnderlyingType);
                break;
            case AST_NODE_TYPE::FUNCTION_DECLARATION:
                AnalyzerPush(&Pending, Node->FunctionDeclaration.Body);
                AnalyzerPush(&Pending, Node->FunctionDeclaration.ReturnType);
                AnalyzerPushList(&Pending, Node->FunctionDeclaration.Parameters);
                break;
            case AST_NODE_TYPE::METHOD_DECLARATION:
                AnalyzerPush(&Pending, Node->MethodDeclaration.Body);
                AnalyzerPush(&Pending, Node->MethodDeclaration.ReturnType);
                AnalyzerPushList(&Pending, Node->MethodDeclaration.Parameters);
                break;
            case AST_NODE_TYPE::LAMBDA:
                AnalyzerPush(&Pending, Node->Lambda.Body);
                AnalyzerPush(&Pending, Node->Lambda.ReturnType);
                AnalyzerPushList(&Pending, Node->Lambda.Parameters);
                break;
            case AST_NODE_TYPE::FUNCTION_TYPE:
                AnalyzerPush(&Pending, Node->FunctionType.ReturnType);
                AnalyzerPushList(&Pending, Node->FunctionType.Parameters);
                break;
            case AST_NODE_TYPE::PARAMETER:
                AnalyzerPush(&Pending, Node->Parameter.DefaultValue);
                AnalyzerPush(&Pending, Node->Parameter.ParameterType);
                break;
            case AST_NODE_TYPE::VARIABLE_DECLARATION:
                AnalyzerPush(&Pending, Node->VariableDeclaration.Initializer);
                AnalyzerPush(&Pending, Node->VariableDeclaration.VariableType);
                break;
            case AST_NODE_TYPE::FIELD_DECLARATION:
                AnalyzerPush(&Pending, Node->FieldDeclaration.Initializer);
                AnalyzerPush(&Pending, Node->FieldDeclaration.FieldType);
                break;
            case AST_NODE_TYPE::ARRAY_ACCESS:
                AnalyzerPush(&Pending, Node->ArrayAccess.Index);
                AnalyzerPush(&Pending, Node->ArrayAccess.Array);
                break;
            case AST_NODE_TYPE::MEMBER_ACCESS:
                AnalyzerPush(&Pending, Node->MemberAccess.Object);
                break;
            case AST_NODE_TYPE::UNARY:
                AnalyzerPush(&Pending, Node->Unary.Operand);
                break;
            case AST_NODE_TYPE::BINARY:
                AnalyzerPush(&Pending, Node->Binary.Right);
                AnalyzerPush(&Pending, Node->Binary.Left);
                break;
            case AST_NODE_TYPE::ASSIGNMENT:
                AnalyzerPush(&Pending, Node->Assignment.Value);
                AnalyzerPush(&Pending, Node->Assignment.Target);
                break;
            case AST_NODE_TYPE::TERNARY:
                AnalyzerPush(&Pending, Node->Ternary.ElseExpression);
                AnalyzerPush(&Pending, Node->Ternary.ThenExpression);
                AnalyzerPush(&Pending, Node->Ternary.Condition);
                break;
            case AST_NODE_TYPE::FUNCTION_CALL:
                AnalyzerPushList(&Pending, Node->FunctionCall.Arguments);
                AnalyzerPush(&Pending, Node->FunctionCall.Callee);
                break;
            case AST_NODE_TYPE::ARGUMENT:
                AnalyzerPush(&Pending, Node->Argument.Value);
                break;
            case AST_NODE_TYPE::EXPRESSION_STATEMENT:
                AnalyzerPush(&Pending, Node->ExpressionStatement.Expression);
                break;
            case AST_NODE_TYPE::IF:
                AnalyzerPush(&Pending, Node->If.ElseBranch);
                AnalyzerPush(&Pending, Node->If.ThenBranch);
                AnalyzerPush(&Pending, Node->If.Condition);
                break;
            case AST_NODE_TYPE::WHILE:
                AnalyzerPush(&Pending, Node->While.Body);
                AnalyzerPush(&Pending, Node->While.Condition);
                break;
            case AST_NODE_TYPE::FOR:
                AnalyzerPush(&Pending, Node->For.Body);
                AnalyzerPush(&Pending, Node->For.Increment);
                AnalyzerPush(&Pending, Node->For.Condition);
                AnalyzerPush(&Pending, Node->For.Initializer);
                break;
            case AST_NODE_TYPE::RETURN:
                AnalyzerPush(&Pending, Node->Return.Value);
                break;
            case AST_NODE_TYPE::ENUM_MEMBER:
                AnalyzerPush(&Pending, Node->EnumMember.Value);
                break;
            case AST_NODE_TYPE::POINTER_TYPE:
                AnalyzerPush(&Pending, Node->PointerType.BaseType);
                break;
            case AST_NODE_TYPE::ARRAY_TYPE:
                AnalyzerPush(&Pending, Node->ArrayType.Size);
                AnalyzerPush(&Pending, Node->ArrayType.ElementType);
                break;
            case AST_NODE_TYPE::STRING_INTERPOLATION:
                AnalyzerPushList(&Pending, Node->StringInterpolation.Segments);
                break;
            case AST_NODE_TYPE::INTERPOLATION_EXPRESSION:
                AnalyzerPush(&Pending, Node->InterpolationExpression.Expression);
                break;
            default:
                break;
        }
    }
}

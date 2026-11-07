// Inker/Source/StaticAnalyzer/Flow.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>
#include <algorithm>
#include <iterator>

STATIC CONSTEXPR UINT32 FLOW_NORMAL   = 1;
STATIC CONSTEXPR UINT32 FLOW_RETURN   = 2;
STATIC CONSTEXPR UINT32 FLOW_BREAK    = 4;
STATIC CONSTEXPR UINT32 FLOW_CONTINUE = 8;

STATIC VOID SemanticCheckCondition(SEMANTIC_CONTEXT *Context, AST_NODE *Node)
{
    SEMANTIC_VALUE Value = SemanticExpression(Context, Node);
    if (!SemanticNumeric(Value.Type) && Value.Type->Kind != SEMANTIC_KIND::BOOLEAN && Value.Type->Kind != SEMANTIC_KIND::AUTO && Value.Type->Kind != SEMANTIC_KIND::ERROR && Value.Type->Kind != SEMANTIC_KIND::POINTER && Value.Type->Kind != SEMANTIC_KIND::NULL_VALUE)
    {
        SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, "BOOLEAN or numeric condition", SemanticName(Value.Type));
    }
}

STATIC VOID SemanticArraySizes(SEMANTIC_CONTEXT *Context, AST_NODE *Node)
{
    while (Node != NULLPTR && (Node->Type == AST_NODE_TYPE::ARRAY_TYPE || Node->Type == AST_NODE_TYPE::POINTER_TYPE))
    {
        if (Node->Type == AST_NODE_TYPE::ARRAY_TYPE)
        {
            if (Node->ArrayType.Size != NULLPTR)
            {
                SEMANTIC_VALUE Size = SemanticExpression(Context, Node->ArrayType.Size);
                if (!SemanticIntegral(Size.Type) && Size.Type->Kind != SEMANTIC_KIND::AUTO && Size.Type->Kind != SEMANTIC_KIND::ERROR)
                {
                    SemanticReport(Context, Node->ArrayType.Size, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, "integer array size", SemanticName(Size.Type));
                }
                if (Size.IntegerKnown && (Size.Integer == 0 || (!Size.Type->Unsigned && SemanticSigned(Size.Integer, Size.Type->Bits) < 0)))
                {
                    SemanticReport(Context, Node->ArrayType.Size, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_ARRAY_SIZE);
                }
            }
            Node = Node->ArrayType.ElementType;
        }
        else
        {
            Node = Node->PointerType.BaseType;
        }
    }
}

STATIC VOID SemanticVariable(SEMANTIC_CONTEXT *Context, AST_NODE *Node)
{
    auto Binding = Context->Analyzer->Declarations.find(Node);
    if (Binding == Context->Analyzer->Declarations.end())
    {
        return;
    }
    ANALYZER_SYMBOL *Symbol      = Binding->second;
    BOOLEAN          Field       = Node->Type == AST_NODE_TYPE::FIELD_DECLARATION;
    AST_NODE        *Declared    = Field ? Node->FieldDeclaration.FieldType : Node->VariableDeclaration.VariableType;
    AST_NODE        *Initializer = Field ? Node->FieldDeclaration.Initializer : Node->VariableDeclaration.Initializer;
    SEMANTIC_TYPE   *Type        = SemanticDeclared(Context->Analyzer, Symbol->Scope, Declared);
    SemanticArraySizes(Context, Declared);
    Context->Locals[Symbol] = Type;
    if (Initializer != NULLPTR)
    {
        SEMANTIC_VALUE Value = SemanticExpression(Context, Initializer, Type);
        Value                = SemanticConvert(Context, Value, Type, Initializer);
        Type                 = Value.Type;
        if (SemanticHasAuto(Type) && !Context->DefinitionOnly)
        {
            SemanticReport(Context, Initializer, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::CANNOT_INFER_TYPE, Symbol->Name);
            Type = SemanticType(Context->Analyzer, SEMANTIC_KIND::ERROR);
        }
        if (Type->Kind == SEMANTIC_KIND::VOID)
        {
            SemanticReport(Context, Initializer, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE, "a variable type");
        }
        Context->Locals[Symbol] = Type;
        Context->Initialized.insert(Symbol);
        Context->Known[Symbol] = Value;
    }
    else if (Type->Kind == SEMANTIC_KIND::RECORD)
    {
        Context->Initialized.insert(Symbol);
    }
    if (Context->Variant != NULLPTR)
    {
        Context->Variant->Locals[Symbol]  = Type;
        Context->Variant->NodeTypes[Node] = Type;
    }
    else
    {
        Context->Analyzer->ExpressionTypes[Node] = Type;
    }
}

STATIC VOID SemanticListReturns(SEMANTIC_CONTEXT *Context, CONST SEMANTIC_CONTEXT &Branch, SIZE Before)
{
    Context->Returns.insert(Context->Returns.end(), Branch.Returns.begin() + static_cast<SSIZE>(Before), Branch.Returns.end());
}

UINT32 SemanticStatement(SEMANTIC_CONTEXT *Context, AST_NODE *Node)
{
    if (Node == NULLPTR)
    {
        return FLOW_NORMAL;
    }
    ANALYZER_SCOPE *PreviousScope = Context->Scope;
    auto            Scope         = Context->Analyzer->NodeScopes.find(Node);
    if (Scope != Context->Analyzer->NodeScopes.end())
    {
        Context->Scope = Scope->second;
    }
    UINT32 Flow = FLOW_NORMAL;
    switch (Node->Type)
    {
        case AST_NODE_TYPE::BLOCK:
        case AST_NODE_TYPE::PROGRAM:
        case AST_NODE_TYPE::NAMESPACE_DECLARATION:
        {
            CONST AST_NODE_LIST &List = Node->Type == AST_NODE_TYPE::BLOCK ? Node->Block.Statements : Node->Type == AST_NODE_TYPE::NAMESPACE_DECLARATION ? Node->NamespaceDeclaration.Declarations : Node->Program;
            for (SIZE Index = 0; Index < List.Count; Index++)
            {
                if ((Flow & FLOW_NORMAL) == 0)
                {
                    SemanticReport(Context, List.Items[Index], DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::UNREACHABLE_CODE);
                    SEMANTIC_CONTEXT Unreachable = *Context;
                    Unreachable.Unreachable      = TRUE;
                    SemanticStatement(&Unreachable, List.Items[Index]);
                }
                else
                {
                    Flow = (Flow & ~FLOW_NORMAL) | SemanticStatement(Context, List.Items[Index]);
                }
            }
            break;
        }
        case AST_NODE_TYPE::VARIABLE_DECLARATION:
        case AST_NODE_TYPE::FIELD_DECLARATION:
            SemanticVariable(Context, Node);
            break;
        case AST_NODE_TYPE::EXPRESSION_STATEMENT:
            SemanticExpression(Context, Node->ExpressionStatement.Expression);
            break;
        case AST_NODE_TYPE::RETURN:
        {
            SEMANTIC_TYPE *Return = Context->Function == NULLPTR ? SemanticType(Context->Analyzer, SEMANTIC_KIND::VOID) : SemanticSymbolType(Context->Analyzer, Context->Function)->Return;
            SEMANTIC_VALUE Value  = SemanticExpression(Context, Node->Return.Value, Return);
            if (Node->Return.Value == NULLPTR && Return->Kind != SEMANTIC_KIND::VOID && Return->Kind != SEMANTIC_KIND::AUTO)
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::MISSING_RETURN);
            }
            else
            {
                Value = SemanticConvert(Context, Value, Return, Node->Return.Value == NULLPTR ? Node : Node->Return.Value);
            }
            Context->Returns.push_back(Value);
            if (Context->Variant != NULLPTR && Return->Kind == SEMANTIC_KIND::AUTO && Value.Type->Kind != SEMANTIC_KIND::AUTO && Value.Type->Kind != SEMANTIC_KIND::ERROR && Context->Variant->Return == NULLPTR)
            {
                Context->Variant->Return = Value.Type;
            }
            Flow = FLOW_RETURN;
            break;
        }
        case AST_NODE_TYPE::IF:
        {
            SemanticCheckCondition(Context, Node->If.Condition);
            SEMANTIC_CONTEXT Then     = *Context;
            SEMANTIC_CONTEXT Else     = *Context;
            SIZE             Before   = Context->Returns.size();
            UINT32           ThenFlow = SemanticStatement(&Then, Node->If.ThenBranch);
            UINT32           ElseFlow = SemanticStatement(&Else, Node->If.ElseBranch);
            SemanticListReturns(Context, Then, Before);
            SemanticListReturns(Context, Else, Before);
            if ((ThenFlow & FLOW_NORMAL) != 0 && (ElseFlow & FLOW_NORMAL) != 0)
            {
                std::set<ANALYZER_SYMBOL *> Common;
                std::set_intersection(Then.Initialized.begin(), Then.Initialized.end(), Else.Initialized.begin(), Else.Initialized.end(), std::inserter(Common, Common.end()));
                Context->Initialized = std::move(Common);
                std::set<STRING> Paths;
                std::set_intersection(Then.InitializedPaths.begin(), Then.InitializedPaths.end(), Else.InitializedPaths.begin(), Else.InitializedPaths.end(), std::inserter(Paths, Paths.end()));
                Context->InitializedPaths = std::move(Paths);
            }
            else if ((ThenFlow & FLOW_NORMAL) != 0)
            {
                Context->Initialized      = std::move(Then.Initialized);
                Context->InitializedPaths = std::move(Then.InitializedPaths);
            }
            else if ((ElseFlow & FLOW_NORMAL) != 0)
            {
                Context->Initialized      = std::move(Else.Initialized);
                Context->InitializedPaths = std::move(Else.InitializedPaths);
            }
            Context->Known.clear();
            Flow = ThenFlow | ElseFlow;
            break;
        }
        case AST_NODE_TYPE::WHILE:
        {
            SemanticCheckCondition(Context, Node->While.Condition);
            SEMANTIC_CONTEXT Body = *Context;
            Body.Known.clear();
            SIZE   Before   = Context->Returns.size();
            UINT32 BodyFlow = SemanticStatement(&Body, Node->While.Body);
            SemanticListReturns(Context, Body, Before);
            Context->Known.clear();
            BOOLEAN Always = Node->While.Condition->Type == AST_NODE_TYPE::BOOLEAN && Node->While.Condition->Boolean.Value;
            Flow           = (BodyFlow & FLOW_RETURN) | ((!Always || (BodyFlow & FLOW_BREAK) != 0) ? FLOW_NORMAL : 0);
            break;
        }
        case AST_NODE_TYPE::FOR:
        {
            SemanticStatement(Context, Node->For.Initializer);
            auto           Symbol    = Context->Analyzer->Declarations.find(Node->For.Initializer);
            SEMANTIC_TYPE *IndexType = Symbol == Context->Analyzer->Declarations.end() ? SemanticType(Context->Analyzer, SEMANTIC_KIND::ERROR) : Context->Locals[Symbol->second];
            if (!SemanticNumeric(IndexType) && IndexType->Kind != SEMANTIC_KIND::ERROR && IndexType->Kind != SEMANTIC_KIND::AUTO)
            {
                SemanticReport(Context, Node->For.Initializer, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR);
            }
            SEMANTIC_VALUE End = SemanticExpression(Context, Node->For.Condition, IndexType);
            SemanticConvert(Context, End, IndexType, Node->For.Condition);
            SEMANTIC_VALUE Step = SemanticExpression(Context, Node->For.Increment, IndexType);
            Step                = SemanticConvert(Context, Step, IndexType, Node->For.Increment);
            if ((Step.IntegerKnown && Step.Integer == 0) || (Step.FloatKnown && Step.Float == 0))
            {
                SemanticReport(Context, Node->For.Increment, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ZERO_RANGE_STEP);
            }
            SEMANTIC_CONTEXT Body = *Context;
            Body.Known.clear();
            SIZE   Before   = Context->Returns.size();
            UINT32 BodyFlow = SemanticStatement(&Body, Node->For.Body);
            SemanticListReturns(Context, Body, Before);
            Context->Known.clear();
            Flow = FLOW_NORMAL | (BodyFlow & FLOW_RETURN);
            break;
        }
        case AST_NODE_TYPE::BREAK:
            Flow = FLOW_BREAK;
            break;
        case AST_NODE_TYPE::CONTINUE:
            Flow = FLOW_CONTINUE;
            break;
        default:
            break;
    }
    Context->Scope = PreviousScope;
    return Flow;
}

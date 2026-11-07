// Inker/Source/StaticAnalyzer/Expression.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <StaticAnalyzer/Semantics.hpp>
#include <algorithm>
#include <limits>

STATIC BOOLEAN SemanticTruthType(CONST SEMANTIC_TYPE *Type)
{
    return SemanticNumeric(Type) || Type->Kind == SEMANTIC_KIND::BOOLEAN || Type->Kind == SEMANTIC_KIND::POINTER || Type->Kind == SEMANTIC_KIND::NULL_VALUE || Type->Kind == SEMANTIC_KIND::AUTO || Type->Kind == SEMANTIC_KIND::ERROR;
}

STATIC VOID SemanticCondition(SEMANTIC_CONTEXT *Context, CONST SEMANTIC_VALUE &Value, AST_NODE *Site)
{
    if (!SemanticTruthType(Value.Type))
    {
        SemanticReport(Context, Site, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, "BOOLEAN or numeric condition", SemanticName(Value.Type));
    }
}

STATIC BOOLEAN SemanticAccessible(ANALYZER_SYMBOL *Symbol, SEMANTIC_CONTEXT *Context)
{
    if (Symbol->Access == TOKENS::TOKEN_KW_PUBLIC)
    {
        return TRUE;
    }
    if (Symbol->Unit != Context->Unit)
    {
        return FALSE;
    }
    ANALYZER_SYMBOL *Owner = Symbol->Scope == NULLPTR ? NULLPTR : Symbol->Scope->Owner;
    if (Owner == NULLPTR || Owner->Kind != ANALYZER_SYMBOL_KIND::TYPE)
    {
        return TRUE;
    }
    ANALYZER_SCOPE *Scope = Context->Scope;
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

STATIC SEMANTIC_VALUE SemanticReference(SEMANTIC_CONTEXT *Context, ANALYZER_SYMBOL *Symbol, AST_NODE *Node, BOOLEAN Read)
{
    SEMANTIC_VALUE Value;
    Value.Type = SemanticType(Context->Analyzer, SEMANTIC_KIND::ERROR);
    Value.Node = Node;
    Symbol     = AnalyzerUnalias(Symbol);
    if (Symbol == NULLPTR || Symbol->Kind == ANALYZER_SYMBOL_KIND::ERROR)
    {
        return Value;
    }
    Value.Symbol     = Symbol;
    auto Local       = Context->Locals.find(Symbol);
    Value.Type       = Local == Context->Locals.end() ? SemanticSymbolType(Context->Analyzer, Symbol) : Local->second;
    Value.LValue     = Symbol->Kind == ANALYZER_SYMBOL_KIND::VARIABLE || Symbol->Kind == ANALYZER_SYMBOL_KIND::PARAMETER || Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD;
    Value.AccessPath = std::to_string(reinterpret_cast<UINTN>(Symbol));
    if (Symbol->Kind == ANALYZER_SYMBOL_KIND::TYPE)
    {
        Value.Type = SemanticType(Context->Analyzer, SEMANTIC_KIND::TYPE_OBJECT, 0, FALSE, NULLPTR, 0, FALSE, Symbol);
    }
    if (Read)
    {
        Symbol->Used                = TRUE;
        BOOLEAN Scalar              = SemanticNumeric(Value.Type) || Value.Type->Kind == SEMANTIC_KIND::BOOLEAN || Value.Type->Kind == SEMANTIC_KIND::POINTER || Value.Type->Kind == SEMANTIC_KIND::ENUM;
        BOOLEAN LocalInitialized    = Context->Initialized.contains(Symbol);
        BOOLEAN ExternalInitialized = Local == Context->Locals.end() && (Symbol->Initialized || (Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD && Symbol->Node->FieldDeclaration.Initializer != NULLPTR));
        if (Value.LValue && Scalar && !LocalInitialized && !ExternalInitialized && !Context->Unreachable && !Context->CheckingType)
        {
            SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::UNINITIALIZED_VALUE, Symbol->Name);
        }
        auto Known = Context->Known.find(Symbol);
        if (Known != Context->Known.end())
        {
            Value.IntegerKnown = Known->second.IntegerKnown;
            Value.Integer      = Known->second.Integer;
            Value.FloatKnown   = Known->second.FloatKnown;
            Value.Float        = Known->second.Float;
            Value.TextKnown    = Known->second.TextKnown;
            Value.Text         = Known->second.Text;
        }
        else if (((Context->Function == NULLPTR && !Context->CheckingType) || Symbol->Kind == ANALYZER_SYMBOL_KIND::ENUM_MEMBER) && Symbol->Value.Type != NULLPTR)
        {
            Value.IntegerKnown = Symbol->Value.IntegerKnown;
            Value.Integer      = Symbol->Value.Integer;
            Value.FloatKnown   = Symbol->Value.FloatKnown;
            Value.Float        = Symbol->Value.Float;
        }
    }
    return Value;
}

STATIC TOKENS SemanticCompound(TOKENS Operator)
{
    switch (Operator)
    {
        case TOKENS::TOKEN_OP_PLUS_ASSIGN:
            return TOKENS::TOKEN_OP_PLUS;
        case TOKENS::TOKEN_OP_MINUS_ASSIGN:
            return TOKENS::TOKEN_OP_MINUS;
        case TOKENS::TOKEN_OP_MULTIPLY_ASSIGN:
            return TOKENS::TOKEN_OP_MULTIPLY;
        case TOKENS::TOKEN_OP_DIVIDE_ASSIGN:
            return TOKENS::TOKEN_OP_DIVIDE;
        case TOKENS::TOKEN_OP_MODULO_ASSIGN:
            return TOKENS::TOKEN_OP_MODULO;
        case TOKENS::TOKEN_OP_BIT_AND_ASSIGN:
            return TOKENS::TOKEN_OP_BIT_AND;
        case TOKENS::TOKEN_OP_BIT_OR_ASSIGN:
            return TOKENS::TOKEN_OP_BIT_OR;
        case TOKENS::TOKEN_OP_BIT_XOR_ASSIGN:
            return TOKENS::TOKEN_OP_BIT_XOR;
        case TOKENS::TOKEN_OP_SHL_ASSIGN:
            return TOKENS::TOKEN_OP_SHL;
        default:
            return TOKENS::TOKEN_OP_SHR;
    }
}

SEMANTIC_VALUE SemanticExpression(SEMANTIC_CONTEXT *Context, AST_NODE *Node, SEMANTIC_TYPE *Expected, BOOLEAN Read)
{
    SEMANTIC_VALUE Value;
    Value.Type = SemanticType(Context->Analyzer, SEMANTIC_KIND::ERROR);
    Value.Node = Node;
    if (Node == NULLPTR)
    {
        Value.Type = SemanticType(Context->Analyzer, SEMANTIC_KIND::VOID);
        return Value;
    }
    if (Context->Depth >= 256)
    {
        SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        return Value;
    }
    Context->Depth++;
    STATIC_ANALYZER *Analyzer = Context->Analyzer;
    switch (Node->Type)
    {
        case AST_NODE_TYPE::INTEGER:
            Value.Type         = SemanticType(Analyzer, SEMANTIC_KIND::INTEGER, Node->Integer.Value >= std::numeric_limits<INT32>::min() && Node->Integer.Value <= std::numeric_limits<INT32>::max() ? 32 : 64);
            Value.IntegerKnown = TRUE;
            Value.Integer      = static_cast<UINT64>(Node->Integer.Value) & SemanticMask(Value.Type->Bits);
            Value.Literal      = TRUE;
            break;
        case AST_NODE_TYPE::FLOAT:
            Value.Type       = SemanticType(Analyzer, SEMANTIC_KIND::FLOAT, 64);
            Value.FloatKnown = TRUE;
            Value.Float      = Node->Float.Value;
            Value.Literal    = TRUE;
            break;
        case AST_NODE_TYPE::STRING:
            Value.Type      = SemanticType(Analyzer, SEMANTIC_KIND::STRING);
            Value.TextKnown = TRUE;
            Value.Text      = Node->String.Value;
            Value.Literal   = TRUE;
            break;
        case AST_NODE_TYPE::BOOLEAN:
            Value.Type         = SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE);
            Value.IntegerKnown = TRUE;
            Value.Integer      = Node->Boolean.Value;
            Value.Literal      = TRUE;
            break;
        case AST_NODE_TYPE::NULL_LITERAL:
            Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::NULL_VALUE);
            break;
        case AST_NODE_TYPE::IDENTIFIER:
        {
            auto             Binding = Analyzer->Bindings.find(Node);
            ANALYZER_SYMBOL *Symbol  = Binding == Analyzer->Bindings.end() ? AnalyzerFind(Context->Scope, Node->Identifier.Name) : Binding->second;
            Value                    = SemanticReference(Context, Symbol, Node, Read);
            break;
        }
        case AST_NODE_TYPE::MEMBER_ACCESS:
        {
            SEMANTIC_VALUE  Object  = SemanticExpression(Context, Node->MemberAccess.Object, NULLPTR, Read);
            ANALYZER_SCOPE *Members = Object.Type->Owner == NULLPTR ? NULLPTR : Object.Type->Owner->Members;
            if (Object.Type->Kind == SEMANTIC_KIND::AUTO && Context->DefinitionOnly)
            {
                Value.Type = Object.Type;
                break;
            }
            if (Object.Type->Kind == SEMANTIC_KIND::ERROR)
            {
                break;
            }
            if (Members == NULLPTR || (Object.Type->Kind != SEMANTIC_KIND::NAMESPACE && Object.Type->Kind != SEMANTIC_KIND::TYPE_OBJECT && Object.Type->Kind != SEMANTIC_KIND::RECORD))
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_MEMBER_TYPE, SemanticName(Object.Type));
                break;
            }
            auto Member = Members->Symbols.find(STRING(Node->MemberAccess.Member));
            if (Member == Members->Symbols.end())
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL, Node->MemberAccess.Member);
                break;
            }
            ANALYZER_SYMBOL *Symbol = AnalyzerUnalias(Member->second);
            if (!SemanticAccessible(Member->second, Context))
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL, Node->MemberAccess.Member);
                break;
            }
            if (Object.Type->Kind == SEMANTIC_KIND::TYPE_OBJECT && Symbol != NULLPTR && Symbol->Kind == ANALYZER_SYMBOL_KIND::FIELD)
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INSTANCE_MEMBER, Symbol->Name);
                break;
            }
            Value = SemanticReference(Context, Symbol, Node, Read);
            if (Object.Type->Kind == SEMANTIC_KIND::RECORD)
            {
                Value.LValue = Object.LValue;
            }
            Value.AccessPath = Object.AccessPath + "." + STRING(Node->MemberAccess.Member);
            break;
        }
        case AST_NODE_TYPE::ARRAY:
        {
            SEMANTIC_TYPE *ElementExpected = Expected != NULLPTR && Expected->Kind == SEMANTIC_KIND::ARRAY ? Expected->Element : Expected != NULLPTR && Expected->Kind == SEMANTIC_KIND::AUTO ? Expected : NULLPTR;
            SEMANTIC_TYPE *Element         = NULLPTR;
            for (SIZE Index = 0; Index < Node->Array.Elements.Count; Index++)
            {
                SEMANTIC_VALUE Item = SemanticExpression(Context, Node->Array.Elements.Items[Index], ElementExpected);
                if (ElementExpected != NULLPTR)
                {
                    Item = SemanticConvert(Context, Item, ElementExpected, Item.Node);
                }
                if (Element == NULLPTR)
                {
                    Element = Item.Type;
                }
                else if (!SemanticCompatible(Element, Item.Type))
                {
                    if (SemanticNumeric(Element) && SemanticNumeric(Item.Type))
                    {
                        SemanticConvert(Context, Item, Element, Item.Node);
                    }
                    else
                    {
                        SemanticReport(Context, Item.Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, SemanticName(Element), SemanticName(Item.Type));
                    }
                }
            }
            if (Element == NULLPTR)
            {
                Element = ElementExpected;
                if (Element == NULLPTR || SemanticHasAuto(Element))
                {
                    SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::CANNOT_INFER_TYPE, "empty array");
                    Element = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                }
            }
            SIZE Count = Node->Array.Elements.Count;
            if (Expected != NULLPTR && Expected->Kind == SEMANTIC_KIND::ARRAY && Expected->Sized)
            {
                if (Count > Expected->Count)
                {
                    SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ARRAY_CAPACITY, std::to_string(Count), std::to_string(Expected->Count));
                }
                Count = Expected->Count;
            }
            Value.Type    = SemanticType(Analyzer, SEMANTIC_KIND::ARRAY, 0, FALSE, Element, Count, TRUE);
            Value.Literal = TRUE;
            break;
        }
        case AST_NODE_TYPE::ARRAY_ACCESS:
        {
            SEMANTIC_VALUE Array = SemanticExpression(Context, Node->ArrayAccess.Array, NULLPTR, FALSE);
            SEMANTIC_VALUE Index = SemanticExpression(Context, Node->ArrayAccess.Index);
            if (Array.Type->Kind == SEMANTIC_KIND::AUTO && Context->DefinitionOnly)
            {
                Value.Type = Array.Type;
                break;
            }
            if (Array.Type->Kind == SEMANTIC_KIND::ERROR || Index.Type->Kind == SEMANTIC_KIND::ERROR)
            {
                break;
            }
            if (Array.Type->Kind != SEMANTIC_KIND::ARRAY && Array.Type->Kind != SEMANTIC_KIND::POINTER && Array.Type->Kind != SEMANTIC_KIND::STRING)
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_OPERAND, "indexing");
                break;
            }
            if (!SemanticIntegral(Index.Type) && Index.Type->Kind != SEMANTIC_KIND::AUTO)
            {
                SemanticReport(Context, Node->ArrayAccess.Index, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, "integer index", SemanticName(Index.Type));
            }
            Value.Type       = Array.Type->Kind == SEMANTIC_KIND::STRING ? SemanticType(Analyzer, SEMANTIC_KIND::CHARACTER, 8, TRUE) : Array.Type->Element;
            Value.LValue     = Array.Type->Kind != SEMANTIC_KIND::STRING;
            Value.Symbol     = Array.Symbol;
            Value.AccessPath = Array.AccessPath + (Index.IntegerKnown ? "[" + std::to_string(Index.Integer) + "]" : "[?]");
            if (Array.Type->Sized && Index.IntegerKnown && ((Index.Type->Unsigned ? Index.Integer : static_cast<UINT64>(SemanticSigned(Index.Integer, Index.Type->Bits))) >= Array.Type->Count))
            {
                SemanticReport(Context, Node->ArrayAccess.Index, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::ARRAY_INDEX_OUT_OF_RANGE, Index.Type->Unsigned ? std::to_string(Index.Integer) : std::to_string(SemanticSigned(Index.Integer, Index.Type->Bits)), std::to_string(Array.Type->Count));
            }
            if (Read && Array.Symbol != NULLPTR && !Context->Initialized.contains(Array.Symbol) && !Array.Symbol->Initialized && !Context->InitializedPaths.contains(Value.AccessPath) && !Context->Unreachable && !Context->CheckingType)
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::WARNING, DIAGNOSTIC_MESSAGE::UNINITIALIZED_VALUE, Array.Symbol->Name);
            }
            break;
        }
        case AST_NODE_TYPE::ASSIGNMENT:
        {
            SEMANTIC_VALUE Target = SemanticExpression(Context, Node->Assignment.Target, NULLPTR, Node->Assignment.Operator != TOKENS::TOKEN_OP_ASSIGN);
            SEMANTIC_VALUE Source = SemanticExpression(Context, Node->Assignment.Value, Target.Type);
            if (!Target.LValue && Target.Type->Kind != SEMANTIC_KIND::ERROR && Target.Type->Kind != SEMANTIC_KIND::AUTO)
            {
                SemanticReport(Context, Node->Assignment.Target, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_ASSIGNMENT_TARGET);
            }
            if (Node->Assignment.Operator != TOKENS::TOKEN_OP_ASSIGN)
            {
                Source = SemanticBinary(Context, SemanticCompound(Node->Assignment.Operator), Target, Source, Node);
            }
            Value            = SemanticConvert(Context, Source, Target.Type, Node->Assignment.Value);
            Value.Node       = Node;
            Value.Symbol     = Target.Symbol;
            Value.LValue     = Target.LValue;
            Value.AccessPath = Target.AccessPath;
            if (Target.Symbol != NULLPTR)
            {
                if (Node->Assignment.Target->Type == AST_NODE_TYPE::IDENTIFIER)
                {
                    Context->Initialized.insert(Target.Symbol);
                    Context->Known[Target.Symbol] = Value;
                }
                else if (!Target.AccessPath.ends_with("[?]"))
                {
                    Context->InitializedPaths.insert(Target.AccessPath);
                }
            }
            break;
        }
        case AST_NODE_TYPE::BINARY:
        {
            SEMANTIC_VALUE Left = SemanticExpression(Context, Node->Binary.Left);
            if (Node->Binary.Operator == TOKENS::TOKEN_OP_AND || Node->Binary.Operator == TOKENS::TOKEN_OP_OR)
            {
                SemanticCondition(Context, Left, Node->Binary.Left);
                SEMANTIC_CONTEXT RightContext = *Context;
                BOOLEAN          Known        = Left.IntegerKnown || Left.FloatKnown;
                BOOLEAN          Truth        = Left.IntegerKnown ? Left.Integer != 0 : Left.Float != 0;
                BOOLEAN          Skip         = Known && (Node->Binary.Operator == TOKENS::TOKEN_OP_AND ? !Truth : Truth);
                RightContext.Unreachable      = Context->Unreachable || Skip;
                SEMANTIC_VALUE Right          = SemanticExpression(&RightContext, Node->Binary.Right);
                SemanticCondition(Context, Right, Node->Binary.Right);
                if (!Skip && Known)
                {
                    Context->Initialized = std::move(RightContext.Initialized);
                    Context->Known       = std::move(RightContext.Known);
                }
                else if (!Known)
                {
                    Context->Known.clear();
                }
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE);
                if (Skip)
                {
                    Value.IntegerKnown = TRUE;
                    Value.Integer      = Truth;
                }
                else if (Known && (Right.IntegerKnown || Right.FloatKnown))
                {
                    Value.IntegerKnown = TRUE;
                    Value.Integer      = Right.IntegerKnown ? Right.Integer != 0 : Right.Float != 0;
                }
            }
            else
            {
                SEMANTIC_VALUE Right = SemanticExpression(Context, Node->Binary.Right);
                Value                = SemanticBinary(Context, Node->Binary.Operator, Left, Right, Node);
            }
            break;
        }
        case AST_NODE_TYPE::TERNARY:
        {
            SEMANTIC_VALUE Condition = SemanticExpression(Context, Node->Ternary.Condition);
            SemanticCondition(Context, Condition, Node->Ternary.Condition);
            SEMANTIC_CONTEXT ThenContext = *Context;
            SEMANTIC_CONTEXT ElseContext = *Context;
            SEMANTIC_VALUE   Then        = SemanticExpression(&ThenContext, Node->Ternary.ThenExpression, Expected);
            SEMANTIC_VALUE   Else        = SemanticExpression(&ElseContext, Node->Ternary.ElseExpression, Expected);
            if (SemanticNumeric(Then.Type) && SemanticNumeric(Else.Type))
            {
                Else = SemanticConvert(Context, Else, Then.Type, Else.Node);
            }
            else if (!SemanticCompatible(Then.Type, Else.Type))
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::TYPE_MISMATCH, SemanticName(Then.Type), SemanticName(Else.Type));
            }
            Value.Type = Then.Type;
            Context->Known.clear();
            break;
        }
        case AST_NODE_TYPE::UNARY:
        {
            TOKENS         Operator = Node->Unary.Operator;
            SEMANTIC_VALUE Operand  = SemanticExpression(Context, Node->Unary.Operand, NULLPTR, Operator != TOKENS::TOKEN_OP_BIT_AND);
            Value                   = Operand;
            Value.Node              = Node;
            if (Operator == TOKENS::TOKEN_OP_BIT_AND)
            {
                if (!Operand.LValue && Operand.Type->Kind != SEMANTIC_KIND::ERROR)
                {
                    SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_OPERAND, "&");
                }
                Value.Type         = SemanticType(Analyzer, SEMANTIC_KIND::POINTER, 64, TRUE, Operand.Type);
                Value.IntegerKnown = FALSE;
                Value.FloatKnown   = FALSE;
                Value.LValue       = FALSE;
                Context->Known.clear();
            }
            else if (Operator == TOKENS::TOKEN_OP_MULTIPLY)
            {
                if (Operand.Type->Kind != SEMANTIC_KIND::POINTER || Operand.Type->Element->Kind == SEMANTIC_KIND::VOID)
                {
                    SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_OPERAND, "*");
                    Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
                }
                else
                {
                    Value.Type         = Operand.Type->Element;
                    Value.LValue       = TRUE;
                    Value.IntegerKnown = FALSE;
                    Value.FloatKnown   = FALSE;
                }
            }
            else if (Operator == TOKENS::TOKEN_OP_NOT)
            {
                SemanticCondition(Context, Operand, Node);
                Value.Type         = SemanticType(Analyzer, SEMANTIC_KIND::BOOLEAN, 1, TRUE);
                Value.IntegerKnown = Operand.IntegerKnown || Operand.FloatKnown;
                Value.Integer      = Operand.IntegerKnown ? Operand.Integer == 0 : Operand.Float == 0;
                Value.FloatKnown   = FALSE;
                Value.LValue       = FALSE;
            }
            else if (Operand.Type->Kind == SEMANTIC_KIND::AUTO && Context->DefinitionOnly)
            {
                break;
            }
            else if (!SemanticNumeric(Operand.Type) || (Operator == TOKENS::TOKEN_OP_BIT_NOT && !SemanticIntegral(Operand.Type)))
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_OPERAND, "unary operator");
                Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::ERROR);
            }
            else if (Operator == TOKENS::TOKEN_OP_INCREMENT || Operator == TOKENS::TOKEN_OP_DECREMENT)
            {
                if (!Operand.LValue)
                {
                    SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::INVALID_INCREMENT_TARGET, Operator == TOKENS::TOKEN_OP_INCREMENT ? "++" : "--");
                }
                SEMANTIC_VALUE One;
                One.Type               = Operand.Type;
                One.IntegerKnown       = SemanticIntegral(Operand.Type);
                One.Integer            = 1;
                One.FloatKnown         = Operand.Type->Kind == SEMANTIC_KIND::FLOAT;
                One.Float              = 1;
                SEMANTIC_VALUE Changed = SemanticBinary(Context, Operator == TOKENS::TOKEN_OP_INCREMENT ? TOKENS::TOKEN_OP_PLUS : TOKENS::TOKEN_OP_MINUS, Operand, One, Node);
                Changed                = SemanticConvert(Context, Changed, Operand.Type, Node);
                if (Operand.Symbol != NULLPTR)
                {
                    Context->Initialized.insert(Operand.Symbol);
                    Context->Known[Operand.Symbol] = Changed;
                }
                Value        = Node->Unary.Postfix ? Operand : Changed;
                Value.Node   = Node;
                Value.LValue = FALSE;
            }
            else
            {
                if (Operator == TOKENS::TOKEN_OP_MINUS)
                {
                    if (Value.FloatKnown)
                    {
                        Value.Float = -Value.Float;
                    }
                    if (Value.IntegerKnown)
                    {
                        Value.Integer = (UINT64(0) - Value.Integer) & SemanticMask(Value.Type->Bits);
                    }
                }
                else if (Operator == TOKENS::TOKEN_OP_BIT_NOT && Value.IntegerKnown)
                {
                    Value.Integer = (~Value.Integer) & SemanticMask(Value.Type->Bits);
                }
                Value.LValue = FALSE;
            }
            break;
        }
        case AST_NODE_TYPE::FUNCTION_CALL:
        {
            SEMANTIC_VALUE              Callee = SemanticExpression(Context, Node->FunctionCall.Callee);
            std::vector<SEMANTIC_VALUE> Arguments;
            SEMANTIC_TYPE              *Signature = Callee.Type;
            for (SIZE Index = 0; Index < Node->FunctionCall.Arguments.Count; Index++)
            {
                AST_NODE      *Argument = Node->FunctionCall.Arguments.Items[Index];
                SEMANTIC_TYPE *Wanted   = Signature->Kind == SEMANTIC_KIND::FUNCTION && Index < Signature->Parameters.size() ? Signature->Parameters[Index] : NULLPTR;
                Arguments.push_back(SemanticExpression(Context, Argument->Argument.Value, Wanted));
            }
            if (Callee.Type->Kind == SEMANTIC_KIND::FUNCTION)
            {
                Value.Type = SemanticCall(Context, Callee.Type->Owner, Arguments, Node);
            }
            else if (Callee.Type->Kind == SEMANTIC_KIND::AUTO && Context->DefinitionOnly)
            {
                Value.Type = Callee.Type;
            }
            else if (Callee.Type->Kind != SEMANTIC_KIND::ERROR)
            {
                SemanticReport(Context, Node, DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::NOT_CALLABLE, SemanticName(Callee.Type));
            }
            Context->Known.clear();
            break;
        }
        case AST_NODE_TYPE::ARGUMENT:
            Value = SemanticExpression(Context, Node->Argument.Value, Expected);
            break;
        case AST_NODE_TYPE::STRING_INTERPOLATION:
            Value.Type = SemanticType(Analyzer, SEMANTIC_KIND::STRING);
            for (SIZE Index = 0; Index < Node->StringInterpolation.Segments.Count; Index++)
            {
                SEMANTIC_VALUE Segment = SemanticExpression(Context, Node->StringInterpolation.Segments.Items[Index]);
                if (Segment.Type->Kind == SEMANTIC_KIND::VOID)
                {
                    SemanticReport(Context, Node->StringInterpolation.Segments.Items[Index], DIAGNOSTIC_LEVEL::ERROR, DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE, "an interpolated value");
                }
            }
            break;
        case AST_NODE_TYPE::INTERPOLATION_EXPRESSION:
            Value = SemanticExpression(Context, Node->InterpolationExpression.Expression);
            break;
        default:
            break;
    }
    Value.Node = Node;
    if (Expected != NULLPTR && (Node->Type == AST_NODE_TYPE::INTEGER || Node->Type == AST_NODE_TYPE::FLOAT || Node->Type == AST_NODE_TYPE::STRING))
    {
        Value = SemanticConvert(Context, Value, Expected, Node);
    }
    if (Context->Variant != NULLPTR)
    {
        Context->Variant->NodeTypes[Node] = Value.Type;
    }
    else
    {
        Analyzer->ExpressionTypes[Node] = Value.Type;
    }
    Context->Depth--;
    return Value;
}

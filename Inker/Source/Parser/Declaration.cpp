// Inker/Source/Parser/Declaration.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Parser/Parser.hpp>

BOOLEAN ParserIsDeclaration(CONST PARSER *Parser)
{
    TOKENS Token = ParserPeek(Parser);

    if (Token == TOKENS::TOKEN_KW_SET || Token == TOKENS::TOKEN_KW_IMPORT || ParserIsType(Token))
    {
        return TRUE;
    }

    if (Token != TOKENS::TOKEN_IDENTIFIER)
    {
        return FALSE;
    }

    SIZE Offset = 1;

    while (ParserPeek(Parser, Offset) == TOKENS::TOKEN_SP_DOT && ParserPeek(Parser, Offset + 1) == TOKENS::TOKEN_IDENTIFIER)
    {
        Offset += 2;
    }

    if (ParserPeek(Parser, Offset) == TOKENS::TOKEN_OP_MULTIPLY && Offset == 1)
    {
        STRING_VIEW Name  = ParserText(Parser);
        BOOLEAN     Known = FALSE;

        for (STRING_VIEW TypeName : Parser->TypeNames)
        {
            if (Name == TypeName)
            {
                Known = TRUE;
                break;
            }
        }

        if (!Known)
        {
            return FALSE;
        }
    }

    while (ParserPeek(Parser, Offset) == TOKENS::TOKEN_OP_MULTIPLY)
    {
        Offset++;
    }

    if (ParserPeek(Parser, Offset) == TOKENS::TOKEN_IDENTIFIER)
    {
        return TRUE;
    }

    return FALSE;
}

STATIC STRING_VIEW ParserPath(PARSER *Parser)
{
    AST_LOC Start = ParserLocation(Parser);
    ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "identifier");

    while (ParserMatch(Parser, TOKENS::TOKEN_SP_DOT))
    {
        ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "identifier");
    }

    return STRING_VIEW(Parser->Lexer->Source).substr(Start.Start, ParserFinish(Parser, Start).Length);
}

STATIC AST_NODE *ParserComposite(PARSER *Parser, AST_LOC Start)
{
    TOKENS Kind = ParserPeek(Parser);
    ParserAdvance(Parser);
    STRING_VIEW Name = Kind == TOKENS::TOKEN_KW_NAMESPACE ? ParserPath(Parser) : ParserText(Parser);

    if (Kind != TOKENS::TOKEN_KW_NAMESPACE)
    {
        ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "identifier");
    }

    AST_NODE_TYPE Type = AST_NODE_TYPE::NAMESPACE_DECLARATION;

    if (Kind == TOKENS::TOKEN_KW_CLASS)
    {
        Type = AST_NODE_TYPE::CLASS_DECLARATION;
    }

    if (Kind == TOKENS::TOKEN_KW_STRUCT)
    {
        Type = AST_NODE_TYPE::STRUCT_DECLARATION;
    }

    if (Kind == TOKENS::TOKEN_KW_ENUM)
    {
        Type = AST_NODE_TYPE::ENUM_DECLARATION;
    }

    AST_NODE      *Node = ParserNode(Parser, Type, Start);
    AST_NODE_LIST *Members;

    if (Type == AST_NODE_TYPE::NAMESPACE_DECLARATION)
    {
        Node->NamespaceDeclaration.Name = Name;
        Members                         = &Node->NamespaceDeclaration.Declarations;
    }
    else if (Type == AST_NODE_TYPE::CLASS_DECLARATION)
    {
        Node->ClassDeclaration.Name = Name;
        Members                     = &Node->ClassDeclaration.Members;
    }
    else if (Type == AST_NODE_TYPE::STRUCT_DECLARATION)
    {
        Node->StructDeclaration.Name = Name;
        Members                      = &Node->StructDeclaration.Members;
    }
    else
    {
        Node->EnumDeclaration.Name = Name;
        Members                    = &Node->EnumDeclaration.Members;
    }

    ParserConsume(Parser, TOKENS::TOKEN_SP_LBRACE, "{");

    if (Kind == TOKENS::TOKEN_KW_ENUM)
    {
        while (ParserPeek(Parser) != TOKENS::TOKEN_SP_RBRACE && ParserPeek(Parser) != TOKENS::TOKEN_EOF)
        {
            SIZE Position = Parser->Position;

            try
            {
                AST_LOC     MemberStart = ParserLocation(Parser);
                STRING_VIEW MemberName  = ParserText(Parser);
                ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "enum member");
                AST_NODE *Value    = ParserMatch(Parser, TOKENS::TOKEN_OP_ASSIGN) ? ParserExpression(Parser, 2) : NULLPTR;
                AST_NODE *Member   = ParserNode(Parser, AST_NODE_TYPE::ENUM_MEMBER, ParserFinish(Parser, MemberStart));
                Member->EnumMember = {MemberName, Value};

                if (ParserPeek(Parser) != TOKENS::TOKEN_SP_RBRACE)
                {
                    ParserConsume(Parser, TOKENS::TOKEN_SP_COMMA, ",");
                }

                ParserAppend(Parser, Members, Member);
            }
            catch (CONST PARSER_FAILURE &)
            {
                while (ParserPeek(Parser) != TOKENS::TOKEN_EOF && ParserPeek(Parser) != TOKENS::TOKEN_SP_RBRACE && ParserPeek(Parser) != TOKENS::TOKEN_SP_COMMA)
                {
                    ParserAdvance(Parser);
                }

                ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA);

                if (Parser->Position == Position && ParserPeek(Parser) != TOKENS::TOKEN_SP_RBRACE)
                {
                    ParserAdvance(Parser);
                }
            }
        }
    }
    else
    {
        ParserSequence(Parser, Members, TRUE, Kind != TOKENS::TOKEN_KW_NAMESPACE);
    }

    ParserConsume(Parser, TOKENS::TOKEN_SP_RBRACE, "}");
    ParserMatch(Parser, TOKENS::TOKEN_SP_SEMICOLON);
    Node->Location = ParserFinish(Parser, Start);

    return Node;
}

STATIC VOID ParserParameters(PARSER *Parser, AST_NODE_LIST *Parameters)
{
    ParserConsume(Parser, TOKENS::TOKEN_SP_LPAREN, "(");

    if (ParserPeek(Parser) == TOKENS::TOKEN_TYPE_VOID && ParserPeek(Parser, 1) == TOKENS::TOKEN_SP_RPAREN)
    {
        ParserAdvance(Parser);
    }
    else if (ParserPeek(Parser) != TOKENS::TOKEN_SP_RPAREN)
    {
        do
        {
            AST_LOC   Start = ParserLocation(Parser);
            AST_NODE *Type  = ParserType(Parser);

            if (Type->Type == AST_NODE_TYPE::TYPE && Type->TypeInfo.Token == TOKENS::TOKEN_TYPE_VOID)
            {
                ParserFailAt(Parser, Type->Location, DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE, "a named parameter");
            }

            STRING_VIEW Name = ParserText(Parser);
            ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "parameter name");
            Type = ParserArrayDeclarator(Parser, Type);

            if (ParserPeek(Parser) == TOKENS::TOKEN_OP_ASSIGN)
            {
                ParserFail(Parser, DIAGNOSTIC_MESSAGE::UNSUPPORTED_FEATURE, "default parameter values");
            }

            AST_NODE *Parameter  = ParserNode(Parser, AST_NODE_TYPE::PARAMETER, ParserFinish(Parser, Start));
            Parameter->Parameter = {Name, Type, NULLPTR};
            ParserAppend(Parser, Parameters, Parameter);
        } while (ParserMatch(Parser, TOKENS::TOKEN_SP_COMMA));
    }

    ParserConsume(Parser, TOKENS::TOKEN_SP_RPAREN, ")");
}

AST_NODE *ParserDeclaration(PARSER *Parser, BOOLEAN Member, BOOLEAN Terminator)
{
    PARSER_DEPTH Guard(Parser);
    AST_LOC      Start = ParserLocation(Parser);

    if (ParserMatch(Parser, TOKENS::TOKEN_KW_IMPORT))
    {
        if (Member || !Terminator)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_DECLARATION);
        }

        AST_LOC FileLocation = ParserLocation(Parser);

        if (ParserPeek(Parser) != TOKENS::TOKEN_LIT_STRING)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_TOKEN, "quoted file path");
        }

        AST_NODE *Literal = ParserString(Parser);

        if (Literal->Type != AST_NODE_TYPE::STRING || Literal->String.Value.empty() || Literal->String.Value.find('\0') != STRING_VIEW::npos)
        {
            ParserFailAt(Parser, FileLocation, DIAGNOSTIC_MESSAGE::INVALID_ARGUMENT, "import path must be a nonempty plain string");
        }

        STRING_VIEW File = Literal->String.Value;
        STRING_VIEW Path;
        STRING_VIEW Alias;

        if (ParserMatch(Parser, TOKENS::TOKEN_KW_USE))
        {
            Path = ParserPath(Parser);
        }

        if (ParserMatch(Parser, TOKENS::TOKEN_KW_AS))
        {
            Alias = ParserText(Parser);
            ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "alias");
        }

        ParserConsume(Parser, TOKENS::TOKEN_SP_SEMICOLON, ";", "import declaration");
        AST_NODE *Node       = ParserNode(Parser, AST_NODE_TYPE::USE_DECLARATION, ParserFinish(Parser, Start));
        Node->UseDeclaration = {Path, Alias, File};

        return Node;
    }

    BOOLEAN Inferred = ParserMatch(Parser, TOKENS::TOKEN_KW_SET);
    TOKENS  Kind     = ParserPeek(Parser);

    if (Inferred && (Kind == TOKENS::TOKEN_KW_NAMESPACE || Kind == TOKENS::TOKEN_KW_STRUCT || Kind == TOKENS::TOKEN_KW_CLASS || Kind == TOKENS::TOKEN_KW_ENUM))
    {
        if (!Terminator)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_DECLARATION);
        }

        return ParserComposite(Parser, Start);
    }

    AST_NODE *Type;

    if (Inferred)
    {
        Type           = ParserNode(Parser, AST_NODE_TYPE::TYPE, Start);
        Type->TypeInfo = {TOKENS::TOKEN_TYPE_AUTO32, STRING_VIEW(Parser->Lexer->Source).substr(Start.Start, Start.Length)};
    }
    else
    {
        Type = ParserType(Parser);
    }

    STRING_VIEW Name = ParserText(Parser);
    ParserConsume(Parser, TOKENS::TOKEN_IDENTIFIER, "declaration name");

    if (!Inferred && ParserPeek(Parser) == TOKENS::TOKEN_SP_LPAREN)
    {
        if (!Terminator)
        {
            ParserFail(Parser, DIAGNOSTIC_MESSAGE::INVALID_DECLARATION);
        }

        AST_NODE      *Node       = ParserNode(Parser, Member ? AST_NODE_TYPE::METHOD_DECLARATION : AST_NODE_TYPE::FUNCTION_DECLARATION, Start);
        AST_NODE_LIST *Parameters = Member ? &Node->MethodDeclaration.Parameters : &Node->FunctionDeclaration.Parameters;
        ParserParameters(Parser, Parameters);
        SIZE OldLoops     = Parser->LoopDepth;
        Parser->LoopDepth = 0;
        Parser->FunctionDepth++;
        AST_NODE *Body;

        try
        {
            Body = ParserBlock(Parser);
        }
        catch (...)
        {
            Parser->LoopDepth = OldLoops;
            Parser->FunctionDepth--;
            throw;
        }

        Parser->LoopDepth = OldLoops;
        Parser->FunctionDepth--;

        if (Member)
        {
            Node->MethodDeclaration.Name       = Name;
            Node->MethodDeclaration.ReturnType = Type;
            Node->MethodDeclaration.Body       = Body;
        }
        else
        {
            Node->FunctionDeclaration.Name       = Name;
            Node->FunctionDeclaration.ReturnType = Type;
            Node->FunctionDeclaration.Body       = Body;
        }

        Node->Location = ParserFinish(Parser, Start);

        return Node;
    }

    Type = ParserArrayDeclarator(Parser, Type);

    if (Type->Type == AST_NODE_TYPE::TYPE && Type->TypeInfo.Token == TOKENS::TOKEN_TYPE_VOID)
    {
        ParserFailAt(Parser, Type->Location, DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE, "a variable type");
    }

    AST_NODE *Initializer = ParserMatch(Parser, TOKENS::TOKEN_OP_ASSIGN) ? ParserExpression(Parser) : NULLPTR;
    AST_NODE *Base        = Type;

    while (Base->Type == AST_NODE_TYPE::ARRAY_TYPE)
    {
        Base = Base->ArrayType.ElementType;
    }

    if ((Inferred || (Base->Type == AST_NODE_TYPE::TYPE && Base->TypeInfo.Token >= TOKENS::TOKEN_TYPE_AUTO8 && Base->TypeInfo.Token <= TOKENS::TOKEN_TYPE_AUTO64)) && Initializer == NULLPTR)
    {
        if (ParserPeek(Parser) == TOKENS::TOKEN_SP_SEMICOLON || ParserPeek(Parser) == TOKENS::TOKEN_EOF || ParserPeek(Parser) == TOKENS::TOKEN_SP_COMMA || ParserPeek(Parser) == TOKENS::TOKEN_SP_RBRACE)
        {
            ParserFailAt(Parser, {Start.Start, ParserFinish(Parser, Start).Length, Start.Line, Start.Column}, DIAGNOSTIC_MESSAGE::MISSING_INITIALIZER, Name);
        }

        ParserFail(Parser, DIAGNOSTIC_MESSAGE::EXPECTED_TOKEN, "=");
    }

    if (Terminator)
    {
        ParserConsume(Parser, TOKENS::TOKEN_SP_SEMICOLON, ";", Member ? "field declaration" : "variable declaration");
    }

    AST_NODE *Node = ParserNode(Parser, Member ? AST_NODE_TYPE::FIELD_DECLARATION : AST_NODE_TYPE::VARIABLE_DECLARATION, ParserFinish(Parser, Start));

    if (Member)
    {
        Node->FieldDeclaration = {Name, Type, Initializer};
    }
    else
    {
        Node->VariableDeclaration = {Name, Type, Initializer};
    }

    return Node;
}

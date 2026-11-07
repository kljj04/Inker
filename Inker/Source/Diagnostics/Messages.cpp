// Inker/Source/Diagnostics/Messages.cpp
// Inker Project
// Copyright (C) 2026 Inker Developer kljj04

#include <Types.hpp>
#include <Diagnostics/Messages.hpp>

CONST CHAR8 *DiagnosticMessageString(DIAGNOSTIC_MESSAGE Message)
{
    switch (Message)
    {
        case DIAGNOSTIC_MESSAGE::NONE:
        {
            return "";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_CHARACTER:
        {
            return "invalid character";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_NUMBER:
        {
            return "invalid number";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ESCAPE:
        {
            return "invalid escape sequence";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_STRING:
        {
            return "unterminated string";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_COMMENT:
        {
            return "unterminated block comment";
        }

        case DIAGNOSTIC_MESSAGE::UNTERMINATED_INTERPOLATION:
        {
            return "unterminated interpolation";
        }

        case DIAGNOSTIC_MESSAGE::NESTING_LIMIT:
        {
            return "maximum nesting depth exceeded";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_IDENTIFIER:
        {
            return "expected identifier";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_EXPRESSION:
        {
            return "expected expression";
        }

        case DIAGNOSTIC_MESSAGE::UNEXPECTED_TOKEN:
        {
            return "unexpected token";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_TOKEN:
        {
            return "expected '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_TYPE:
        {
            return "expected type";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_DECLARATION:
        {
            return "expected declaration";
        }

        case DIAGNOSTIC_MESSAGE::UNEXPECTED_EOF:
        {
            return "unexpected end of file";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_LITERAL:
        {
            return "invalid literal '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_OPERAND:
        {
            return "invalid operand for '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ARGUMENT:
        {
            return "invalid argument '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ASSIGNMENT:
        {
            return "invalid assignment";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_CONVERSION:
        {
            return "cannot convert '{0}' to '{1}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_TYPE:
        {
            return "invalid type '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_DECLARATION:
        {
            return "invalid declaration";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_CONTROL_FLOW:
        {
            return "invalid use of '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::UNDEFINED_SYMBOL:
        {
            return "undefined symbol '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::REDEFINED_SYMBOL:
        {
            return "redefinition of '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::AMBIGUOUS_SYMBOL:
        {
            return "ambiguous symbol '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INACCESSIBLE_SYMBOL:
        {
            return "symbol '{0}' is inaccessible";
        }

        case DIAGNOSTIC_MESSAGE::TYPE_MISMATCH:
        {
            return "expected type '{0}', found '{1}'";
        }

        case DIAGNOSTIC_MESSAGE::ARGUMENT_COUNT_MISMATCH:
        {
            return "expected {0} arguments, found {1}";
        }

        case DIAGNOSTIC_MESSAGE::MISSING_RETURN:
        {
            return "missing return value";
        }

        case DIAGNOSTIC_MESSAGE::OUT_OF_RANGE:
        {
            return "value '{0}' is out of range";
        }

        case DIAGNOSTIC_MESSAGE::INTEGER_OVERFLOW:
        {
            return "integer overflow";
        }

        case DIAGNOSTIC_MESSAGE::DIVISION_BY_ZERO:
        {
            return "division by zero";
        }

        case DIAGNOSTIC_MESSAGE::UNINITIALIZED_VALUE:
        {
            return "use of uninitialized value '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::UNUSED_SYMBOL:
        {
            return "unused symbol '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::UNUSED_VALUE:
        {
            return "unused value";
        }

        case DIAGNOSTIC_MESSAGE::UNREACHABLE_CODE:
        {
            return "unreachable code";
        }

        case DIAGNOSTIC_MESSAGE::SHADOWED_SYMBOL:
        {
            return "declaration shadows '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::IMPLICIT_CONVERSION:
        {
            return "implicit conversion from '{0}' to '{1}'";
        }

        case DIAGNOSTIC_MESSAGE::PRECISION_LOSS:
        {
            return "conversion from '{0}' to '{1}' may lose precision";
        }

        case DIAGNOSTIC_MESSAGE::DEPRECATED_USAGE:
        {
            return "'{0}' is deprecated";
        }

        case DIAGNOSTIC_MESSAGE::UNSUPPORTED_FEATURE:
        {
            return "unsupported feature '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INTERNAL_ERROR:
        {
            return "internal error: {0}";
        }

        case DIAGNOSTIC_MESSAGE::MISSING_SEMICOLON:
        {
            return "expected ';' after {0}";
        }

        case DIAGNOSTIC_MESSAGE::MISSING_INITIALIZER:
        {
            return "inferred declaration '{0}' requires an initializer";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_NAME:
        {
            return "expected {0}";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ASSIGNMENT_TARGET:
        {
            return "left side of assignment must be a variable, member, array element or dereference";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_INCREMENT_TARGET:
        {
            return "operand of '{0}' must be a variable, member, array element or dereference";
        }

        case DIAGNOSTIC_MESSAGE::LOOP_CONTROL_OUTSIDE_LOOP:
        {
            return "'{0}' is only allowed inside a loop";
        }

        case DIAGNOSTIC_MESSAGE::RETURN_OUTSIDE_FUNCTION:
        {
            return "'return' is only allowed inside a function";
        }

        case DIAGNOSTIC_MESSAGE::ZERO_RANGE_STEP:
        {
            return "range for step must not be zero";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_RANGE_FOR:
        {
            return "range for requires '(declaration = start, exclusive end, step)'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_TYPE_SPECIFIERS:
        {
            return "invalid combination of type specifiers '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_ARRAY_SIZE:
        {
            return "array size must be greater than zero";
        }

        case DIAGNOSTIC_MESSAGE::VOID_VALUE_TYPE:
        {
            return "void cannot be used as {0}";
        }

        case DIAGNOSTIC_MESSAGE::PREVIOUS_DECLARATION:
        {
            return "previous declaration of '{0}' is here";
        }

        case DIAGNOSTIC_MESSAGE::MODULE_NOT_FOUND:
        {
            return "cannot find module file '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::MODULE_READ_FAILED:
        {
            return "cannot read module file '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::CIRCULAR_IMPORT:
        {
            return "circular import: {0}; each module is loaded only once";
        }

        case DIAGNOSTIC_MESSAGE::IMPORT_NOT_FOUND:
        {
            return "module does not export '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_IMPORT_ALIAS:
        {
            return "file name '{0}' is not a valid module name; specify an alias with 'as'";
        }

        case DIAGNOSTIC_MESSAGE::EXPECTED_NAMESPACE:
        {
            return "expected namespace '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::UNSUPPORTED_AUTO_TYPE:
        {
            return "{0} cannot infer {1}";
        }

        case DIAGNOSTIC_MESSAGE::ARRAY_CAPACITY:
        {
            return "initializer requires {0} elements including any null terminator, but array capacity is {1}";
        }

        case DIAGNOSTIC_MESSAGE::FLOAT_OVERFLOW:
        {
            return "floating-point value overflows {0}; result is infinity";
        }

        case DIAGNOSTIC_MESSAGE::INTEGER_WRAP:
        {
            return "value '{0}' overflows {1}; wrapped value is {2}";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_BINARY_TYPES:
        {
            return "operator cannot be applied to '{0}' and '{1}'";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_SHIFT:
        {
            return "shift count {0} is outside the range [0, {1})";
        }

        case DIAGNOSTIC_MESSAGE::INVALID_MEMBER_TYPE:
        {
            return "type '{0}' has no accessible members";
        }

        case DIAGNOSTIC_MESSAGE::INSTANCE_MEMBER:
        {
            return "member '{0}' requires an instance";
        }

        case DIAGNOSTIC_MESSAGE::ARRAY_INDEX_OUT_OF_RANGE:
        {
            return "array index {0} is outside the range [0, {1})";
        }

        case DIAGNOSTIC_MESSAGE::NOT_CALLABLE:
        {
            return "'{0}' is not callable";
        }

        case DIAGNOSTIC_MESSAGE::CANNOT_INFER_TYPE:
        {
            return "cannot infer type of '{0}'";
        }

        case DIAGNOSTIC_MESSAGE::ENUM_CONSTANT_REQUIRED:
        {
            return "enum value must be a constant integer expression";
        }

        case DIAGNOSTIC_MESSAGE::USE_BEFORE_DECLARATION:
        {
            return "symbol '{0}' is used before its declaration";
        }

        case DIAGNOSTIC_MESSAGE::DECLARED_HERE:
        {
            return "'{0}' is declared here";
        }

        default:
        {
            return "unknown diagnostic message";
        }
    }
}

STRING DiagnosticFormatMessage(DIAGNOSTIC_MESSAGE Message, std::initializer_list<STRING_VIEW> Arguments)
{
    STRING_VIEW Template = DiagnosticMessageString(Message);
    STRING      Result;

    for (SIZE Position = 0; Position < Template.size();)
    {
        if (Template[Position] == '{' && Position + 2 < Template.size() && Template[Position + 1] >= '0' && Template[Position + 1] <= '9' && Template[Position + 2] == '}')
        {
            SIZE Index = (SIZE) (Template[Position + 1] - '0');

            if (Index < Arguments.size())
            {
                Result.append(*(Arguments.begin() + Index));
            }
            else
            {
                Result.append(Template.substr(Position, 3));
            }

            Position += 3;
        }
        else
        {
            Result.push_back(Template[Position++]);
        }
    }

    return Result;
}

CONST CHAR8 *DiagnosticLexerMessage(LEXER_ERROR Error)
{
    switch (Error)
    {
        case LEXER_ERROR::INVALID_CHARACTER:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_CHARACTER);
        }

        case LEXER_ERROR::INVALID_NUMBER:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_NUMBER);
        }

        case LEXER_ERROR::INVALID_ESCAPE:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::INVALID_ESCAPE);
        }

        case LEXER_ERROR::UNTERMINATED_STRING:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_STRING);
        }

        case LEXER_ERROR::UNTERMINATED_COMMENT:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_COMMENT);
        }

        case LEXER_ERROR::UNTERMINATED_INTERPOLATION:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::UNTERMINATED_INTERPOLATION);
        }

        case LEXER_ERROR::NESTING_LIMIT:
        {
            return DiagnosticMessageString(DIAGNOSTIC_MESSAGE::NESTING_LIMIT);
        }

        default:
        {
            return "unknown lexer error";
        }
    }
}

STRING DiagnosticParserMessage(DIAGNOSTIC_MESSAGE Message, std::initializer_list<STRING_VIEW> Arguments)
{
    return DiagnosticFormatMessage(Message, Arguments);
}

STRING DiagnosticRuntimeMessage(DIAGNOSTIC_MESSAGE Message, std::initializer_list<STRING_VIEW> Arguments)
{
    return DiagnosticFormatMessage(Message, Arguments);
}

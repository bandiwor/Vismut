//
// Created by kir on 10.10.2025.
//

#ifndef VISMUT_TYPES_MAPS_H
#define VISMUT_TYPES_MAPS_H

#define TOKENS_MAP(X)   \
    X(TOKEN_EOF, "<eof>") \
    X(TOKEN_I64_TYPE, "i64") \
    X(TOKEN_FLOAT_TYPE, "f64") \
    X(TOKEN_STRING_TYPE, "string") \
    X(TOKEN_IDENTIFIER, "<identifier>") \
    X(TOKEN_NAME_DECLARATION, "$") \
    X(TOKEN_CONDITION_STATEMENT, "#") \
    X(TOKEN_CONDITION_ELSE_IF, "!#") \
    X(TOKEN_EXCLAMATION_MARK, "!") \
    X(TOKEN_QUESTION, "?") \
    X(TOKEN_WHILE_STATEMENT, "@") \
    X(TOKEN_MODULE_DIV, "%") \
    X(TOKEN_FOR_STATEMENT, "%%") \
    X(TOKEN_NAMESPACE_DECLARATION, "<>") \
    X(TOKEN_STRUCTURE_DECLARATION, "$>") \
    X(TOKEN_RETURN_STATEMENT, "'") \
    X(TOKEN_PRINT_STATEMENT, "::") \
    X(TOKEN_INPUT_STATEMENT, ":>") \
    X(TOKEN_PLUS, "+") \
    X(TOKEN_INCREMENT, "++") \
    X(TOKEN_MINUS, "-") \
    X(TOKEN_DECREMENT, "--") \
    X(TOKEN_STAR, "*") \
    X(TOKEN_POWER, "**") \
    X(TOKEN_DIVIDE, "/") \
    X(TOKEN_INT_DIVIDE, "//") \
    X(TOKEN_ARROW,"->") \
    X(TOKEN_THEN, "=>") \
    X(TOKEN_DOT, ".") \
    X(TOKEN_COMMA, ",") \
    X(TOKEN_SEMICOLON, ";") \
    X(TOKEN_COLON, ":") \
    X(TOKEN_LBRACE,"{") \
    X(TOKEN_RBRACE, "}") \
    X(TOKEN_LBRACKET, "[") \
    X(TOKEN_RBRACKET, "]") \
    X(TOKEN_LPAREN, "(") \
    X(TOKEN_RPAREN, ")") \
    X(TOKEN_LESS_THAN, "<") \
    X(TOKEN_LESS_THAN_OR_EQUALS, "<=") \
    X(TOKEN_GREATER_THAN, ">") \
    X(TOKEN_GREATER_THAN_OR_EQUALS, ">=") \
    X(TOKEN_ASSIGN, "=") \
    X(TOKEN_EQUALS, "==") \
    X(TOKEN_BITWISE_OR, "|") \
    X(TOKEN_LOGICAL_OR, "||") \
    X(TOKEN_BITWISE_AND, "&") \
    X(TOKEN_LOGICAL_AND, "&&") \
    X(TOKEN_XOR, "^") \
    X(TOKEN_TILDA, "~") \
    X(TOKEN_NOT_EQUALS, "!=") \
    X(TOKEN_INT_LITERAL, "<i64 literal>") \
    X(TOKEN_FLOAT_LITERAL, "<f64 literal>") \
    X(TOKEN_CHARS_LITERAL, "<const char*>") \
    X(TOKEN_UNKNOWN, "<unknown>")

#define AST_NODES_MAP(X) \
    X(AST_UNKNOWN, "<unknown>") \
    X(AST_MODULE, "<module>") \
    X(AST_BLOCK, "<block>") \
    X(AST_LITERAL, "<literal>") \
    X(AST_VAR_REF, "<var ref>") \
    X(AST_VAR_DECL, "<var decl>") \
    X(AST_PRINT_STMT, "<print stmt>") \
    X(AST_IF_STMT, "<if stmt>") \
    X(AST_WHILE_STMT, "<while stmt>") \
    X(AST_FUNCTION_DECL, "<func decl>") \
    X(AST_FUNCTION_CALL, "<func call>") \
    X(AST_UNARY, "<unary>") \
    X(AST_BINARY, "<binary>") \
    X(AST_TERNARY, "<ternary>") \
    X(AST_TYPE_CAST, "<type cast>")

#define AST_BINARY_MAP(X) \
    X(AST_BINARY_UNKNOWN, "<unknown>") \
    X(AST_BINARY_ADD, "+") \
    X(AST_BINARY_SUB, "-") \
    X(AST_BINARY_MUL, "*") \
    X(AST_BINARY_POW, "**") \
    X(AST_BINARY_DIV, "/") \
    X(AST_BINARY_INT_DIV, "//") \
    X(AST_BINARY_MOD, "%") \
    X(AST_BINARY_ASSIGN, "=") \
    X(AST_BINARY_LESS_THAN, "<") \
    X(AST_BINARY_LESS_THAN_OR_EQUALS, "<=") \
    X(AST_BINARY_GREATER_THAN, ">") \
    X(AST_BINARY_GREATER_THAN_OR_EQUALS, ">=") \
    X(AST_BINARY_EQUALS, "==") \
    X(AST_BINARY_NOT_EQUALS, "!=") \
    X(AST_BINARY_BITWISE_OR, "|") \
    X(AST_BINARY_BITWISE_AND, "&") \
    X(AST_BINARY_LOGICAL_OR, "||") \
    X(AST_BINARY_LOGICAL_AND, "&&") \
    X(AST_BINARY_SHIFT_LEFT, "<<") \
    X(AST_BINARY_SHIFT_RIGHT, ">>")

#define AST_UNARY_MAP(X) \
    X(AST_UNARY_UNKNOWN, "<unknown>") \
    X(AST_UNARY_PLUS, "+") \
    X(AST_UNARY_MINUS, "-") \
    X(AST_UNARY_LOGICAL_NOT, "!") \
    X(AST_UNARY_BITWISE_NOT, "~") \
    X(AST_UNARY_INCREMENT, "++") \
    X(AST_UNARY_DECREMENT, "--")

#define VALUE_TYPE_MAP(X) \
    X(VALUE_UNKNOWN, "<unknown>") \
    X(VALUE_VOID, "void") \
    X(VALUE_AUTO, "auto") \
    X(VALUE_I64, "i64") \
    X(VALUE_F64, "float") \
    X(VALUE_STR, "str")


#endif //VISMUT_TYPES_MAPS_H

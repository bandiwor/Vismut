//
// Created by kir on 10.10.2025.
//

#ifndef VISMUT_TYPES_MAPS_H
#define VISMUT_TYPES_MAPS_H

#define TOKENS_MAP(X)   \
    X(TOKEN_EOF, L"<eof>") \
    X(TOKEN_I64_TYPE, L"i64") \
    X(TOKEN_FLOAT_TYPE, L"f64") \
    X(TOKEN_STRING_TYPE, L"string") \
    X(TOKEN_IDENTIFIER, L"<identifier>") \
    X(TOKEN_NAME_DECLARATION, L"$") \
    X(TOKEN_CONDITION_STATEMENT, L"#") \
    X(TOKEN_CONDITION_ELSE_IF, L"!#") \
    X(TOKEN_EXCLAMATION_MARK, L"!") \
    X(TOKEN_QUESTION, L"?") \
    X(TOKEN_WHILE_STATEMENT, L"@") \
    X(TOKEN_MODULE_DIV, L"%") \
    X(TOKEN_FOR_STATEMENT, L"%%") \
    X(TOKEN_NAMESPACE_DECLARATION, L"<>") \
    X(TOKEN_STRUCTURE_DECLARATION, L"$>") \
    X(TOKEN_RETURN_STATEMENT, L"'") \
    X(TOKEN_PRINT_STATEMENT, L"::") \
    X(TOKEN_INPUT_STATEMENT, L":>") \
    X(TOKEN_PLUS, L"+") \
    X(TOKEN_INCREMENT, L"++") \
    X(TOKEN_MINUS, L"-") \
    X(TOKEN_DECREMENT, L"--") \
    X(TOKEN_STAR, L"*") \
    X(TOKEN_POWER, L"**") \
    X(TOKEN_DIVIDE, L"/") \
    X(TOKEN_INT_DIVIDE, L"//") \
    X(TOKEN_ARROW, L"->") \
    X(TOKEN_THEN, L"=>") \
    X(TOKEN_DOT, L".") \
    X(TOKEN_COMMA, L",") \
    X(TOKEN_SEMICOLON, L";") \
    X(TOKEN_COLON, L":") \
    X(TOKEN_LBRACE, L"{") \
    X(TOKEN_RBRACE, L"}") \
    X(TOKEN_LBRACKET, L"[") \
    X(TOKEN_RBRACKET, L"]") \
    X(TOKEN_LPAREN, L"(") \
    X(TOKEN_RPAREN, L")") \
    X(TOKEN_LESS_THAN, L"<") \
    X(TOKEN_LESS_THAN_OR_EQUALS, L"<=") \
    X(TOKEN_GREATER_THAN, L">") \
    X(TOKEN_GREATER_THAN_OR_EQUALS, L">=") \
    X(TOKEN_ASSIGN, L"=") \
    X(TOKEN_EQUALS, L"==") \
    X(TOKEN_BITWISE_OR, L"|") \
    X(TOKEN_LOGICAL_OR, L"||") \
    X(TOKEN_BITWISE_AND, L"&") \
    X(TOKEN_LOGICAL_AND, L"&&") \
    X(TOKEN_XOR, L"^") \
    X(TOKEN_TILDA, L"~") \
    X(TOKEN_NOT_EQUALS, L"!=") \
    X(TOKEN_INT_LITERAL, L"<i64 literal>") \
    X(TOKEN_FLOAT_LITERAL, L"<f64 literal>") \
    X(TOKEN_CHARS_LITERAL, L"<const wchar_t*>") \
    X(TOKEN_UNKNOWN, L"<unknown>")

#define AST_NODES_MAP(X) \
    X(AST_UNKNOWN, L"<unknown>") \
    X(AST_MODULE, L"<module>") \
    X(AST_BLOCK, L"<block>") \
    X(AST_LITERAL, L"<literal>") \
    X(AST_VAR_REF, L"<var ref>") \
    X(AST_VAR_DECL, L"<var decl>") \
    X(AST_PRINT_STMT, L"<print stmt>") \
    X(AST_IF_STMT, L"<if stmt>") \
    X(AST_WHILE_STMT, L"<while stmt>") \
    X(AST_FUNCTION_DECL, L"<func decl>") \
    X(AST_FUNCTION_CALL, L"<func call>") \
    X(AST_UNARY, L"<unary>") \
    X(AST_BINARY, L"<binary>") \
    X(AST_TERNARY, L"<ternary>") \
    X(AST_TYPE_CAST, L"<type cast>")

#define AST_BINARY_MAP(X) \
    X(AST_BINARY_UNKNOWN, L"<unknown>") \
    X(AST_BINARY_ADD, L"+") \
    X(AST_BINARY_SUB, L"-") \
    X(AST_BINARY_MUL, L"*") \
    X(AST_BINARY_POW, L"**") \
    X(AST_BINARY_DIV, L"/") \
    X(AST_BINARY_INT_DIV, L"//") \
    X(AST_BINARY_MOD, L"%") \
    X(AST_BINARY_ASSIGN, L"=") \
    X(AST_BINARY_LESS_THAN, L"<") \
    X(AST_BINARY_LESS_THAN_OR_EQUALS, L"<=") \
    X(AST_BINARY_GREATER_THAN, L">") \
    X(AST_BINARY_GREATER_THAN_OR_EQUALS, L">=") \
    X(AST_BINARY_EQUALS, L"==") \
    X(AST_BINARY_NOT_EQUALS, L"!=") \
    X(AST_BINARY_BITWISE_OR, L"|") \
    X(AST_BINARY_BITWISE_AND, L"&") \
    X(AST_BINARY_LOGICAL_OR, L"||") \
    X(AST_BINARY_LOGICAL_AND, L"&&") \
    X(AST_BINARY_SHIFT_LEFT, L"<<") \
    X(AST_BINARY_SHIFT_RIGHT, L">>")

#define AST_UNARY_MAP(X) \
    X(AST_UNARY_UNKNOWN, L"<unknown>") \
    X(AST_UNARY_PLUS, L"+") \
    X(AST_UNARY_MINUS, L"-") \
    X(AST_UNARY_LOGICAL_NOT, L"!") \
    X(AST_UNARY_BITWISE_NOT, L"~") \
    X(AST_UNARY_INCREMENT, L"++") \
    X(AST_UNARY_DECREMENT, L"--")

#define VALUE_TYPE_MAP(X) \
    X(VALUE_UNKNOWN, L"<unknown>") \
    X(VALUE_VOID, L"void") \
    X(VALUE_AUTO, L"auto") \
    X(VALUE_I64, L"i64") \
    X(VALUE_F64, L"float") \
    X(VALUE_WSTR, L"str")


#endif //VISMUT_TYPES_MAPS_H

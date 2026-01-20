#include "token.h"


void Token_Print(const VToken *token) {
    wprintf(L"%ls", VTokenType_String(token->type));
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            wprintf(L": %ls\n", token->data.w_chars);
            break;
        case TOKEN_CHARS_LITERAL:
            wprintf(L": \"%ls\"\n", token->data.w_chars);
            break;
        case TOKEN_INT_LITERAL:
            wprintf(L": %lld\n", token->data.i64);
            break;
        case TOKEN_FLOAT_LITERAL:
            wprintf(L": %f\n", token->data.f64);
            break;
        default:
            wprintf(L"\n");
            break;
    }
}

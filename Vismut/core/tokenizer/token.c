#include "token.h"


void Token_Print(const VToken *token) {
    printf("%s", VTokenType_String(token->type));
    switch (token->type) {
        case TOKEN_IDENTIFIER:
            printf(": %s\n", token->data.chars);
            break;
        case TOKEN_CHARS_LITERAL:
            printf(": \"%s\"\n", token->data.chars);
            break;
        case TOKEN_INT_LITERAL:
            printf(": %lld\n", token->data.i64);
            break;
        case TOKEN_FLOAT_LITERAL:
            printf(": %f\n", token->data.f64);
            break;
        default:
            printf("\n");
            break;
    }
}

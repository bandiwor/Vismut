//
// Created by kir on 09.10.2025.
//
#include "tokenizer.h"

#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#include "../convert.h"
#include "../errors/errors.h"

attribute_cold
Tokenizer Tokenizer_Create(const char *source, const size_t source_length, const char *source_filename,
                           Arena *arena) {
    const size_t buffer_size = 256;
    char *buffer = Arena_Array(arena, char, buffer_size);
    memset(buffer, 0, sizeof(char) * buffer_size);

    Tokenizer tokenizer = {
        .source = source,
        .source_length = source_length,
        .buffer = buffer,
        .buffer_size = buffer_size,
        .start_token_position = 0,
        .current_position = 0,
        .current_char = 0,
        .arena = arena,
        .source_filename = source_filename,
    };
    if (source_length > 0) {
        tokenizer.current_char = source[0];
    }

    return tokenizer;
}

attribute_cold
void Tokenizer_Reset(Tokenizer *tokenizer) {
    tokenizer->current_position = 0;
    tokenizer->start_token_position = 0;
    if (tokenizer->source_length > 0) {
        tokenizer->current_char = tokenizer->source[0];
    }
}

attribute_hot
static bool IsSpace(const char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\b' || c == '\v';
}

static bool IsDigit(const char c) {
    return c >= '0' && c <= '9';
}

attribute_hot
static bool IsASCIIAlphaOrUnderscore(const char c) {
    return (c >= 'a' && c <= 'z')
           || (c == '_')
           || (c >= 'A' && c <= 'Z');
}

attribute_hot
static void Tokenizer_Advance(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (likely(tokenizer->current_position + 1 < tokenizer->source_length)) {
        tokenizer->current_char = tokenizer->source[++tokenizer->current_position];
    } else {
        tokenizer->current_char = '\0';
    }
}

static void Tokenizer_SetPosition(Tokenizer *tokenizer, const size_t position) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (tokenizer->source_length == 0) return;
    DEBUG_ASSERT(position < tokenizer->source_length);

    tokenizer->current_position = position;
    tokenizer->current_char = tokenizer->source[position];
}

static void Tokenizer_DoubleAdvance(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (likely(tokenizer->current_position + 2 < tokenizer->source_length)) {
        tokenizer->current_position += 2;
        tokenizer->current_char = tokenizer->source[tokenizer->current_position];
    }
}

attribute_pure
static char Tokenizer_NextChar(const Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (likely(tokenizer->current_position + 1 < tokenizer->source_length)) {
        return tokenizer->source[tokenizer->current_position + 1];
    }

    return '\0';
}

static void Tokenizer_SkipSingleComment(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_LOG_ASSERT(
        tokenizer->current_char == '/' && Tokenizer_NextChar(tokenizer) == '/',
        "current char and next char must to be equal '/'"
    );

    // Skip '//'
    Tokenizer_DoubleAdvance(tokenizer);
    // Skip line
    while (tokenizer->current_char != '\n' && tokenizer->current_char != '\0') {
        if (tokenizer->current_char == '\r' && Tokenizer_NextChar(tokenizer) == '\n') {
            Tokenizer_DoubleAdvance(tokenizer);
            return;
        }
        Tokenizer_Advance(tokenizer);
    }
    // Skip '\n'
    Tokenizer_Advance(tokenizer);
}

static void Tokenizer_SkipMultiLineComment(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_LOG_ASSERT(
        tokenizer->current_char == '/' && Tokenizer_NextChar(tokenizer) == '*',
        "current char must to be equal '/', next char must to be equal '*'"
    );

    // Skip '/*'
    Tokenizer_DoubleAdvance(tokenizer);
    // Skip comment
    while (
        tokenizer->current_char != L'*' &&
        Tokenizer_NextChar(tokenizer) != L'/' &&
        tokenizer->current_char != '\0'
    ) {
        Tokenizer_Advance(tokenizer);
    }

    // Skip '*/'
    Tokenizer_DoubleAdvance(tokenizer);
}

attribute_pure
static VTokenType Tokenizer_GetKeyword(const char *buffer,
                                       const size_t length) {
#define COMPARE_3(c1, c2, c3, token)                                           \
  do {                                                                         \
    if (buffer[0] == (c1) && buffer[1] == (c2) && buffer[2] == (c3))           \
      return token;                                                            \
  } while (0)

    if (length == 3) {
        COMPARE_3('i', '6', '4', TOKEN_I64_TYPE);
        COMPARE_3('f', '6', '4', TOKEN_FLOAT_TYPE);
        COMPARE_3('s', 't', 'r', TOKEN_STRING_TYPE);
        return TOKEN_UNKNOWN;
    }

    return TOKEN_UNKNOWN;

#undef COMPARE_3
}

static errno_t Tokenizer_ScanIdentifierIntoBuffer(Tokenizer *tokenizer, size_t *keyword_length) {
    DEBUG_ASSERT(IsASCIIAlphaOrUnderscore(tokenizer->current_char));

    size_t length = 0;
    tokenizer->buffer[length++] = tokenizer->current_char;

    Tokenizer_Advance(tokenizer);

    while (IsASCIIAlphaOrUnderscore(tokenizer->current_char) || IsDigit(tokenizer->current_char)) {
        if (length >= tokenizer->buffer_size) {
            return VISMUT_ERROR_BUFFER_OVERFLOW;
        }
        tokenizer->buffer[length++] = tokenizer->current_char;
        Tokenizer_Advance(tokenizer);
    }

    *keyword_length = length;
    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_ParseIdentifier(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);
    DEBUG_ASSERT(IsASCIIAlphaOrUnderscore(tokenizer->current_char));

    size_t keyword_length;

    errno_t err;
    RISKY_EXPRESSION_SAFE(
        Tokenizer_ScanIdentifierIntoBuffer(tokenizer, &keyword_length),
        err
    );

    token->position.length = keyword_length;

    VTokenType keyword_type;
    if ((keyword_type = Tokenizer_GetKeyword(tokenizer->buffer, keyword_length)) != TOKEN_UNKNOWN) {
        token->type = keyword_type;
        return VISMUT_ERROR_OK;
    }

    char *identifier = Arena_Array(tokenizer->arena, char, keyword_length + 1);
    memcpy(identifier, tokenizer->buffer, keyword_length);
    identifier[keyword_length] = L'\0';

    token->type = TOKEN_IDENTIFIER;
    token->data.chars = identifier;

    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_ParseNumber(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);
    DEBUG_ASSERT(
        IsDigit(tokenizer->current_char) ||
        (tokenizer->current_char == '-' && IsDigit(Tokenizer_NextChar(tokenizer))))
    ;

    static char buffer[128] = {0};
    const size_t start_position = tokenizer->current_position;
    size_t buffer_length = 1;

    bool is_negative = false;
    if (tokenizer->current_char == L'-') {
        is_negative = true;
        Tokenizer_Advance(tokenizer);
    }

    buffer[0] = tokenizer->current_char;
    Tokenizer_Advance(tokenizer);
    int base = 10;
    bool has_point = false, has_exponent = false;

    if (buffer[0] == '0') {
        switch (tokenizer->current_char) {
            case 'x':
            case 'X':
                base = 16;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            case 'o':
            case 'O':
                base = 8;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            case 'b':
            case 'B':
                base = 2;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            default:
                break;
        }
    }

    while (tokenizer->current_char != '\0') {
        if (unlikely(buffer_length + 1 >= _countof(buffer))) {
            return VISMUT_ERROR_BUFFER_OVERFLOW;
        }

        char c = tokenizer->current_char;
        bool is_valid = false;

        if (base == 16) {
            is_valid = (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
        } else if (base == 8) {
            is_valid = c >= '0' && c <= '7';
        } else if (base == 2) {
            is_valid = c == '0' || c == '1';
        } else {
            is_valid = true;
            if (c >= L'0' && c <= L'9') {
            } else if (c == L'.' && !has_point && !has_exponent) {
                has_point = true;
            } else if (tolower(c) == 'e' && !has_exponent) {
                has_exponent = true;
                buffer[buffer_length++] = c;
                Tokenizer_Advance(tokenizer);
                c = tokenizer->current_char;
                if (c == '+' || c == '-') {
                    buffer[buffer_length++] = c;
                    Tokenizer_Advance(tokenizer);
                }
            } else {
                is_valid = false;
            }
        }
        if (!is_valid)
            break;

        buffer[buffer_length++] = c;
        Tokenizer_Advance(tokenizer);
    }
    buffer[buffer_length] = '\0';

    char *end_ptr;
    errno = 0;

    if (base != 10) {
        int64_t num = 0;

        switch (base) {
            case 2:
                num = StrToInt64Bin(buffer);
                break;
            case 8:
                num = StrToInt64Oct(buffer);
                break;
            case 16:
                num = StrToInt64Hex(buffer);
                break;
            default:
                DEBUG_ASSERT("Unreachable code");
        }

        token->type = TOKEN_INT_LITERAL;
        token->data.i64 = is_negative ? -num : num;
    } else {
        if (has_point || has_exponent) {
            const double num = strtod(buffer, &end_ptr);
            if (*end_ptr != '\0' || errno == ERANGE) {
                return (errno == ERANGE) ? VISMUT_ERROR_NUMBER_OVERFLOW : VISMUT_ERROR_NUMBER_PARSE;
            }
            token->type = TOKEN_FLOAT_LITERAL;
            token->data.f64 = is_negative ? -num : num;
        } else {
            const long long num = strtoll(buffer, &end_ptr, 10);
            if (*end_ptr != '\0' || errno == ERANGE) {
                return (errno == ERANGE) ? VISMUT_ERROR_NUMBER_OVERFLOW : VISMUT_ERROR_NUMBER_PARSE;
            }
            token->type = TOKEN_INT_LITERAL;
            token->data.i64 = is_negative ? -num : num;
        }
    }

    token->position.length = tokenizer->current_position - start_position;

    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_CountStringLength(Tokenizer *tokenizer, const char string_symbol, size_t *out_length) {
    const size_t start_position = tokenizer->current_position;
    size_t string_length = 0;
    bool escaped_symbol = false;

    while (tokenizer->current_char != '\0') {
        if (tokenizer->current_char == '\\') {
            if (!escaped_symbol) {
                escaped_symbol = true;
                Tokenizer_Advance(tokenizer);
                continue;
            }
            escaped_symbol = false;
            string_length++;
        } else if (tokenizer->current_char == string_symbol) {
            if (!escaped_symbol) {
                Tokenizer_Advance(tokenizer);
                break;
            }
            escaped_symbol = false;
            string_length++;
        } else {
            if (escaped_symbol) {
                switch (tokenizer->current_char) {
                    case 'n': // newline
                    case 't': // tab
                    case 'r': // carriage return
                    // case '0':  // null character
                    case '"': // double quote
                    case '\'': // single quote
                    case '\\': // backslash
                        string_length++;
                        break;
                    default:
                        // Некорректная escape-последовательность
                        tokenizer->current_position = start_position;
                        return EILSEQ;
                }
                escaped_symbol = false;
            } else {
                string_length++;
            }
        }

        Tokenizer_Advance(tokenizer);
    }

    Tokenizer_SetPosition(tokenizer, start_position);

    if (tokenizer->current_char == '\0' && !escaped_symbol) {
        return ENOENT;
    }

    *out_length = string_length;
    return 0;
}

static errno_t Tokenizer_ParseString(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);

    const char string_symbol = tokenizer->current_char;
    const size_t start_position = tokenizer->current_position;

    Tokenizer_Advance(tokenizer); // Пропускаем открывающую кавычку
    token->type = TOKEN_CHARS_LITERAL;

    size_t string_length = 0;
    errno_t count_result;

    if ((count_result = Tokenizer_CountStringLength(tokenizer, string_symbol, &string_length)) != 0) {
        tokenizer->current_position = start_position;
        return count_result;
    }

    char *string = Arena_Array(tokenizer->arena, char, string_length + 1);

    Tokenizer_SetPosition(tokenizer, start_position);
    Tokenizer_Advance(tokenizer);

    size_t index = 0;
    bool escaped_symbol = false;

    while (tokenizer->current_char != '\0') {
        if (tokenizer->current_char == '\\') {
            if (!escaped_symbol) {
                escaped_symbol = true;
                Tokenizer_Advance(tokenizer);
                continue;
            }
            escaped_symbol = false;
            string[index++] = '\\';
        } else if (tokenizer->current_char == string_symbol) {
            if (!escaped_symbol) {
                Tokenizer_Advance(tokenizer);
                break;
            }
            escaped_symbol = false;
            string[index++] = string_symbol;
        } else {
            if (escaped_symbol) {
                switch (tokenizer->current_char) {
                    case 'n': string[index++] = '\n';
                        break;
                    case 't': string[index++] = '\t';
                        break;
                    case 'r': string[index++] = '\r';
                        break;
                    case '"': string[index++] = '"';
                        break;
                    case '\'': string[index++] = '\'';
                        break;
                    case '\\': string[index++] = '\\';
                        break;
                    default:
                        free(string);
                        tokenizer->current_position = start_position;
                        return EILSEQ;
                }
                escaped_symbol = false;
            } else {
                string[index++] = tokenizer->current_char;
            }
        }

        Tokenizer_Advance(tokenizer);
    }

    string[index] = '\0';

    if (index != string_length) {
        free(string);
        tokenizer->current_position = start_position;
        return EILSEQ;
    }

    token->data.chars = string;
    token->position.length = string_length + 2;

    return 0;
}

errno_t Tokenizer_Next(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);

Tokenizer_Next_start:
    while (IsSpace(tokenizer->current_char)) {
        Tokenizer_Advance(tokenizer);
    }

    if (tokenizer->current_char == '\0') {
        token->type = TOKEN_EOF;
        return VISMUT_ERROR_OK;
    }

    const char current_char = tokenizer->current_char;
    const char next_char = Tokenizer_NextChar(tokenizer);
    token->position.offset = tokenizer->current_position;

    if (IsASCIIAlphaOrUnderscore(current_char)) {
        return Tokenizer_ParseIdentifier(tokenizer, token);
    }
    if (IsDigit(current_char) || (current_char == '-' && IsDigit(next_char))) {
        return Tokenizer_ParseNumber(tokenizer, token);
    }

    if (current_char == '/') {
        if (next_char == '/') {
            Tokenizer_SkipSingleComment(tokenizer);
            goto Tokenizer_Next_start;
        }
        if (next_char == '*') {
            Tokenizer_SkipMultiLineComment(tokenizer);
            goto Tokenizer_Next_start;
        }
    }

    if (current_char == '\"') {
        return Tokenizer_ParseString(tokenizer, token);
    }

#define CASE_1(char, type_)\
    case char: type = type_;\
    break;
#define CASE_1_2(char1, char2, type1, type2) \
    case char1: {  \
        if (next_char == char2) {  \
            is_wide_token = true;   \
            type = type2;   \
            Tokenizer_Advance(tokenizer);   \
            break;  \
        }   \
        type = type1;   \
        break;  \
    }
#define CASE_1_2_2(char1, char2_1, char2_2, type1, type2_1, type2_2) \
    case char1: {  \
        if (next_char == char2_1) {  \
            is_wide_token = true;   \
            type = type2_1;   \
            Tokenizer_Advance(tokenizer);   \
            break;  \
        }   \
        if (next_char == char2_2) {  \
            is_wide_token = true;   \
            type = type2_2;   \
            Tokenizer_Advance(tokenizer);   \
            break;  \
        }   \
        type = type1;   \
        break;  \
    }

    Tokenizer_Advance(tokenizer);
    VTokenType type;
    bool is_wide_token = false;
    switch (current_char) {
        CASE_1('{', TOKEN_LBRACE)
        CASE_1('}', TOKEN_RBRACE)
        CASE_1('[', TOKEN_LBRACKET)
        CASE_1(']', TOKEN_RBRACKET)
        CASE_1('(', TOKEN_LPAREN)
        CASE_1(')', TOKEN_RPAREN)
        CASE_1('.', TOKEN_DOT)
        CASE_1(',', TOKEN_COMMA)
        CASE_1(';', TOKEN_SEMICOLON)
        CASE_1('^', TOKEN_XOR)
        CASE_1('~', TOKEN_TILDA)
        CASE_1('?', TOKEN_QUESTION)

        CASE_1('@', TOKEN_WHILE_STATEMENT)

        CASE_1_2('#', '!', TOKEN_CONDITION_STATEMENT, TOKEN_CONDITION_ELSE_IF)
        CASE_1_2('*', '*', TOKEN_STAR, TOKEN_POWER)
        CASE_1_2('!', '=', TOKEN_EXCLAMATION_MARK, TOKEN_NOT_EQUALS)
        CASE_1_2('>', '=', TOKEN_GREATER_THAN, TOKEN_GREATER_THAN_OR_EQUALS)
        CASE_1_2('$', '>', TOKEN_NAME_DECLARATION, TOKEN_STRUCTURE_DECLARATION)
        CASE_1_2('+', '+', TOKEN_PLUS, TOKEN_INCREMENT)
        CASE_1_2('/', '/', TOKEN_DIVIDE, TOKEN_INT_DIVIDE)
        CASE_1_2('%', '%', TOKEN_MODULE_DIV, TOKEN_FOR_STATEMENT)
        CASE_1_2('|', '|', TOKEN_BITWISE_OR, TOKEN_LOGICAL_OR)
        CASE_1_2('&', '&', TOKEN_BITWISE_AND, TOKEN_LOGICAL_OR)

        CASE_1_2_2(':', ':', '>', TOKEN_COLON, TOKEN_PRINT_STATEMENT, TOKEN_INPUT_STATEMENT);
        CASE_1_2_2('<', '=', '>', TOKEN_LESS_THAN, TOKEN_LESS_THAN_OR_EQUALS, TOKEN_NAMESPACE_DECLARATION)
        CASE_1_2_2('=', '=', '>', TOKEN_ASSIGN, TOKEN_EQUALS, TOKEN_THEN)
        CASE_1_2_2('-', '>', '-', TOKEN_MINUS, TOKEN_ARROW, TOKEN_DECREMENT)

        default:
            return VISMUT_ERROR_UNKNOWN_CHAR;
    }
    token->position.length = is_wide_token ? 2 : 1;
    token->type = type;
    return VISMUT_ERROR_OK;
}

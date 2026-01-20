//
// Created by kir on 09.10.2025.
//
#include "tokenizer.h"

#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>

#include "../convert.h"
#include "../errors/errors.h"

typedef enum {
    CT_UNKNOWN = 0, // Unknown / Control
    CT_SPACE = 1, // Whitespace
    CT_DIGIT = 2, // Digit 0-9
    CT_ALPHA = 3, // Alpha a-z, A-Z, _
    CT_QUOTES = 4, // Quotes "
    CT_SLASH = 5, // Slash / (start of comment or divide)
    CT_OPERATOR = 6 // Operators needing switch
} CharType;

static const uint8_t CharMap[256] = {
    [0 ... 32] = CT_UNKNOWN, // Control chars default
    ['\t'] = CT_SPACE, ['\n'] = CT_SPACE, ['\v'] = CT_SPACE,
    ['\f'] = CT_SPACE, ['\r'] = CT_SPACE, [' '] = CT_SPACE,

    ['0' ... '9'] = CT_DIGIT,
    ['a' ... 'z'] = CT_ALPHA,
    ['A' ... 'Z'] = CT_ALPHA,
    ['_'] = CT_ALPHA,

    ['"'] = CT_QUOTES,
    ['/'] = CT_SLASH,

    // Operators
    ['{'] = CT_OPERATOR, ['}'] = CT_OPERATOR, ['['] = CT_OPERATOR, [']'] = CT_OPERATOR,
    ['('] = CT_OPERATOR, [')'] = CT_OPERATOR, ['.'] = CT_OPERATOR, [','] = CT_OPERATOR,
    [';'] = CT_OPERATOR, [':'] = CT_OPERATOR, ['^'] = CT_OPERATOR, ['~'] = CT_OPERATOR,
    ['?'] = CT_OPERATOR, ['@'] = CT_OPERATOR, ['!'] = CT_OPERATOR, ['*'] = CT_OPERATOR,
    ['<'] = CT_OPERATOR, ['>'] = CT_OPERATOR, ['='] = CT_OPERATOR, ['+'] = CT_OPERATOR,
    ['-'] = CT_OPERATOR, ['%'] = CT_OPERATOR, ['|'] = CT_OPERATOR, ['&'] = CT_OPERATOR,
    ['#'] = CT_OPERATOR, ['$'] = CT_OPERATOR
};

attribute_cold
Tokenizer Tokenizer_Create(const uint8_t *source, const size_t source_length, const uint8_t *source_filename,
                           Arena *arena) {
    return (Tokenizer){
        .source_filename = source_filename,
        .start = source,
        .cursor = source,
        .limit = source + source_length,
        .token_start = source,
        .arena = arena,
    };
}

static int IsHexDigit(const int digit) {
    return (digit >= '0' && digit <= '9')
           || (digit >= 'a' && digit <= 'f')
           || (digit >= 'A' && digit <= 'F');
}

static int IsDecDigit(const int digit) {
    return digit >= '0' && digit <= '9';
}

static errno_t Tokenizer_ParseNumber(Tokenizer *tokenizer, VToken *token) {
    const uint8_t *start = tokenizer->token_start;
    const uint8_t *cur = tokenizer->cursor;
    const uint8_t *limit = tokenizer->limit;

    bool is_hex = false;
    if (*start == '0' && cur < limit) {
        const uint8_t c = *cur;
        if (c == 'x' || c == 'X') {
            is_hex = true;
            cur++;
        } else if (c == 'b' || c == 'B') { cur++; } else if (c == 'o' || c == 'O') { cur++; }
    }

    if (is_hex) {
        while (cur < limit && IsHexDigit(*cur)) cur++;
    } else {
        while (cur < limit) {
            const uint8_t c = *cur;
            if (IsDecDigit(c)) {
                cur++;
                continue;
            }
            if (c == '.') {
                cur++;
                continue;
            }
            if (c == 'e' || c == 'E') {
                cur++;
                if (cur < limit && (*cur == '+' || *cur == '-')) cur++;
                continue;
            }
            break;
        }
    }

    const size_t length = cur - start;

    uint8_t stack_buf[64];
    uint8_t *buffer;

    if (likely(length < sizeof(stack_buf))) {
        memcpy(stack_buf, start, length);
        stack_buf[length] = '\0';
        buffer = stack_buf;
    } else {
        buffer = Arena_Array(tokenizer->arena, uint8_t, length + 1);
        memcpy(buffer, start, length);
        buffer[length] = '\0';
    }

    tokenizer->cursor = cur;
    token->position.length = length;

    uint8_t *end_ptr;
    const bool is_float = !is_hex && (memchr(buffer, '.', length) || memchr(buffer, 'e', length) || memchr(
                                          buffer, 'E', length));

    if (is_float) {
        token->type = TOKEN_FLOAT_LITERAL;
        errno = 0;
        token->data.f64 = strtod((const char *) buffer, (char **) &end_ptr);
        if (errno == ERANGE) return VISMUT_ERROR_NUMBER_OVERFLOW;
        if (end_ptr == buffer) return VISMUT_ERROR_NUMBER_PARSE;
    } else {
        token->type = TOKEN_INT_LITERAL;
        int base = 10;
        if (buffer[0] == '0' && length > 1) {
            const uint8_t b = buffer[1];
            if (b == 'x' || b == 'X') base = 16;
            else if (b == 'b' || b == 'B') base = 2;
            else if (b == 'o' || b == 'O') base = 8;
        }

        int64_t val = 0;
        if (base == 2) val = StrToInt64Bin(buffer + 2); // Из вашего convert.h
        else if (base == 8) val = StrToInt64Oct(buffer + 2);
        else if (base == 16) val = StrToInt64Hex(buffer + 2);
        else val = StrToInt64Dec(buffer);

        token->data.i64 = val;
    }

    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_ParseString(Tokenizer *tokenizer, VToken *token) {
    const uint8_t quote = *tokenizer->token_start;
    const uint8_t *cur = tokenizer->cursor;
    const uint8_t *limit = tokenizer->limit;

    size_t raw_len = 0;
    const uint8_t *scan = cur;
    while (scan < limit) {
        const uint8_t c = *scan;
        if (c == '\\') {
            scan++;
            if (scan >= limit) return VISMUT_ERROR_UNEXPECTED_TOKEN;
            raw_len++;
            scan++;
        } else if (c == quote) {
            break;
        } else {
            raw_len++;
            scan++;
        }
    }
    if (scan >= limit) return ENOENT;

    uint8_t *str_content = Arena_Array(tokenizer->arena, uint8_t, raw_len + 1);
    uint8_t *dst = str_content;

    while (cur < scan) {
        uint8_t c = *cur++;
        if (c == '\\') {
            c = *cur++;
            switch (c) {
                case 'n': *dst++ = '\n';
                    break;
                case 't': *dst++ = '\t';
                    break;
                case 'r': *dst++ = '\r';
                    break;
                case '"': *dst++ = '"';
                    break;
                case '\'': *dst++ = '\'';
                    break;
                case '\\': *dst++ = '\\';
                    break;
                case '\0':
                    *dst++ = '\0';
                    break;
                default: return EILSEQ;
            }
        } else {
            *dst++ = c;
        }
    }
    *dst = '\0';

    tokenizer->cursor = scan + 1;
    token->type = TOKEN_CHARS_LITERAL;
    token->data.chars = str_content;
    token->position.length = (tokenizer->cursor - tokenizer->token_start);

    return VISMUT_ERROR_OK;
}

attribute_pure
static VTokenType CheckKeyword3(const uint8_t *str) {
    if (str[0] == 'i' && str[1] == '6' && str[2] == '4') return TOKEN_I64_TYPE;
    if (str[0] == 'f' && str[1] == '6' && str[2] == '4') return TOKEN_FLOAT_TYPE;
    if (str[0] == 's' && str[1] == 't' && str[2] == 'r') return TOKEN_STRING_TYPE;
    return TOKEN_IDENTIFIER;
}

attribute_hot
errno_t Tokenizer_Next(Tokenizer *restrict tokenizer, VToken *restrict token) {
    const uint8_t *curr = tokenizer->cursor;
    const uint8_t *const limit = tokenizer->limit;

    while (true) {
        while (curr < limit && CharMap[*curr] == CT_SPACE) {
            curr++;
        }
        if (unlikely(curr >= limit)) {
            token->type = TOKEN_EOF;
            token->position.offset = tokenizer->limit - tokenizer->start;
            token->position.length = 0;
            tokenizer->cursor = curr;
            return VISMUT_ERROR_OK;
        }

        tokenizer->token_start = curr;
        token->position.offset = (size_t) (curr - tokenizer->start);

        const uint8_t c = *curr;
        curr++;
        tokenizer->cursor = curr;

        switch (CharMap[c]) {
            case CT_ALPHA: {
                while (curr < limit) {
                    const uint8_t type = CharMap[*curr];
                    if (type == CT_ALPHA || type == CT_DIGIT) {
                        curr++;
                    } else {
                        break;
                    }
                }

                const size_t len = curr - tokenizer->token_start;
                token->position.length = len;
                tokenizer->cursor = curr;

                if (len == 3) {
                    const VTokenType kw = CheckKeyword3(tokenizer->token_start);
                    if (kw != TOKEN_IDENTIFIER) {
                        token->type = kw;
                        return VISMUT_ERROR_OK;
                    }
                }

                uint8_t *id_str = Arena_Array(tokenizer->arena, uint8_t, len + 1);
                memcpy(id_str, tokenizer->token_start, len);
                id_str[len] = '\0';
                token->type = TOKEN_IDENTIFIER;
                token->data.chars = id_str;
                return VISMUT_ERROR_OK;
            }
            case CT_DIGIT:
                return Tokenizer_ParseNumber(tokenizer, token);
            case CT_QUOTES:
                return Tokenizer_ParseString(tokenizer, token);
            case CT_SLASH: {
                if (curr < limit) {
                    const uint8_t next = *curr;
                    if (next == '/') {
                        curr++;
                        if (curr < limit && *curr == '/') {
                            curr++;
                            const uint8_t *eol = memchr(curr, '\n', limit - curr);
                            curr = eol ? eol : limit;
                            continue;
                        }
                        tokenizer->cursor = curr;
                        token->type = TOKEN_INT_DIVIDE;
                        token->position.length = 2;
                        return VISMUT_ERROR_OK;
                    }
                    if (next == '*') {
                        curr++;
                        while (curr + 1 < limit) {
                            if (curr[0] == '*' && curr[1] == '/') {
                                curr += 2;
                                goto MULTILINE_END;
                            }
                            curr++;
                        }
                        return VISMUT_ERROR_UNEXPECTED_TOKEN; // Unclosed
                    MULTILINE_END:
                        // LOOP RESTART
                        continue;
                    }
                }
                // Just Divide '/'
                token->type = TOKEN_DIVIDE;
                token->position.length = 1;
                tokenizer->cursor = curr;
                return VISMUT_ERROR_OK;
            }
            case CT_OPERATOR:
            default: {
                bool wide = false;
                const uint8_t next = (curr < limit) ? *curr : '\0';
                switch (c) {
                    case '(': token->type = TOKEN_LPAREN;
                        break;
                    case ')': token->type = TOKEN_RPAREN;
                        break;
                    case '{': token->type = TOKEN_LBRACE;
                        break;
                    case '}': token->type = TOKEN_RBRACE;
                        break;
                    case '[': token->type = TOKEN_LBRACKET;
                        break;
                    case ']': token->type = TOKEN_RBRACKET;
                        break;
                    case ';': token->type = TOKEN_SEMICOLON;
                        break;
                    case ',': token->type = TOKEN_COMMA;
                        break;
                    case '.': token->type = TOKEN_DOT;
                        break;
                    case '^': token->type = TOKEN_XOR;
                        break;
                    case '~': token->type = TOKEN_TILDA;
                        break;
                    case '?': token->type = TOKEN_QUESTION;
                        break;
                    case '@': token->type = TOKEN_WHILE_STATEMENT;
                        break;
                    case '+':
                        if (next == '+') {
                            wide = true;
                            token->type = TOKEN_INCREMENT;
                        } else token->type = TOKEN_PLUS;
                        break;
                    case '-':
                        if (next == '-') {
                            wide = true;
                            token->type = TOKEN_DECREMENT;
                        } else if (next == '>') {
                            wide = true;
                            token->type = TOKEN_ARROW;
                        } else token->type = TOKEN_MINUS;
                        break;
                    case '*':
                        if (next == '*') {
                            wide = true;
                            token->type = TOKEN_POWER;
                        } else token->type = TOKEN_STAR;
                        break;
                    case '=':
                        if (next == '=') {
                            wide = true;
                            token->type = TOKEN_EQUALS;
                        } else if (next == '>') {
                            wide = true;
                            token->type = TOKEN_THEN;
                        } else token->type = TOKEN_ASSIGN;
                        break;
                    case '!':
                        if (next == '=') {
                            wide = true;
                            token->type = TOKEN_NOT_EQUALS;
                        } else if (next == '#') {
                            wide = true;
                            token->type = TOKEN_CONDITION_ELSE_IF;
                        } else token->type = TOKEN_EXCLAMATION_MARK;
                        break;
                    case '<':
                        if (next == '=') {
                            wide = true;
                            token->type = TOKEN_LESS_THAN_OR_EQUALS;
                        } else if (next == '>') {
                            wide = true;
                            token->type = TOKEN_NAMESPACE_DECLARATION;
                        } else token->type = TOKEN_LESS_THAN;
                        break;
                    case '>':
                        if (next == '=') {
                            wide = true;
                            token->type = TOKEN_GREATER_THAN_OR_EQUALS;
                        } else token->type = TOKEN_GREATER_THAN;
                        break;
                    case '&':
                        if (next == '&') {
                            wide = true;
                            token->type = TOKEN_LOGICAL_AND;
                        } else token->type = TOKEN_BITWISE_AND;
                        break;
                    case '|':
                        if (next == '|') {
                            wide = true;
                            token->type = TOKEN_LOGICAL_OR;
                        } else token->type = TOKEN_BITWISE_OR;
                        break;
                    case '%':
                        if (next == '%') {
                            wide = true;
                            token->type = TOKEN_FOR_STATEMENT;
                        } else token->type = TOKEN_MODULE_DIV;
                        break;
                    case '$':
                        if (next == '>') {
                            wide = true;
                            token->type = TOKEN_STRUCTURE_DECLARATION;
                        } else token->type = TOKEN_NAME_DECLARATION;
                        break;
                    case '#':
                        token->type = TOKEN_CONDITION_STATEMENT;
                        break;
                    case ':':
                        if (next == ':') {
                            wide = true;
                            token->type = TOKEN_PRINT_STATEMENT;
                        } else if (next == '>') {
                            wide = true;
                            token->type = TOKEN_INPUT_STATEMENT;
                        } else token->type = TOKEN_COLON;
                        break;

                    default:
                        return VISMUT_ERROR_UNKNOWN_CHAR;
                }

                if (wide) {
                    curr++;
                    token->position.length = 2;
                } else {
                    token->position.length = 1;
                }

                tokenizer->cursor = curr;
                return VISMUT_ERROR_OK;
            }
        }
    }
}

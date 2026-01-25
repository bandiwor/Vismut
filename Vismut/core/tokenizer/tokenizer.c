//
// Created by kir on 09.10.2025.
//
#include "tokenizer.h"

#include <stdlib.h>
#include <stdbool.h>

#include "../convert.h"
#include "../errors/errors.h"
#include "../../utils/find_position.h"

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
                           Arena *arena, VismutErrorInfo *error_info) {
    if (error_info != NULL) {
        *error_info = (VismutErrorInfo){
            .error = VISMUT_ERROR_OK,
            .line = -1,
            .column = -1,
            .length = -1,
        };
    }
    return (Tokenizer){
        .source_filename = source_filename,
        .start = source,
        .cursor = source,
        .limit = source + source_length,
        .token_start = source,
        .arena = arena,
        .error_info = error_info,
    };
}

attribute_cold
void Tokenizer_Reset(Tokenizer *tokenizer) {
    if (tokenizer->error_info != NULL) {
        *tokenizer->error_info = (VismutErrorInfo){
            .error = VISMUT_ERROR_OK,
            .line = -1,
            .column = -1,
            .length = -1,
        };
    }
    tokenizer->cursor = tokenizer->start;
    tokenizer->token_start = tokenizer->start;
}

static int IsHexDigit(const int digit) {
    return (digit >= '0' && digit <= '9')
           || (digit >= 'a' && digit <= 'f')
           || (digit >= 'A' && digit <= 'F');
}

static int IsDecDigit(const int digit) {
    return digit >= '0' && digit <= '9';
}

static int IsOctDigit(const int digit) {
    return digit >= '0' && digit <= '7';
}

static int IsBinDigit(const int digit) {
    return digit == '0' || digit == '1';
}

typedef enum {
    NB_HEX,
    NB_DEC,
    NB_OCT,
    NB_BIN,
} NumberBase;

static void Tokenizer_SetError(const Tokenizer *tokenizer, const VismutError err_code, const uint8_t *error_location,
                               const int length, const VismutErrorDetails details) {
    if (tokenizer->error_info == NULL) return;
    const TextPosition error_position = FindPosition(tokenizer->start, tokenizer->limit, error_location);
    tokenizer->error_info->error = err_code;
    tokenizer->error_info->source = tokenizer->start;
    tokenizer->error_info->source_length = tokenizer->limit - tokenizer->start;
    tokenizer->error_info->module = tokenizer->source_filename;
    tokenizer->error_info->column = (int) error_position.column;
    tokenizer->error_info->line = (int) error_position.line;
    tokenizer->error_info->location = error_location;
    tokenizer->error_info->length = length;
    tokenizer->error_info->details = details;
}

static errno_t Tokenizer_ParseNumber(Tokenizer *tokenizer, VToken *token) {
    const uint8_t *start = tokenizer->token_start;
    const uint8_t *cur = tokenizer->cursor;
    const uint8_t *limit = tokenizer->limit;

    NumberBase base = NB_DEC;
    if (*start == '0' && cur < limit) {
        const uint8_t c = *cur;
        if (c == 'x' || c == 'X') {
            base = NB_HEX;
            cur++;
        } else if (c == 'b' || c == 'B') {
            base = NB_BIN;
            cur++;
        } else if (c == 'o' || c == 'O') {
            base = NB_OCT;
            cur++;
        }
    }

    bool is_float = false;
    switch (base) {
        case NB_HEX:
            while (cur < limit && IsHexDigit(*cur)) cur++;
            break;
        case NB_OCT:
            while (cur < limit && IsOctDigit(*cur)) cur++;
            break;
        case NB_BIN:
            while (cur < limit && IsBinDigit(*cur)) cur++;
            break;
        case NB_DEC: {
            bool has_dot = false;
            while (cur < limit) {
                const uint8_t c = *cur;
                if (IsDecDigit(c)) {
                    cur++;
                    continue;
                }
                if (c == '.' && !has_dot) {
                    cur++;
                    has_dot = true;
                    is_float = true;
                    continue;
                }
                if (c == 'e' || c == 'E') {
                    cur++;
                    is_float = true;
                    if (cur < limit && (*cur == '+' || *cur == '-')) cur++;
                    while (cur < limit && IsDecDigit(*cur)) ++cur;
                    break;
                }
                break;
            }
        }
    }

    const size_t length = cur - start;

    uint8_t stack_buf[64];
    uint8_t *buffer;

    if (likely(length < _countof(stack_buf))) {
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

    if (is_float) {
        token->type = TOKEN_FLOAT_LITERAL;
        errno = 0;
        token->data.f64 = strtod((const char *) buffer, (char **) &end_ptr);
        if (errno == ERANGE) {
            Tokenizer_SetError(tokenizer, VISMUT_ERROR_NUMBER_OVERFLOW, tokenizer->token_start,
                               (int) (end_ptr - tokenizer->token_start), (VismutErrorDetails){0});
            return VISMUT_ERROR_NUMBER_OVERFLOW;
        }
        if (end_ptr == buffer) return VISMUT_ERROR_NUMBER_PARSE;
        return VISMUT_ERROR_OK;
    }

    int64_t val = 0;
    switch (base) {
        case NB_HEX:
            val = StrToInt64Hex(buffer + 2);
            break;
        case NB_DEC:
            val = StrToInt64Dec(buffer);
            break;
        case NB_OCT:
            val = StrToInt64Oct(buffer + 2);
            break;
        case NB_BIN:
            val = StrToInt64Bin(buffer + 2);
            break;
    }

    token->type = TOKEN_INT_LITERAL;
    token->data.i64 = val;

    return VISMUT_ERROR_OK;
}

attribute_noinline
static errno_t Tokenizer_ParseString(Tokenizer *tokenizer, VToken *token) {
    const uint8_t quote = *tokenizer->token_start;
    const uint8_t *cur = tokenizer->cursor;
    const uint8_t *limit = tokenizer->limit;

    size_t raw_len = 0;
    const uint8_t *scan = cur;
    while (scan < limit) {
        const uint8_t c = *scan;
        if (unlikely(c == quote)) break;
        if (unlikely(c == '\\')) {
            if (unlikely(scan >= limit)) {
                Tokenizer_SetError(tokenizer, VISMUT_ERROR_UNEXPECTED_SYMBOL, scan, 1,
                                   (VismutErrorDetails){.unexpected_symbol.caught = *scan});
                return VISMUT_ERROR_UNEXPECTED_SYMBOL;
            }
            scan++;
        }

        raw_len++;
        scan++;
    }
    if (scan >= limit) return ENOENT;

    uint8_t *str_content = Arena_Array(tokenizer->arena, uint8_t, raw_len + 1);
    uint8_t *dst = str_content;

    while (cur < scan) {
        uint8_t c = *cur++;
        if (unlikely(c == '\\')) {
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
                default:
                    Tokenizer_SetError(tokenizer, EILSEQ, --cur, 1, (VismutErrorDetails){0});
                    return EILSEQ;
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
#define MAKE_CODE3_BE(a, b, c) (((a) << 16) | ((b) << 8) | (c))
    const int code = MAKE_CODE3_BE(str[0], str[1], str[2]);
    switch (code) {
        case MAKE_CODE3_BE('i', '6', '4'): return TOKEN_I64_TYPE;
        case MAKE_CODE3_BE('f', '6', '4'): return TOKEN_FLOAT_TYPE;
        case MAKE_CODE3_BE('s', 't', 'r'): return TOKEN_STRING_TYPE;
        default:
            return TOKEN_IDENTIFIER;
    }
#undef MAKE_CODE3_BE
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
                            const uint8_t *eol = __builtin_memchr(curr, '\n', limit - curr);
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
                        Tokenizer_SetError(tokenizer, VISMUT_ERROR_UNEXPECTED_SYMBOL, curr, 1,
                                           (VismutErrorDetails){.unexpected_symbol.caught = *curr});
                        return VISMUT_ERROR_UNEXPECTED_SYMBOL; // Unclosed
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
                        switch (next) {
                            case ':':
                                wide = true;
                                token->type = TOKEN_PRINT_STATEMENT;
                                break;
                            case '>':
                                wide = true;
                                token->type = TOKEN_INPUT_STATEMENT;
                                break;
                            default:
                                token->type = TOKEN_COLON;
                                break;
                        }
                        break;
                    default:
                        Tokenizer_SetError(tokenizer, VISMUT_ERROR_UNKNOWN_SYMBOL, tokenizer->token_start, 1,
                                           (VismutErrorDetails){.unknown_symbol.caught = *tokenizer->token_start});
                        return VISMUT_ERROR_UNKNOWN_SYMBOL;
                }
                token->position.length = 1;
                if (unlikely(wide)) {
                    ++curr;
                    ++token->position.length;
                }
                tokenizer->cursor = curr;
                return VISMUT_ERROR_OK;
            }
        }
    }
}

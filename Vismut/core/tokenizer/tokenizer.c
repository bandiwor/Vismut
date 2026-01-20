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
Tokenizer Tokenizer_Create(const wchar_t *source, const size_t source_length, wchar_t *source_filename,
                           Arena *arena) {
    Tokenizer tokenizer = {
        .source = source,
        .source_length = source_length,
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

attribute_const attribute_hot
static bool IsSpace(const wchar_t c) {
    return c == L' ' || c == L'\n' || c == L'\t' || c == L'\r' || c == L'\r';
}

attribute_const
static bool IsDigit(const wchar_t c) {
    return c >= L'0' && c <= L'9';
}

attribute_const attribute_hot
static bool IsAlphaOrUnderscore(const wchar_t c) {
    return ((c >= L'a' && c <= L'z') || // Частые строчные латинские
            (c == L'_') || // Подчеркивание
            (c >= L'A' && c <= L'Z') || // Заглавные латинские
            (c >= 0x410 && c <= 0x44F) || // Кириллица (основной блок)
            (c == 0x401 || c == 0x451) || // Ё и ё
            ((c & 0xFFE0) == 0x0400) || // Расширенная кириллица (0400-04FF)
            ((c >= 0x370 && c <= 0x3FF)) || // Греческие буквы
            ((c & 0xF800) == 0x3000) // Общий диапазон CJK иероглифов
    );
}

attribute_hot
static void Tokenizer_Advance(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (likely(tokenizer->current_position + 1 < tokenizer->source_length)) {
        tokenizer->current_char = tokenizer->source[tokenizer->current_position++ + 1];
    } else {
        tokenizer->current_char = L'\0';
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
static wchar_t Tokenizer_NextChar(const Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);

    if (likely(tokenizer->current_position + 1 < tokenizer->source_length)) {
        return tokenizer->source[tokenizer->current_position + 1];
    }

    return L'\0';
}

static void Tokenizer_SkipSingleComment(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_LOG_ASSERT(
        tokenizer->current_char == L'/' && Tokenizer_NextChar(tokenizer) == L'/',
        L"current char and next char must to be equal '/'"
    );

    // Skip '//'
    Tokenizer_DoubleAdvance(tokenizer);
    // Skip line
    while (tokenizer->current_char != L'\n' && tokenizer->current_char != L'\0') {
        Tokenizer_Advance(tokenizer);
    }
    // Skip '\n'
    Tokenizer_Advance(tokenizer);
}

static void Tokenizer_SkipMultiLineComment(Tokenizer *tokenizer) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_LOG_ASSERT(
        tokenizer->current_char == L'/' && Tokenizer_NextChar(tokenizer) == L'*',
        L"current char must to be equal '/', next char must to be equal '*'"
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
static VTokenType Tokenizer_GetKeyword(const wchar_t *buffer,
                                       const size_t length) {
#define COMPARE_3(c1, c2, c3, token)                                           \
  do {                                                                         \
    if (buffer[0] == (c1) && buffer[1] == (c2) && buffer[2] == (c3))           \
      return token;                                                            \
  } while (0)

    if (length == 3) {
        COMPARE_3(L'i', L'6', L'4', TOKEN_I64_TYPE);
        COMPARE_3(L'f', L'6', L'4', TOKEN_FLOAT_TYPE);
        COMPARE_3(L's', L't', L'r', TOKEN_STRING_TYPE);
        return TOKEN_UNKNOWN;
    }

    return TOKEN_UNKNOWN;

#undef COMPARE_3
}

static errno_t Tokenizer_ScanIdentifierIntoBuffer(Tokenizer *tokenizer, wchar_t *buffer, const size_t buffer_size,
                                                  size_t *keyword_length) {
    size_t length = 1;
    buffer[0] = tokenizer->current_char;

    Tokenizer_Advance(tokenizer);

    while (IsAlphaOrUnderscore(tokenizer->current_char) || IsDigit(tokenizer->current_char)) {
        if (length >= buffer_size) {
            return VISMUT_ERROR_BUFFER_OVERFLOW;
        }
        buffer[length++] = tokenizer->current_char;
        Tokenizer_Advance(tokenizer);
    }

    *keyword_length = length;
    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_ParseIdentifier(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);
    DEBUG_ASSERT(IsAlphaOrUnderscore(tokenizer->current_char));

    static wchar_t buffer[128] = {0};
    size_t keyword_length;

    errno_t err;
    if ((err = Tokenizer_ScanIdentifierIntoBuffer(tokenizer, buffer, _countof(buffer), &keyword_length)) !=
        VISMUT_ERROR_OK) {
        return err;
    }

    token->position.length = keyword_length;

    VTokenType keyword_type;
    if ((keyword_type = Tokenizer_GetKeyword(buffer, keyword_length)) != TOKEN_UNKNOWN) {
        token->type = keyword_type;
        return VISMUT_ERROR_OK;
    }

    wchar_t *identifier = Arena_Array(tokenizer->arena, wchar_t, keyword_length + 1);
    wmemcpy_s(identifier, keyword_length, buffer, keyword_length);
    identifier[keyword_length] = L'\0';

    token->type = TOKEN_IDENTIFIER;
    token->data.w_chars = identifier;

    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_ParseNumber(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);
    DEBUG_ASSERT(
        IsDigit(tokenizer->current_char) ||
        (tokenizer->current_char == L'-' && IsDigit(Tokenizer_NextChar(tokenizer))))
    ;

    static wchar_t buffer[128] = {0};
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

    if (buffer[0] == L'0') {
        switch (tokenizer->current_char) {
            case L'x':
            case L'X':
                base = 16;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            case L'o':
            case L'O':
                base = 8;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            case L'b':
            case L'B':
                base = 2;
                buffer_length = 0;
                Tokenizer_Advance(tokenizer);
                break;
            default:
                break;
        }
    }

    while (tokenizer->current_char != L'\0') {
        if (unlikely(buffer_length + 1 >= _countof(buffer))) {
            return VISMUT_ERROR_BUFFER_OVERFLOW;
        }

        wchar_t c = tokenizer->current_char;
        bool is_valid = false;

        if (base == 16) {
            is_valid = (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f');
        } else if (base == 8) {
            is_valid = c >= L'0' && c <= L'7';
        } else if (base == 2) {
            is_valid = c == L'0' || c == L'1';
        } else {
            is_valid = true;
            if (c >= L'0' && c <= L'9') {
            } else if (c == L'.' && !has_point && !has_exponent) {
                has_point = true;
            } else if (towlower(c) == L'e' && !has_exponent) {
                has_exponent = true;
                buffer[buffer_length++] = c;
                Tokenizer_Advance(tokenizer);
                c = tokenizer->current_char;
                if (c == L'+' || c == L'-') {
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
    buffer[buffer_length] = L'\0';

    wchar_t *end_ptr;
    errno = 0;

    if (base != 10) {
        int64_t num = 0;

        switch (base) {
            case 2:
                num = WStrToInt64Bin(buffer);
                break;
            case 8:
                num = WStrToInt64Oct(buffer);
                break;
            case 16:
                num = WStrToInt64Hex(buffer);
                break;
            default:
                DEBUG_ASSERT("Unreachable code");
        }

        token->type = TOKEN_INT_LITERAL;
        token->data.i64 = is_negative ? -num : num;
    } else {
        if (has_point || has_exponent) {
            const double num = wcstod(buffer, &end_ptr);
            if (*end_ptr != L'\0' || errno == ERANGE) {
                return (errno == ERANGE) ? VISMUT_ERROR_NUMBER_OVERFLOW : VISMUT_ERROR_NUMBER_PARSE;
            }
            token->type = TOKEN_FLOAT_LITERAL;
            token->data.f64 = is_negative ? -num : num;
        } else {
            const long long num = wcstoll(buffer, &end_ptr, 10);
            if (*end_ptr != L'\0' || errno == ERANGE) {
                return (errno == ERANGE) ? VISMUT_ERROR_NUMBER_OVERFLOW : VISMUT_ERROR_NUMBER_PARSE;
            }
            token->type = TOKEN_INT_LITERAL;
            token->data.i64 = is_negative ? -num : num;
        }
    }

    token->position.length = tokenizer->current_position - start_position;

    return VISMUT_ERROR_OK;
}

static errno_t Tokenizer_CountStringLength(Tokenizer *tokenizer, const wchar_t string_symbol, size_t *out_length) {
    const size_t start_position = tokenizer->current_position;
    size_t string_length = 0;
    bool escaped_symbol = false;

    while (tokenizer->current_char != L'\0') {
        if (tokenizer->current_char == L'\\') {
            if (!escaped_symbol) {
                escaped_symbol = true;
                Tokenizer_Advance(tokenizer);
                continue;
            }
            escaped_symbol = false;
            string_length++;
        } else if (tokenizer->current_char == string_symbol) {
            if (!escaped_symbol) {
                Tokenizer_Advance(tokenizer); // Пропускаем закрывающую кавычку
                break;
            }
            escaped_symbol = false;
            string_length++;
        } else {
            if (escaped_symbol) {
                switch (tokenizer->current_char) {
                    case L'n': // newline
                    case L't': // tab
                    case L'r': // carriage return
                    // case L'0':  // null character
                    case L'"': // double quote
                    case L'\'': // single quote
                    case L'\\': // backslash
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

    if (tokenizer->current_char == L'\0' && !escaped_symbol) {
        return ENOENT;
    }

    *out_length = string_length;
    return 0;
}

static errno_t Tokenizer_ParseString(Tokenizer *tokenizer, VToken *token) {
    DEBUG_ASSERT(tokenizer != NULL);
    DEBUG_ASSERT(token != NULL);

    const wchar_t string_symbol = tokenizer->current_char;
    const size_t start_position = tokenizer->current_position;

    Tokenizer_Advance(tokenizer); // Пропускаем открывающую кавычку
    token->type = TOKEN_CHARS_LITERAL;

    size_t string_length = 0;
    errno_t count_result;

    if ((count_result = Tokenizer_CountStringLength(tokenizer, string_symbol, &string_length)) != 0) {
        tokenizer->current_position = start_position;
        return count_result;
    }

    // Выделяем память для строки
    wchar_t *string = Arena_Array(tokenizer->arena, wchar_t, string_length + 1);
    if (string == NULL) {
        tokenizer->current_position = start_position;
        return ENOMEM;
    }

    Tokenizer_SetPosition(tokenizer, start_position);
    Tokenizer_Advance(tokenizer);

    size_t index = 0;
    bool escaped_symbol = false;

    while (tokenizer->current_char != L'\0') {
        if (tokenizer->current_char == L'\\') {
            if (!escaped_symbol) {
                escaped_symbol = true;
                Tokenizer_Advance(tokenizer);
                continue;
            }
            escaped_symbol = false;
            string[index++] = L'\\';
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
                    case L'n': string[index++] = L'\n';
                        break;
                    case L't': string[index++] = L'\t';
                        break;
                    case L'r': string[index++] = L'\r';
                        break;
                    case L'"': string[index++] = L'"';
                        break;
                    case L'\'': string[index++] = L'\'';
                        break;
                    case L'\\': string[index++] = L'\\';
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

    string[index] = L'\0';

    if (index != string_length) {
        free(string);
        tokenizer->current_position = start_position;
        return EILSEQ;
    }

    token->data.w_chars = string;
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

    if (tokenizer->current_char == L'\0') {
        token->type = TOKEN_EOF;
        return VISMUT_ERROR_OK;
    }

    const wchar_t current_char = tokenizer->current_char;
    const wchar_t next_char = Tokenizer_NextChar(tokenizer);
    token->position.offset = tokenizer->current_position;

    if (IsAlphaOrUnderscore(current_char)) {
        return Tokenizer_ParseIdentifier(tokenizer, token);
    }
    if (IsDigit(current_char) || (current_char == L'-' && IsDigit(next_char))) {
        return Tokenizer_ParseNumber(tokenizer, token);
    }

    if (current_char == L'/') {
        if (next_char == L'/') {
            Tokenizer_SkipSingleComment(tokenizer);
            goto Tokenizer_Next_start;
        }
        if (next_char == L'*') {
            Tokenizer_SkipMultiLineComment(tokenizer);
            goto Tokenizer_Next_start;
        }
    }

    if (current_char == L'\"') {
        return Tokenizer_ParseString(tokenizer, token);
    }

#define CASE_1(wchar, type_)\
    case wchar: type = type_;\
    break;
#define CASE_1_2(wchar1, wchar2, type1, type2) \
    case wchar1: {  \
        if (next_char == wchar2) {  \
            is_wide_token = true;   \
            type = type2;   \
            Tokenizer_Advance(tokenizer);   \
            break;  \
        }   \
        type = type1;   \
        break;  \
    }
#define CASE_1_2_2(wchar1, wchar2_1, wchar2_2, type1, type2_1, type2_2) \
    case wchar1: {  \
        if (next_char == wchar2_1) {  \
            is_wide_token = true;   \
            type = type2_1;   \
            Tokenizer_Advance(tokenizer);   \
            break;  \
        }   \
        if (next_char == wchar2_2) {  \
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
        CASE_1(L'{', TOKEN_LBRACE)
        CASE_1(L'}', TOKEN_RBRACE)
        CASE_1(L'[', TOKEN_LBRACKET)
        CASE_1(L']', TOKEN_RBRACKET)
        CASE_1(L'(', TOKEN_LPAREN)
        CASE_1(L')', TOKEN_RPAREN)
        CASE_1(L'.', TOKEN_DOT)
        CASE_1(L',', TOKEN_COMMA)
        CASE_1(L';', TOKEN_SEMICOLON)
        CASE_1(L'^', TOKEN_XOR)
        CASE_1(L'~', TOKEN_TILDA)
        CASE_1(L'?', TOKEN_QUESTION)

        CASE_1(L'@', TOKEN_WHILE_STATEMENT)

        CASE_1_2(L'#', L'!', TOKEN_CONDITION_STATEMENT, TOKEN_CONDITION_ELSE_IF)
        CASE_1_2(L'*', L'*', TOKEN_STAR, TOKEN_POWER)
        CASE_1_2(L'!', L'=', TOKEN_EXCLAMATION_MARK, TOKEN_NOT_EQUALS)
        CASE_1_2(L'>', L'=', TOKEN_GREATER_THAN, TOKEN_GREATER_THAN_OR_EQUALS)
        CASE_1_2(L'$', L'>', TOKEN_NAME_DECLARATION, TOKEN_STRUCTURE_DECLARATION)
        CASE_1_2(L'+', L'+', TOKEN_PLUS, TOKEN_INCREMENT)
        CASE_1_2(L'/', L'/', TOKEN_DIVIDE, TOKEN_INT_DIVIDE)
        CASE_1_2(L'%', L'%', TOKEN_MODULE_DIV, TOKEN_FOR_STATEMENT)
        CASE_1_2(L'|', L'|', TOKEN_BITWISE_OR, TOKEN_LOGICAL_OR)
        CASE_1_2(L'&', L'&', TOKEN_BITWISE_AND, TOKEN_LOGICAL_OR)

        CASE_1_2_2(L':', L':', L'>', TOKEN_COLON, TOKEN_PRINT_STATEMENT, TOKEN_INPUT_STATEMENT);
        CASE_1_2_2(L'<', L'=', L'>', TOKEN_LESS_THAN, TOKEN_LESS_THAN_OR_EQUALS, TOKEN_NAMESPACE_DECLARATION)
        CASE_1_2_2(L'=', L'=', L'>', TOKEN_ASSIGN, TOKEN_EQUALS, TOKEN_THEN)
        CASE_1_2_2(L'-', L'>', L'-', TOKEN_MINUS, TOKEN_ARROW, TOKEN_DECREMENT)

        default:
            return VISMUT_ERROR_UNKNOWN_CHAR;
    }
    token->position.length = is_wide_token ? 2 : 1;
    token->type = type;
    return VISMUT_ERROR_OK;
}

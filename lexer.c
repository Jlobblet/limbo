#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "lexer.h"
#include "error.h"
#include "unicode.h"

static char *KEYWORDS[] = {
        "adt", "alt", "array", "big", "break", "byte", "case", "chan", "con",
        "continue", "cyclic", "do", "else","exit", "fn", "for", "hd", "if",
        "implement", "import", "include", "int", "len", "list", "load", "module",
        "nil", "of", "or", "pick", "real", "ref", "return", "self", "spawn",
        "string", "tagof", "tl", "to", "type","while",
};

static void Token_new(Token *self, LexerContext *context, TokenKind kind,
                      char *start, const char *end) {
    self->kind = kind;
    self->next = NULL;
    self->location = start;
    self->length = end - start;
    self->string_value = NULL;
    self->at_beginning_of_line = context->at_beginning_of_line;
    self->follows_space = context->follows_space;
    self->source_file = context->source_file;
}

static i32 digit(char c, i32 base) {
    i32 value;
    if (isdigit(c)) {
        value = c - '0';
    } else if (isupper(c)) {
        value = c - 'A' + 10;
    } else if (islower(c)) {
        value = c - 'a' + 10;
    } else {
        value = -1;
    }
    if (value >= base) {
        value = -1;
    }
    return value;
}

f64 strtodb(char *str, i32 base) {
    f64 number = 0.0;
    bool negative = false, exponent_negative = false;
    i32 decimal_digits = 0, exponent = 0, d;
    char c;

    c = *str++;
    if (c == '-' || c == '+') {
        negative = c == '-';
        c = *str++;
    }

    while ((d = digit(c, base)) >= 0) {
        number = number * base + d;
        c = *str++;
    }

    if (c == '.') { c = *str++; }

    while ((d = digit(c, base)) >= 0) {
        number = number * base + d;
        decimal_digits++;
        c = *str++;
    }

    if (c == 'e' || c == 'E') {
        c = *str++;
        if(c == '-' || c == '+') {
            if(c == '-') {
                decimal_digits = -decimal_digits;
                exponent_negative = true;
            }
            c = *str++;
        }

        while ((d = digit(c, base)) >= 0) {
            exponent = exponent * base + d;
            c = *str++;
        }
    }

    exponent -= decimal_digits;
    if (exponent < 0) {
        exponent = -exponent;
        exponent_negative = !exponent_negative;
    }
    f64 dem = pow(base, exponent);

    if (exponent_negative) { number /= dem; }
    else { number *= dem; }

    if (negative) { number = -number; }

    return number;
}

Token *read_number_literal(LexerContext *context, char *start, char **new_position) {
    // Decimal integer constants consist of a sequence of decimal digits. A
    // constant with an explicit radix consists of a decimal radix followed by
    // `R` or `r` followed by the digits of the number. The radix is between
    // 2 and 36 inclusive; digits above 10 in the number are expressed using
    // letters `A` to `Z` or `a` to `z`. For example, `16r20` has value 32.
    //
    // Real constants consist of a sequence of decimal digits containing one
    // period `.` and optionally followed by `e` or `E` and then by a possibly
    // signed integer. If there is an explicit exponent, the period is not
    // required.

    // in regex: [0-9]+(r[0-9A-Za-z]+)? or ([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][+-]?[0-9]+)?

    char *p = start;
    uptr length = 0, capacity = 16;
    char *buffer = calloc(capacity, sizeof(char)), *base;

    enum { INTEGER, RADIX_CHAR, RADIX, FRACTION, FRACTION_B,
            EXPONENT_CHAR, EXPONENT_SIGN, EXPONENT } state;
    state = INTEGER;

    if (*p == '.') {
        state = FRACTION;
    }

    buffer[length++] = *p;

    while (true) {
        p++;
        char c = *p;
        if (c == '\0') {
            break;
        }

        switch (state) {
            case INTEGER:
                if (isdigit(c)) break;
                if (c == 'e' || c == 'E') {
                    state = EXPONENT_CHAR;
                    break;
                }
                if (c == '.') {
                    state = FRACTION;
                    break;
                }
                if (c == 'r' || c == 'R') {
                    state = RADIX_CHAR;
                    base = &buffer[length];
                    break;
                }
                goto finished;

            case RADIX_CHAR:
            case RADIX:
                if (isdigit(c)) {
                    state = RADIX;
                    break;
                }
                if (c == '.') {
                    state = FRACTION_B;
                    break;
                }
                goto finished;

            case FRACTION:
                if (isdigit(c)) break;
                if (c == 'e' || c == 'E') state = EXPONENT_SIGN;
                else goto finished;
                break;

            case FRACTION_B:
                if (isdigit(c) || isalpha(c)) break;
                goto finished;

            case EXPONENT_CHAR:
                if (c == '+' || c == '-') {
                    state = EXPONENT_SIGN;
                    break;
                }
                // fallthrough

            case EXPONENT_SIGN:
            case EXPONENT:
                if (isdigit(c)) {
                    state = EXPONENT;
                    break;
                }
                goto finished;

            default:
                error("internal compiler error: unexpected state in number literal %d", state);
        }

        // grow buffer if necessary
        if (length >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity * sizeof(char));
        }
        // add character to buffer
        buffer[length++] = c;
    }

    finished:

    buffer[length] = 0;
    // Backtrack one place
    p--;

    i64 int_value = 0;
    f64 real_value = 0.0;

    switch (state) {
        default:
            error_at(context->source_file, start, "malformed number literal");

        case RADIX:
            *base++ = '\0';
            // Parse the base
            int_value = strtoll(buffer, NULL, 10);
            // validate
            if (int_value < 2 || int_value > 36) {
                error_at(context->source_file, start, "invalid radix in number literal");
            }
            // Parse the number
            int_value = strtoll(base, NULL, (int)int_value);
            break;

        case INTEGER:
            int_value = strtoll(buffer, NULL, 10);
            break;

        case FRACTION:
        case EXPONENT:
            real_value = strtod(buffer, NULL);
            break;

        case FRACTION_B:
            int_value = strtoll(base, NULL, 10);
            if (int_value < 2 || int_value > 36) {
                error_at(context->source_file, start, "invalid radix in number literal");
            }
            real_value = strtodb(buffer, (int)int_value);
            break;
    }

    Token *token = calloc(1, sizeof(Token));
    Token_new(token, context, TOKEN_NUMBER, start, p);
    token->int_value = int_value;
    token->real_value = real_value;
    *new_position = p;
    return token;
}

/// Read an escape sequence.
/// \param context The lexer context.
/// \param position The starting position in the source file.
/// \param new_position The position to update to after the escape sequence.
/// \return The escape sequence.
/// \remark This function implements escape sequences in terms of their C
/// counterparts.
static char read_escaped_character(LexerContext *context, char *position, char **new_position) {
    *new_position = position + 1;
    switch (*position) {
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        case 'a': return '\a';
        case 'b': return '\b';
        case 't': return '\t';
        case 'n': return '\n';
        case 'v': return '\v';
        case 'f': return '\f';
        case 'r': return '\r';
        case '0': return '\0';
        case 'u': error_at(context->source_file, position, "Unicode escape sequences are not yet supported");
        default: error_at(context->source_file, position, "Invalid escape sequence");
    }
}

/// Search for a closing double quote " for a string literal.
/// \param context The lexer context.
/// \param position The starting position in the source file.
/// \return The position of the closing double quote.
/// \remark This function will exit the program if the string literal is invalid.
static char *string_literal_end(LexerContext *context, char *position) {
    char *p = position;
    while (*p != '"') {
        if (*p == '\0' || *p == '\n') {
            error_at(context->source_file, position, "unterminated string literal");
        }
        if (*p == '\\') {
            p++;
        }
        p++;
    }
    return p;
}

/// Read a string literal into a token.
/// \param context The lexer context.
/// \param start The starting position in the source file.
/// \return The token.
static Token *read_string_literal(LexerContext *context, char *start, char **new_position) {
    char *end = string_literal_end(context, start + 1);
    char *buffer = calloc(1, end - start + 1);
    uptr len = 0;

    for (char *p = start + 1; p < end;) {
        if (*p == '\\') {
            buffer[len++] = read_escaped_character(context, p + 1, &p);
        } else {
            buffer[len++] = *p++;
        }
    }

    Token *token = calloc(1, sizeof(Token));
    Token_new(token, context, TOKEN_STRING, start, end);
    token->string_value = buffer;
    token->length = len;

    *new_position = end + 1;

    return token;
}

/// Read a character literal into a token.
/// In Limbo, character literals are always represented as type `int`.
/// \param context The lexer context.
/// \param start The starting position in the source file.
/// \return The token.
static Token *read_char_literal(LexerContext *context, char *start, char **new_position) {
    char *position = start + 1;
    if (*position == '\0') {
        error_at(context->source_file, position, "unterminated character literal");
    }

    u32 c;
    if (*position == '\\') {
        c = (unsigned char) read_escaped_character(context, position + 1, &position);
    } else {
        c = utf8_decode(&position, position);
    }

    char *end = strchr(position, '\'');
    if (!end) {
        error_at(context->source_file, position, "unterminated character literal");
    }

    Token *token = calloc(1, sizeof(Token));
    Token_new(token, context, TOKEN_NUMBER, start, end);
    token->int_value = c;

    *new_position = end + 1;

    return token;
}

bool lex(SourceFile *file, Token **tokens) {
    char *p = file->contents;
    Token last_token = {}, *current = &last_token;

    LexerContext context = {
        .source_file = file,
        .at_beginning_of_line = true,
        .follows_space = false
    };

    while (*p) {
        // Skip comments
        if (*p == '#') {
            p++;
            // Advance to the end of the line
            while (*p && *p != '\n') {
                p++;
            }
            context.follows_space = true;
            continue;
        }

        // Skip newlines
        if (*p == '\n') {
            p++;
            context.at_beginning_of_line = true;
            context.follows_space = false;
            continue;
        }

        // Skip whitespace
        if (isspace(*p)) {
            p++;
            context.follows_space = true;
            continue;
        }

        // Tokens start here

        // Number literal
        if (isdigit(*p) || *p == '.' && isdigit(p[1])) {
            current = current->next = read_number_literal(&context, p, &p);
        }

        // String literal
        if (*p == '"') {
            current = current->next = read_string_literal(&context, p, &p);
        }

        // Character literal
        if (*p == '\'') {
            current = current->next = read_char_literal(&context, p, &p);
        }

        // Keyword

        // Identifier

        // Punctuator

    }

    // End of file
}

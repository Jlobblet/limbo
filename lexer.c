#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "lexer.h"
#include "error.h"
#include "unicode.h"

static char *KEYWORDS[] = {
        "implement", "continue", "include", "cyclic", "import", "module",
        "return", "string", "array", "break", "spawn", "tagof", "while", "byte",
        "case", "chan", "else", "exit", "list", "load", "pick", "real", "self",
        "type", "adt", "alt", "big", "con", "for", "int", "len", "nil", "ref",
        "do", "fn", "hd", "if", "of", "or", "tl", "to",
};
static int KEYWORD_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

static char *PUNCT[] = {
        "<<=", ">>=", "==", "<=", ">=", "!=", "<<", ">>", "&&", "||", "<-",
        "::", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ":=", "++", "--",
        "**", "->", "=>", "+", "-", "*", "/", "%", "&", "|", "^", "<", ">",
        "=", "~", "!", ":", ";", "(", ")", "{", "}", "[", "]", ",", ".",
};
static int PUNCT_COUNT = sizeof(PUNCT) / sizeof(PUNCT[0]);

/// Create a new token.
/// \param self Pointer to the token to initialise.
/// \param context The lexer context.
/// \param kind The kind of token.
/// \param start The start of the token.
/// \param end The end of the token.
static void Token_new(Token *self, LexerContext *context, TokenKind kind,
                      const char *start, const char *end) {
    self->kind = kind;
    self->next = NULL;
    self->location = start;
    self->length = end - start;

    self->string_value = NULL;
    self->int_value = 0;
    self->real_value = 0;

    self->source_file = context->source_file;
    self->source_file_name = context->source_file->name;
    self->source_file_line = context->line_number;
    self->source_file_column = context->column_number;
    self->at_beginning_of_line = context->at_beginning_of_line;
    self->follows_space = context->follows_space;
}

/// Calculate the value of a digit in the given base.
/// \param c The digit to calculate the value of.
/// \param base The base to use.
/// \return The value of the digit in the given base, or `-1` if it is invalid.
/// \remark The base must be between 2 and 36.
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

/// Convert a string into a floating point number with the specified base.
/// \param nptr The string to convert.
/// \param endptr A pointer that is set to the end of the parsed number.
/// \param base The base of the number.
/// \return The floating point value of the string.
/// \remark The base must be between 2 and 36.
/// \remark The characters A-Z and a-z are treated as 10-35.
f64 strtodb(const char *nptr, const char **endptr, i32 base) {
    f64 number = 0.0;
    bool negative = false, exponent_negative = false;
    i32 decimal_digits = 0, exponent = 0, d;
    char c;

    c = *nptr++;
    if (c == '-' || c == '+') {
        negative = c == '-';
        c = *nptr++;
    }

    while ((d = digit(c, base)) >= 0) {
        number = number * base + d;
        c = *nptr++;
    }

    if (c == '.') { c = *nptr++; }

    while ((d = digit(c, base)) >= 0) {
        number = number * base + d;
        decimal_digits++;
        c = *nptr++;
    }

    if (c == 'e' || c == 'E') {
        c = *nptr++;
        if(c == '-' || c == '+') {
            if(c == '-') {
                decimal_digits = -decimal_digits;
                exponent_negative = true;
            }
            c = *nptr++;
        }

        while ((d = digit(c, base)) >= 0) {
            exponent = exponent * base + d;
            c = *nptr++;
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

    if (endptr) { *endptr = nptr; }

    return number;
}

/// Read a number literal.
/// \param context The lexer context.
/// \param token The token to initialise.
/// \param start The starting position in the source file.
/// \param new_position The position to update to after the number.
/// \note
///     Decimal integer constants consist of a sequence of decimal digits. A
///     constant with an explicit radix consists of a decimal radix followed by
///     `R` or `r` followed by the digits of the number. The radix is between
///     2 and 36 inclusive; digits above 10 in the number are expressed using
///     letters `A` to `Z` or `a` to `z`. For example, `16r20` has value 32.
///
///     Real constants consist of a sequence of decimal digits containing one
///     period `.` and optionally followed by `e` or `E` and then by a possibly
///     signed integer. If there is an explicit exponent, the period is not
///     required.
static void read_number_literal(LexerContext *context, Token *token,
                                const char *start, const char **new_position) {
    // in regex: [0-9]+(r[0-9A-Za-z]+)?
    // or ([0-9]+(\.[0-9]*)?|\.[0-9]+)([eE][+-]?[0-9]+)?

    const char *p = start;
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
                if (isdigit(c) || isalpha(c)) {
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
                error("internal compiler error: unexpected state in number literal %d",
                      state);
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

    i64 int_value;
    f64 real_value;

    switch (state) {
        default:
            error_at(context->source_file, start, "malformed number literal");

        case RADIX:
            *base++ = '\0';
            // Parse the base
            int_value = strtoll(buffer, NULL, 10);
            // validate
            if (int_value < 2 || int_value > 36) {
                error_at(context->source_file, start,
                         "invalid radix in number literal");
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
            *base++ = '\0';
            int_value = strtoll(buffer, NULL, 10);
            if (int_value < 2 || int_value > 36) {
                error_at(context->source_file, start,
                         "invalid radix in number literal");
            }
            real_value = strtodb(base, NULL, (int)int_value);
            break;
    }

    Token_new(token, context, TOKEN_INTEGRAL, start, p);
    switch (state) {
        case RADIX:
        case INTEGER:
            token->int_value = int_value;
            break;
        case FRACTION:
        case FRACTION_B:
        case EXPONENT:
            token->real_value = real_value;
            token->kind = TOKEN_REAL;
            break;
        default:
            error("internal compiler error: unexpected state in number literal %d",
                  state);
    }
    *new_position = p;
}

/// Read an escape sequence.
/// \param context The lexer context.
/// \param position The starting position in the source file.
/// \param new_position The position to update to after the escape sequence.
/// \return The escape sequence.
/// \remark This function implements escape sequences in terms of their C
/// counterparts.
static char read_escaped_character(LexerContext *context, const char *position,
                                   const char **new_position) {
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
        case 'u': error_at(context->source_file, position,
                           "Unicode escape sequences are not yet supported");
        default: error_at(context->source_file, position,
                          "Invalid escape sequence");
    }
}

/// Search for a closing double quote " for a string literal.
/// \param context The lexer context.
/// \param position The starting position in the source file.
/// \return The position of the closing double quote.
/// \remark This function will exit the program if the string literal is
/// invalid.
static const char *string_literal_end(LexerContext *context,
                                      const char *position) {
    const char *p = position;
    while (*p != '"') {
        if (*p == '\0' || *p == '\n') {
            error_at(context->source_file, position,
                     "unterminated string literal");
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
/// \param token The token to initialise.
/// \param start The starting position in the source file.
/// \param new_position The position to update to after the string literal.
static void read_string_literal(LexerContext *context, Token *token,
                                const char *start, const char **new_position) {
    const char *end = string_literal_end(context, start + 1);
    char *buffer = calloc(1, end - start + 1);
    uptr len = 0;

    for (const char *p = start + 1; p < end;) {
        if (*p == '\\') {
            buffer[len++] = read_escaped_character(context, p + 1, &p);
        } else {
            buffer[len++] = *p++;
        }
    }

    Token_new(token, context, TOKEN_STRING, start, end);
    token->string_value = buffer;
    token->length = len;

    *new_position = end + 1;
}

/// Read a character literal into a token.
/// In Limbo, character literals are always represented as type `int`.
/// \param context The lexer context.
/// \param token The token to initialise.
/// \param start The starting position in the source file.
/// \param new_position The position to update to after the character literal.
static void read_char_literal(LexerContext *context, Token *token,
                              const char *start, const char **new_position) {
    const char *position = start + 1;
    if (*position == '\0') {
        error_at(context->source_file, position,
                 "unterminated character literal");
    }

    u32 c;
    if (*position == '\\') {
        c = (unsigned char) read_escaped_character(context,
                                                   position + 1, &position);
    } else {
        c = utf8_decode(position, &position);
    }

    const char *end = strchr(position, '\'');
    if (!end) {
        error_at(context->source_file, position,
                 "unterminated character literal");
    }

    Token_new(token, context, TOKEN_INTEGRAL, start, end);
    token->int_value = c;

    *new_position = end + 1;
}

/// Read the length of text that matches a keyword exactly.
/// \param context The lexer context.
/// \param start The starting position in the source file.
/// \param new_position The position to update to after the keyword.
/// \param keywords The keywords to match.
/// \param keyword_count The length of the keywords array.
/// \return The length of the keyword matched, or 0 if no keyword was matched.
static uptr read_keyword(const LexerContext *context, const char *start,
                         const char **new_position, char *keywords[],
                         uptr keyword_count) {
    for (uptr i = 0; i < keyword_count; i++) {
        uptr len = strlen(keywords[i]);
        if (strncmp(start, keywords[i], len) == 0) {
            *new_position = start + len;
            return len;
        }
    }
    return 0;
}

/// Read the length of text that is a valid identifier.
/// \param context The lexer context.
/// \param start The starting position in the source file.
/// \param new_position The position to update to after the identifier.
/// \return The length of the identifier matched, or 0 if no identifier was
/// matched.
static uptr read_identifier(const LexerContext *context, const char *start,
                            const char **new_position) {
    const char *p = start, *p_next;
    uptr len = 0;
    u32 c;

    // Read the first character.
    if (!(c = utf8_decode(p, &p_next))) {
        error_at(context->source_file, start,
                 "invalid character in identifier");
    }

    // Check if the first character is a valid start of an identifier.
    if (!is_identifier_start(c)) {
        return 0;
    }

    len += p_next - p;
    p = p_next;

    // Read the rest of the identifier.
    while (true) {
        if (!(c = utf8_decode(p, &p_next))) {
            error_at(context->source_file, start,
                     "invalid character in identifier");
        }

        if (!is_identifier_rest(c)) {
            break;
        }

        len += p_next - p;
        p = p_next;
    }

    *new_position = p;
    assert(len == p - start);
    return len;
}

LexerContext LexerContext_from(const SourceFile *file) {
    LexerContext context = {
            .source_file = file,
            .position = file->contents,
            .at_beginning_of_line = true,
            .follows_space = false,
            .line_number = 1,
            .column_number = 1,
    };
    return context;
}

void lex_one(LexerContext *context, Token *token) {
    while (*context->position) {
        // Skip comments
        if (*context->position == '#') {
            context->position++;
            context->column_number++;
            // Advance to the end of the line
            while (*context->position && *context->position != '\n') {
                context->position++;
                context->column_number++;
            }
            context->follows_space = true;
            continue;
        }

        // Skip newlines
        if (*context->position == '\n') {
            context->position++;
            context->column_number = 1;
            context->line_number++;
            context->at_beginning_of_line = true;
            context->follows_space = false;
            continue;
        }

        // Skip whitespace
        if (isspace(*context->position)) {
            context->position++;
            context->column_number++;
            context->follows_space = true;
            continue;
        }

        // Tokens start here

        // Number literal
        if (isdigit(*context->position)
            || *context->position == '.' && isdigit(context->position[1])) {
            read_number_literal(context, token,
                                context->position, &context->position);
            context->column_number += token->length;
            return;
        }

        // String literal
        if (*context->position == '"') {
            read_string_literal(context, token,
                                context->position, &context->position);
            context->column_number += token->length;
            return;
        }

        // Character literal
        if (*context->position == '\'') {
            read_char_literal(context, token,
                              context->position, &context->position);
            context->column_number += token->length;
            return;
        }

        // Keyword
        uptr keyword_len = read_keyword(context,
                                        context->position,&context->position,
                                        KEYWORDS, KEYWORD_COUNT);
        if (keyword_len) {
            Token_new(token, context, TOKEN_KEYWORD,
                      context->position - keyword_len, context->position);
            context->column_number += token->length;
            return;
        }

        // Identifier
        uptr ident_len = read_identifier(context,
                                         context->position, &context->position);
        if (ident_len) {
            Token_new(token, context, TOKEN_IDENTIFIER,
                      context->position - ident_len, context->position);
            context->column_number += token->length;
            return;
        }

        // Punctuator
        uptr punct_len = read_keyword(context,
                                      context->position, &context->position,
                                      PUNCT, PUNCT_COUNT);
        if (punct_len) {
            Token_new(token, context, TOKEN_PUNCTUATOR,
                      context->position - punct_len, context->position);
            context->column_number += token->length;
            return;
        }

        // Invalid character
        error_at(context->source_file, context->position, "invalid character");
    }

    // End of file
    Token_new(token, context, TOKEN_EOF, context->position, context->position);
}

Token *lex(SourceFile *file) {
    Token head = {}, *current = &head;

    LexerContext context = LexerContext_from(file);
    while (*context.position) {
        Token *token = calloc(1, sizeof(Token));
        lex_one(&context, token);
        current = current->next = token;
    }

    return head.next;
}

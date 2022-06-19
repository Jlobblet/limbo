#ifndef LIMBO_LEXER_H
#define LIMBO_LEXER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include "num.h"

// Structs

/// An enum representing the different kinds of tokens.
typedef enum TokenKind {
    /// A token representing an identifier.
    TOKEN_IDENTIFIER,
    /// A token representing a punctuator.
    TOKEN_PUNCTUATOR,
    /// A token representing a reserved keyword.
    TOKEN_KEYWORD,
    /// A token representing a string literal.
    TOKEN_STRING,
    /// A token representing a number literal of any type.
    TOKEN_NUMBER,
    /// A token representing the end of a file.
    TOKEN_EOF,
} TokenKind;

/// A struct containing metadata about a source file.
typedef struct SourceFile {
    /// The path to the source file.
    char *name;
    /// The number of the file in order of lexing.
    uptr file_number;
    /// The contents of the source file.
    char *contents;
} SourceFile;

typedef struct Token Token;
/// A struct containing metadata about a token.
struct Token {
    // Token information

    /// The kind of the token.
    /// \see TokenKind
    TokenKind kind;
    /// The next token in the token stream, if any.
    /// \remark If this is `NULL`, then this is the last token in the token
    /// stream.
    /// In that case, the `TokenKind` of this token is `TOKEN_EOF`.
    Token *next;
    /// Pointer to the start of the token in the source file.
    char *location;
    /// The length of the token in the source file.
    uptr length;

    // Values, if applicable.

    /// The value of the token if it is a string literal.
    /// \remark If `TokenKind` is not `TOKEN_STRING`, then this is `NULL`.
    char *string_value;
    /// The value of the token if it is an integral number literal.
    /// \remark If `TokenKind` is not `TOKEN_NUMBER`, then this is zero.
    /// \remark If `TokenKind` is `TOKEN_NUMBER`, but the number is a real
    /// number, then this is zero.
    /// \see token_kind
    /// \see real_value
    i64 int_value;
    /// The value of the token if it is a decimal number literal.
    /// \remark If `TokenKind` is not `TOKEN_NUMBER`, zero.
    /// \remark If `TokenKind` is `TOKEN_NUMBER`, but the number is an integral
    /// number, then this is zero.
    /// \see token_kind
    /// \see int_value
    f64 real_value;

    // Source file information
    /// The source file that this token came from.
    /// \see SourceFile
    SourceFile *source_file;
    /// The file name of the source file that this token came from.
    char *source_file_name;
    /// The line number of the token in the source file.
    uptr source_file_line;
    /// The column number of the token in the source file.
    uptr source_file_column;
    /// Whether the token is at the start of a line.
    bool at_beginning_of_line;
    /// Whether the token follows a whitespace character.
    bool follows_space;
};

typedef struct LexerContext {
    /// The source file that is being lexed.
    SourceFile *source_file;
    /// Whether the current position is at the start of a line.
    bool at_beginning_of_line;
    /// Whether the current position follows a whitespace character.
    bool follows_space;
} LexerContext;

Token *read_number_literal(LexerContext *context, char *start, char **new_position);

// Token manipulation

// File manipulation

bool lex(SourceFile *file, Token **tokens);

/// Convert a source file into an array of `Token`s.
/// \param filepath The path to the source file.
/// \param tokens A pointer to the array of `Token`s.
/// \see Token
bool Lex_file(char *filepath, Token **tokens);

#endif //LIMBO_LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "unicode.h"

noreturn void error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

static void formatted_error(const char *filename, const char *input,
                            uptr line_number, const char *location,
                            const char *fmt, va_list args) {
    /// Find the line that contains `location`.
    const char *line_start = location;
    while (input < line_start && line_start[-1] != '\n') {
        line_start--;
    }

    // Find the end of the line that contains `location`.
    const char *line_end = location;
    while (line_end[0] != '\n' && line_end[0] != '\0') {
        line_end++;
    }

    // Print out the line.
    uptr indentation = fprintf(stderr, "%s:%lu: ", filename, line_number);
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);

    // Print out the error.
    uptr position = display_width(line_start, location - line_start) + indentation;

    // Spaces
    fprintf(stderr, "%*s", (int)position, "");
    // Caret
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

noreturn void error_at(const SourceFile *file, const char *location, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    formatted_error(file->name, file->contents, file->file_number, location, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

noreturn void error_token(const Token *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    formatted_error(token->source_file->name, token->source_file->contents,
                    token->source_file->file_number, token->location, fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void warn_token(const Token *token, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    formatted_error(token->source_file->name, token->source_file->contents,
                    token->source_file->file_number, token->location, fmt, args);
    va_end(args);
}

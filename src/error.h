#ifndef LIMBO_ERROR_H
#define LIMBO_ERROR_H

#include <stdnoreturn.h>
#include <stdarg.h>
#include "lexer.h"

/// Report an error and then exit the program.
/// \param fmt The format string for the error message.
/// \param ... The arguments for the format string.
/// \remark This function will not return.
/// \remark This functions exits the program with status `EXIT_FAILURE`.
noreturn void error(const char *fmt, ...);

/// Report an error message at the given location, then exit the program.
/// \param file The source file containing the error.
/// \param location The location of the error in the source file.
/// \param fmt The format string for the error message.
/// \param ... The arguments for the format string.
/// \remark This function will not return.
/// \remark This functions exits the program with status `EXIT_FAILURE`.
noreturn void error_at(const SourceFile *file, const char *location,
                       const char *fmt, ...);

/// Report an error message caused by the given token, then exit the program.
/// \param token The token that caused the error.
/// \param fmt The format string for the error message.
/// \param ... The arguments for the format string.
/// \remark This function will not return.
/// \remark This functions exits the program with status `EXIT_FAILURE`.
noreturn void error_token(const Token *token, const char *fmt, ...);

/// Report a warning message caused by the given token.
/// \param token The token that caused the warning.
/// \param fmt The format string for the warning message.
/// \param ... The arguments for the format string.
void warn_token(const Token *token, const char *fmt, ...);

#endif //LIMBO_ERROR_H

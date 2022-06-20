#ifndef LIMBO_UNICODE_H
#define LIMBO_UNICODE_H

#include <stdbool.h>
#include "num.h"

typedef enum UTF8Length {
    UTF8_INVALID = 0,
    UTF8_1 = 1,
    UTF8_2 = 2,
    UTF8_3 = 3,
    UTF8_4 = 4,
} UTF8Length;

/// Encode a codepoint as a UTF-8 string.
/// \param codepoint The codepoint to encode.
/// \param buffer The buffer to write the UTF-8 string to.
/// \return The number of bytes written to `buffer`.
/// \remark If `buffer` is `NULL`, then  `errno` is set to `EINVAL`.
UTF8Length utf8_encode(u32 codepoint, char *buffer);

/// Read a UTF-8 encoded codepoint from a string, and advance the pointer. If
/// the string does not contain a valid UTF-8 codepoint, then the pointer is
/// not advanced, and the function returns `0` and `errno` is set to indicate
/// the error.
/// \param p The pointer to read the UTF-8 string from.
/// \param new_position The location of the next codepoint in the string.
/// \return The codepoint read.
/// \remark If the codepoint is invalid, then `errno` is set to `EILSEQ`.
/// \remark If `p` or `new_position` is `NULL`, then `errno` is set to `EINVAL`.
u32 utf8_decode(char *p, char **new_position);

/// The number of columns required to display a given codepoint in a monospace
/// font.
/// \param codepoint The codepoint to measure.
/// \return The number of columns required to display the codepoint.
UTF8Length codepoint_width(u32 codepoint);

/// Return the number of columns needed to display a given string in a
/// monospace format.
/// \param str The string to measure.
/// \param len The length of the string.
/// \return The number of columns needed to display the string.
/// \remark If `str` is `NULL`, then `errno` is set to `EINVAL`.
uptr display_width(char *str, uptr len);

/// Whether a given codepoint is allowed as the first character of an
/// identifier.
/// \param codepoint The codepoint to check.
/// \return Whether the codepoint is allowed.
/// \see is_identifier_rest
bool is_identifier_start(u32 codepoint);

/// Whether a given codepoint is allowed as a subsequent character of
/// an identifier.
/// \param codepoint The codepoint to check.
/// \return Whether the codepoint is allowed.
/// \see is_identifier_start
bool is_identifier_rest(u32 codepoint);

/// Whether a given string is a valid identifier.
/// \param str The string to check.
/// \param len The length of the string.
/// \return Whether the string is a valid identifier.
/// \remark If `str` is `NULL`, then `errno` is set to `EINVAL`.
bool is_identifier(char *str, uptr len);

#endif //LIMBO_UNICODE_H

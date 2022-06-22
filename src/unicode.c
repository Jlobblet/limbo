#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "unicode.h"
#include "num.h"

/// Struct that stores the start end end of a range of `u32` values.
/// Used to represent a range of Unicode codepoints.
typedef struct Interval {
    /// The start of the range.
    u32 start;
    /// The end of the range.
    u32 end;
} Interval;

/// Helper function to test whether an array of intervals is sorted in
/// ascending order.
/// \param intervals The array of intervals to test.
/// \param length The number of intervals in the array.
/// \return Whether the array is sorted.
static bool is_sorted_range(const Interval intervals[], u32 num_ranges) {
    Interval last = {0, 0};
    for (uptr i = 0; i < num_ranges; i++) {
        // The start of an interval must not be greater than its end.
        if (intervals[i].start > intervals[i].end) {
            return false;
        }
        // The start of an interval must not be less than the end of the
        // previous - overlapping ranges are not allowed.
        if (intervals[i].start < last.end) {
            return false;
        }
        last = intervals[i];
    }
    return true;
}

/// Check whether a given codepoint is within an array of permitted intervals.
/// \param codepoint The codepoint to check.
/// \param intervals The array of permitted intervals.
/// \param len The number of intervals in the array.
/// \remark Each intervals within the array is inclusive of the start and end.
/// \remark The array does not need to be in sorted order.
/// \return Whether the codepoint is permitted.
bool is_in_range(u32 codepoint, const Interval intervals[], uptr len) {
    // Just iterate over the intervals and check whether the codepoint is
    // within any of them.
    for (uptr i = 0; i < len; i++) {
        if (intervals->start <= codepoint && codepoint <= intervals->end) {
            return true;
        }
    }
    return false;
}

/// Check whether a given codepoint is within a sorted array of permitted
/// ranges.
/// \param codepoint The codepoint to check.
/// \param intervals The array of permitted ranges.
/// \remark Each intervals within the array is inclusive of the start and end.
/// \remark The array must be in sorted order.
/// \return Whether the codepoint is permitted.
bool is_in_sorted_range(u32 codepoint, const Interval intervals[], uptr len) {
    uptr low = 0;
    uptr high = len - 1;

    // If the codepoint is less than the start of the first interval, or greater
    // than the end of the last interval, it is not in the range.
    if (codepoint < intervals[low].start || codepoint > intervals[high].end) {
        return false;
    }

    // Perform a binary search
    while (low <= high) {
        uptr mid = (low + high) / 2;
        if (codepoint > intervals[mid].end) {
            low = mid + 1;
        } else if (codepoint < intervals[mid].start) {
            high = mid - 1;
        } else {
            return true;
        }
    }
    return false;
}

/// The first byte of a two-byte codepoint is 110xxxxx
static const unsigned char TWO_BYTE_MASK   = 0b11000000;
/// The first byte of a three-byte codepoint is 1110xxxx
static const unsigned char THREE_BYTE_MASK = 0b11100000;
/// The first byte of a four-byte codepoint is 11110xxx
static const unsigned char FOUR_BYTE_MASK  = 0b11110000;
/// The later bytes of a codepoint are 10xxxxxx
static const unsigned char REST_MASK       = 0b10000000;
/// The later bytes of a codepoint are 10xxxxxx
static const unsigned char DATA_MASK       = 0b00111111;

UTF8Length utf8_encode(u32 codepoint, char *buffer) {
    if (buffer == NULL) {
        errno = EINVAL;
        return UTF8_INVALID;
    }

    // If the codepoint is valid ASCII, i.e. a one-byte codepoint
    if (codepoint <= 0x7f) {
        buffer[0] = (char)codepoint;
        return UTF8_1;
    }

    // Two-byte codepoints
    // 0x80 - 0x7ff
    if (codepoint <= 0x7ff) {
        buffer[0] = (char)(TWO_BYTE_MASK | (codepoint >> 6));
        buffer[1] = (char)(REST_MASK | (codepoint & DATA_MASK));
        return UTF8_2;
    }

    // Three-byte codepoints
    // 0x800 - 0xffff
    if (codepoint <= 0xffff) {
        buffer[0] = (char)(THREE_BYTE_MASK | (codepoint >> 12));
        buffer[1] = (char)(REST_MASK | ((codepoint >> 6) & DATA_MASK));
        buffer[2] = (char)(REST_MASK | (codepoint & DATA_MASK));
        return UTF8_3;
    }

    // Four-byte codepoints
    // 0x10000 - 0x10ffff
    buffer[0] = (char)(FOUR_BYTE_MASK | (codepoint >> 18));
    buffer[1] = (char)(REST_MASK | ((codepoint >> 12) & DATA_MASK));
    buffer[2] = (char)(REST_MASK | ((codepoint >> 6) & DATA_MASK));
    buffer[3] = (char)(REST_MASK | (codepoint & DATA_MASK));
    return UTF8_4;
}

u32 utf8_decode(const char *p, const char **new_position) {
    if (p == NULL || new_position == NULL) {
        errno = EINVAL;
        return 0;
    }

    // If the codepoint is valid ASCII, i.e. a one-byte codepoint
    if ((unsigned char)*p <= 0x7f) {
        *new_position = p + 1;
        return *p;
    }

    int len;
    u32 codepoint;

    if ((unsigned char)*p >= FOUR_BYTE_MASK) {
        len = 4;
        codepoint = *p & 0b111;
    } else if ((unsigned char)*p >= THREE_BYTE_MASK) {
        len = 3;
        codepoint = *p & 0b1111;
    } else if ((unsigned char)*p >= TWO_BYTE_MASK) {
        len = 2;
        codepoint = *p & 0b11111;
    } else {
        errno = EILSEQ;
        return 0;
    }

    for (int i = 1; i < len; i++) {
        // If the codepoint is invalid, i.e. the next byte is not 10xxxxxx
        if ((unsigned char)p[i] >> 6 != 0b10) {
            errno = EILSEQ;
            return 0;
        }
        codepoint = (codepoint << 6) | (p[i] & DATA_MASK);
    }

    *new_position = p + len;
    return codepoint;
}

uptr display_width(const char *str, uptr len) {
    if (str == NULL) {
        errno = EINVAL;
        return 0;
    }

    if (len == 0) {
        return 0;
    }

    const char *start = str;
    uptr width = 0;

    while (str - start < len) {
        u32 codepoint = utf8_decode(str, &str);
        width += codepoint_width(codepoint);
    }

    return width;
}

bool is_identifier_start(u32 codepoint) {
    // TODO find out all allowed codepoints for the start of an identifier
    static Interval allowed_ranges[] = {
            {'A', 'Z'},
            {'_', '_'},
            {'a', 'z'},
    };
    int num_ranges = sizeof(allowed_ranges) / sizeof(*allowed_ranges);
    assert(is_sorted_range(allowed_ranges, num_ranges));
    return is_in_sorted_range(codepoint, allowed_ranges, num_ranges);
}

bool is_identifier_rest(u32 codepoint) {
    // TODO find out all allowed codepoints for the rest of an identifier
    static Interval allowed_ranges[] = {
            {'0', '9'},
            {'A', 'Z'},
            {'_', '_'},
            {'a', 'z'},
    };
    int num_ranges = sizeof(allowed_ranges) / sizeof(*allowed_ranges);
    assert(is_sorted_range(allowed_ranges, num_ranges));
    return is_in_sorted_range(codepoint, allowed_ranges, num_ranges);
}

bool is_identifier(const char *str, uptr len) {
    if (str == NULL) {
        errno = EINVAL;
        return false;
    }

    if (len == 0) {
        return false;
    }
    const char *end = str + len;
    u32 current = utf8_decode(str, &str);
    if (!is_identifier_start(current)) {
        return false;
    }
    while (str < end) {
        current = utf8_decode(str, &str);
        if (!is_identifier_rest(current)) {
            return false;
        }
    }
    return true;
}

UTF8Length codepoint_width(u32 codepoint) {
    // Implementation based on https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c.

    if (codepoint == 0) {
        return 0;
    }
    if (codepoint < 0x20 || (codepoint >= 0x7f && codepoint < 0xa0)) {
        return -1;
    }

    // These codepoints are zero width
    static Interval ranges0[] = {
            {0x0300,  0x036F},
            {0x0483,  0x0486},
            {0x0488,  0x0489},
            {0x0591,  0x05BD},
            {0x05BF,  0x05BF},
            {0x05C1,  0x05C2},
            {0x05C4,  0x05C5},
            {0x05C7,  0x05C7},
            {0x0600,  0x0603},
            {0x0610,  0x0615},
            {0x064B,  0x065E},
            {0x0670,  0x0670},
            {0x06D6,  0x06E4},
            {0x06E7,  0x06E8},
            {0x06EA,  0x06ED},
            {0x070F,  0x070F},
            {0x0711,  0x0711},
            {0x0730,  0x074A},
            {0x07A6,  0x07B0},
            {0x07EB,  0x07F3},
            {0x0901,  0x0902},
            {0x093C,  0x093C},
            {0x0941,  0x0948},
            {0x094D,  0x094D},
            {0x0951,  0x0954},
            {0x0962,  0x0963},
            {0x0981,  0x0981},
            {0x09BC,  0x09BC},
            {0x09C1,  0x09C4},
            {0x09CD,  0x09CD},
            {0x09E2,  0x09E3},
            {0x0A01,  0x0A02},
            {0x0A3C,  0x0A3C},
            {0x0A41,  0x0A42},
            {0x0A47,  0x0A48},
            {0x0A4B,  0x0A4D},
            {0x0A70,  0x0A71},
            {0x0A81,  0x0A82},
            {0x0ABC,  0x0ABC},
            {0x0AC1,  0x0AC5},
            {0x0AC7,  0x0AC8},
            {0x0ACD,  0x0ACD},
            {0x0AE2,  0x0AE3},
            {0x0B01,  0x0B01},
            {0x0B3C,  0x0B3C},
            {0x0B3F,  0x0B3F},
            {0x0B41,  0x0B43},
            {0x0B4D,  0x0B4D},
            {0x0B56,  0x0B56},
            {0x0B82,  0x0B82},
            {0x0BC0,  0x0BC0},
            {0x0BCD,  0x0BCD},
            {0x0C3E,  0x0C40},
            {0x0C46,  0x0C48},
            {0x0C4A,  0x0C4D},
            {0x0C55,  0x0C56},
            {0x0CBC,  0x0CBC},
            {0x0CBF,  0x0CBF},
            {0x0CC6,  0x0CC6},
            {0x0CCC,  0x0CCD},
            {0x0CE2,  0x0CE3},
            {0x0D41,  0x0D43},
            {0x0D4D,  0x0D4D},
            {0x0DCA,  0x0DCA},
            {0x0DD2,  0x0DD4},
            {0x0DD6,  0x0DD6},
            {0x0E31,  0x0E31},
            {0x0E34,  0x0E3A},
            {0x0E47,  0x0E4E},
            {0x0EB1,  0x0EB1},
            {0x0EB4,  0x0EB9},
            {0x0EBB,  0x0EBC},
            {0x0EC8,  0x0ECD},
            {0x0F18,  0x0F19},
            {0x0F35,  0x0F35},
            {0x0F37,  0x0F37},
            {0x0F39,  0x0F39},
            {0x0F71,  0x0F7E},
            {0x0F80,  0x0F84},
            {0x0F86,  0x0F87},
            {0x0F90,  0x0F97},
            {0x0F99,  0x0FBC},
            {0x0FC6,  0x0FC6},
            {0x102D,  0x1030},
            {0x1032,  0x1032},
            {0x1036,  0x1037},
            {0x1039,  0x1039},
            {0x1058,  0x1059},
            {0x1160,  0x11FF},
            {0x135F,  0x135F},
            {0x1712,  0x1714},
            {0x1732,  0x1734},
            {0x1752,  0x1753},
            {0x1772,  0x1773},
            {0x17B4,  0x17B5},
            {0x17B7,  0x17BD},
            {0x17C6,  0x17C6},
            {0x17C9,  0x17D3},
            {0x17DD,  0x17DD},
            {0x180B,  0x180D},
            {0x18A9,  0x18A9},
            {0x1920,  0x1922},
            {0x1927,  0x1928},
            {0x1932,  0x1932},
            {0x1939,  0x193B},
            {0x1A17,  0x1A18},
            {0x1B00,  0x1B03},
            {0x1B34,  0x1B34},
            {0x1B36,  0x1B3A},
            {0x1B3C,  0x1B3C},
            {0x1B42,  0x1B42},
            {0x1B6B,  0x1B73},
            {0x1DC0,  0x1DCA},
            {0x1DFE,  0x1DFF},
            {0x200B,  0x200F},
            {0x202A,  0x202E},
            {0x2060,  0x2063},
            {0x206A,  0x206F},
            {0x20D0,  0x20EF},
            {0x302A,  0x302F},
            {0x3099,  0x309A},
            {0xA806,  0xA806},
            {0xA80B,  0xA80B},
            {0xA825,  0xA826},
            {0xFB1E,  0xFB1E},
            {0xFE00,  0xFE0F},
            {0xFE20,  0xFE23},
            {0xFEFF,  0xFEFF},
            {0xFFF9,  0xFFFB},
            {0x10A01, 0x10A03},
            {0x10A05, 0x10A06},
            {0x10A0C, 0x10A0F},
            {0x10A38, 0x10A3A},
            {0x10A3F, 0x10A3F},
            {0x1D167, 0x1D169},
            {0x1D173, 0x1D182},
            {0x1D185, 0x1D18B},
            {0x1D1AA, 0x1D1AD},
            {0x1D242, 0x1D244},
            {0xE0001, 0xE0001},
            {0xE0020, 0xE007F},
            {0xE0100, 0xE01EF}
    };
    static int num_ranges0 = sizeof(ranges0) / sizeof(*ranges0);
    assert(is_sorted_range(ranges0, num_ranges0));

    if (is_in_sorted_range(codepoint, ranges0, num_ranges0)) {
        return 0;
    }

    static Interval ranges2[] = {
            {0x1100, 0x115F}, {0x2329, 0x2329}, {0x232A, 0x232A}, {0x2E80, 0x303E},
            {0x3040, 0xA4CF}, {0xAC00, 0xD7A3}, {0xF900, 0xFAFF}, {0xFE10, 0xFE19},
            {0xFE30, 0xFE6F}, {0xFF00, 0xFF60}, {0xFFE0, 0xFFE6}, {0x1F000, 0x1F644},
            {0x20000, 0x2FFFD}, {0x30000, 0x3FFFD},
    };
    static int num_ranges2 = sizeof(ranges2) / sizeof(*ranges2);
    assert(is_sorted_range(ranges2, num_ranges2));

    if (is_in_sorted_range(codepoint, ranges2, num_ranges2)) {
        return 2;
    }

    return 1;
}

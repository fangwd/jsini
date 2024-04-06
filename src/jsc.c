/*
 * Copyright (c) Weidong Fang
 */

#include "jsini.h"

/**
 * Escapes a unicode character in JSON format and writes the sequence to buffer.
 * The buffer should be large enough to hold 6-12 bytes depending on the code
 * point of the character. Returns the number of bytes written to the buffer.
 *
 * Inline comments are taken from http://www.ietf.org/rfc/rfc4627.txt and
 * http://en.wikipedia.org/wiki/UTF-16.
 */
int32_t json_escape_unicode(int32_t ch, char *buffer) {
    /* If the character is in the Basic Multilingual Plane (U+0000 through
     * U+FFFF), then it may be represented as a six-character sequence.
     *
     * To escape an extended character that is not in the Basic Multilingual
     * Plane, the character is represented as a twelve-character sequence,
     * encoding the UTF-16 surrogate pair.
     */
    int32_t lead, trail;

    if(ch <= 0xffff) {
        if (ch >= 0xd800 && ch <= 0xdfff) {
            /* Reserved for surrogate pairs and should not be encoded */
            return -1;
        }
        sprintf(buffer, "\\u%04x", ch);
        return 6;
    }


    /* 0x010000 is subtracted from the code point, leaving a 20 bit number in
     * the range 0..0x0FFFFF.
     */
    ch -= 0x10000;

    /* the top ten bits (a number in the range 0..0x03ff) are added to 0xd800 to
     * give the first code unit or lead surrogate, which will be in the range
     * 0xd800..0xdbff.
     */
    lead = 0xd800 | ((ch & 0xffc00) >> 10);

    /* The low ten bits (also in the range 0..0x03ff) are added to 0xdc00 to
     * give the second code unit or trail surrogate, which will be in the
     * range 0xdc00..0xdfff.
     */
    trail = 0xdc00 | (ch & 0x003ff);

    sprintf(buffer, "\\u%04x\\u%04x", lead, trail);

    return 12;
}

static int32_t decode_hex_string(const char *s, int32_t *ch) {
    int32_t i, n;

    if (s[0] != '\\' || s[1] != 'u') {
        return -1;
    }

    for (i = 2, n = 0; i < 6; i++) {
        char c = s[i];
        n *= 16;
        if (c >= '0' && c <= '9')
            n += c - '0';
        else if (c >= 'a' && c <= 'f')
            n += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            n += c - 'A' + 10;
        else
            return -1;
    }

    *ch = n;

    return 6;
}

int32_t json_unescape_unicode(const char *start, const char *end, int32_t *ch) {
    if (start + 6 > end) {
        return -1;
    }

    if (decode_hex_string(start, ch) != 6) {
        return -1;
    }

    if (*ch >= 0xd800 && *ch <= 0xdbff) {
        int32_t ch2;

        if (start + 12 > end) {
            return -1;
        }

        if (decode_hex_string(start + 6, &ch2) != 6) {
            return -1;
        }

        /* http://en.wikipedia.org/wiki/Mapping_of_Unicode_characters#Surrogates
         * 0x10000 + (H − 0xD800) * 0x400 + (L − 0xDC00)
         */
        *ch = 0x10000 + (*ch - 0xd800) * 0x400 + ch2 - 0xdc00;

        return 12;
    }

    return 6;
}

/* Encodes utf-8 character to bytes. returns the number of resulted bytes or -1
 * if the character is not in the range of u+0000 to u+10ffff.
 *
 * http://en.wikipedia.org/wiki/UTF-8
 */
int32_t encode_utf8(int32_t ch, char *buffer) {
    if (ch <= 0x7f) {
        buffer[0] = (char) ch;
        return 1;
    }

    if (ch <= 0x7ff) {
        buffer[0] = (char) (0xc0 | (ch >> 6));
        buffer[1] = (char) (0x80 | (ch & 0x03f));
        return 2;
    }

    if (ch <= 0xffff) {
        buffer[0] = (char) (0xe0 | (ch >> 12));
        buffer[1] = (char) (0x80 | ((ch >> 6) & 0x3f));
        buffer[2] = (char) (0x80 | (ch & 0x3f));
        return 3;
    }

    if (ch <= 0x10ffff) {
        buffer[0] = (char) (0xf0 | (ch >> 18));
        buffer[1] = (char) (0x80 | ((ch >> 12) & 0x3f));
        buffer[2] = (char) (0x80 | ((ch >> 6) & 0x3f));
        buffer[3] = (char) (0x80 | (ch & 0x3f));
        return 4;
    }

    return -1;
}

/**
 * Decodes a utf-8 character from a sequence of bytes. Returns the number of
 * bytes in the sequence that represents the character or -1 if the sequence
 * does not begin with a valid utf-8 character.
 *
 * The is the reverse process of encode_utf8().
 */

#define UTF8_MASK(c) (((c & 0xC0) == 0x80) ? (c & 0x3F) : -1)

int32_t decode_utf8(const char *s, int32_t *ch) {
    int32_t c0, c1, c2, c3;

    c0 = s[0];

    if ((c0 & 0x80) == 0) {
        *ch = c0;
        return 1;
    }

    if ((c1 = UTF8_MASK(s[1])) < 0) {
        return -1;
    }

    if ((c0 & 0xe0) == 0xc0) { /* 110xxxxx */
        int32_t c = ((c0 & 0x1f) << 6) | c1;
        if (c >= 0x80) {
            *ch = c;
            return 2;
        }
        return -1;
    }

    if ((c2 = UTF8_MASK(s[2])) < 0) {
        return -1;
    }

    if ((c0 & 0xf0) == 0xe0) { /* 1110xxxx */
        *ch = ((c0 & 0x0f) << 12) | (c1 << 6) | c2;
        if (*ch >= 0x0800 && (*ch <0xd800 || *ch > 0xdfff)) {
            return 3;
        }
        return -1;
    }

    if ((c3 = UTF8_MASK(s[3])) < 0) {
        return -1;
    }

    if ((c0 & 0xf8) == 0xf0) { /* 11110xxx */
        int32_t c = ((c0 & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
        if (c >= 0x10000) {
            *ch = c;
            return 4;
        }
    }

    return -1;
}

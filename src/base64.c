#include "../include/base64.h"

#include <stdlib.h>
#include <string.h>

/* The 64 characters used by base64 encoding, in order */
static const char BASE64_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int is_valid_base64_char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '+' || c == '/' || c == '=';
}

int base64_looks_encoded(const unsigned char *src, size_t len) {
    if (len < 4) return 0;

    size_t sample_size = len < 64 ? len : 64;
    int valid_count = 0;

    for (size_t i = 0; i < sample_size; i++)
        if (is_valid_base64_char(src[i]))
            valid_count++;

    /* At least 90% of sampled bytes must be valid base64 chars (see header notes) */
    return valid_count >= (int)(sample_size * 9 / 10);
}

int base64_decode(const unsigned char *src, size_t src_len,
                  unsigned char **out, size_t *out_len) {
    /* Build a reverse lookup table: character -> 6-bit value
     * In other words: Map each base64 character to its numeric value (0-63)
     * Example: 'A' -> 0, 'B' -> 1, ... 'Z' -> 25, 'a' -> 26, ... '9' -> 61 *
     */
    int lookup[256];
    memset(lookup, -1, sizeof(lookup));
    for (int i = 0; i < 64; i++)
        lookup[(unsigned char)BASE64_ALPHABET[i]] = i;
    lookup['='] = 0; /* padding character counts as zero */

    /* base64 was designed to transmit binary data as plain text.
     * To do that, it takes 3 bytes (24 bits) and splits them into
     * 4 groups of 6 bits, each mapped to a printable character.
     */
    size_t capacity = (src_len / 4) * 3 + 4;
    unsigned char *buf = malloc(capacity);
    if (!buf) return -1;

    size_t write_pos = 0;

    for (size_t i = 0; i < src_len; ) {
        /* Skip characters that are not part of base64 (e.g. newlines) */
        while (i < src_len && lookup[src[i]] == -1 && src[i] != '=')
            i++;

        if (i + 3 >= src_len) break;

        /* Read one group of 4 base64 characters */
        int a = lookup[src[i]];
        int b = lookup[src[i + 1]];
        int c = lookup[src[i + 2]];
        int d = lookup[src[i + 3]];
        i += 4;

        if (a < 0 || b < 0) break;

        /* Combine the four 6-bit values into three 8-bit bytes */
        buf[write_pos++] = (a << 2) | (b >> 4);
        if (src[i - 2] != '=' && c >= 0) buf[write_pos++] = (b << 4) | (c >> 2);
        if (src[i - 1] != '=' && d >= 0) buf[write_pos++] = (c << 6) | d;
    }

    *out     = buf;
    *out_len = write_pos;
    return 0;
}


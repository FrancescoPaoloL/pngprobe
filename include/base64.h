#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

/*
 * Returns 1 if the byte buffer looks like base64-encoded data, 0 otherwise.
 * Samples the first 64 bytes. We accept 90% valid chars (not 100%) because
 * real base64 strings can contain newlines and padding that are not in the
 * base64 alphabet but are still valid.
 */
int base64_looks_encoded(const unsigned char *src, size_t len);


/*
 * Decodes a base64 buffer into raw bytes.
 * On success, returns 0 and writes the result into *out and *out_len.
 * The caller must free() the returned buffer.
 * On failure, returns -1.
 */
int base64_decode(const unsigned char *src, size_t src_len,
                  unsigned char **out, size_t *out_len);

#endif


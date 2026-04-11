#include "../include/pngscanner.h"
#include "../include/base64.h"
#include "../include/zip_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>

/* Values shorter than this are printed as plain text, longer ones are probed */
#define SHORT_VALUE_MAX 64

/*
 * Every PNG file starts with these exact 8 bytes this is the PNG signature.
 * If a file does not start with this, it is not a PNG.
 * See: https://www.w3.org/TR/png/#5PNG-file-signature
 */
static const unsigned char PNG_SIGNATURE[8] =
    { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };

/*
 * PNG stores numbers in big-endian order (most significant byte first).
 * This function reads 4 bytes and assembles them into a single number.
 * See: https://www.w3.org/TR/png/#7Integers-and-byte-order
 */
static uint32_t read_u32_be(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |  (uint32_t)p[3];
}

/* Reads the entire file into memory. Caller must free() the buffer. */
static unsigned char *read_file(const char *path, long *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return NULL; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    unsigned char *buf = malloc(size);
    if (!buf) { fprintf(stderr, "malloc failed\n"); fclose(f); return NULL; }

    if (fread(buf, 1, size, f) != (size_t)size) {
        fprintf(stderr, "fread failed\n");
        free(buf);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *out_size = size;
    return buf;
}

/* Returns 1 if the first 8 bytes match the PNG signature */
static int is_valid_png(const unsigned char *data)
{
    return memcmp(data, PNG_SIGNATURE, 8) == 0;
}

/*
 * Verifies the CRC32 of a PNG chunk.
 *
 * The PNG spec requires a CRC32 checksum at the end of every chunk.
 * The checksum covers the chunk type (4 bytes) + the chunk data (length bytes).
 * It does NOT cover the length field itself.
 *
 * A mismatch means the chunk is corrupted or has been manually modified.
 * We treat it as a warning and continue scanning for a forensic tool,
 * a bad CRC is interesting information, not a reason to stop.
 *
 * See: https://www.w3.org/TR/png/#5Chunk-layout
 */
static void verify_crc(const char *type, const unsigned char *data,
                        uint32_t length, uint32_t stored_crc)
{
    /*
     * crc32() from zlib computes CRC32 incrementally.
     * We start with the type field, then feed in the data.
     * crc32(0, NULL, 0) returns the initial CRC value.
     */
    uLong crc = crc32(0, NULL, 0);
    crc = crc32(crc, (const Bytef *)type, 4);
    crc = crc32(crc, (const Bytef *)data, length);

    if ((uint32_t)crc != stored_crc)
        printf("  [!] CRC mismatch on chunk '%s': expected 0x%08x, got 0x%08x\n",
               type, (uint32_t)crc, stored_crc);
}

/*
 * Processes a single tEXt chunk.
 *
 * A tEXt chunk contains a key and a value separated by a null byte.
 * Short values are printed as text. Long values are checked for base64
 * if they decode to a ZIP, we pass them to zip_process().
 *
 * tEXt chunk format: https://www.w3.org/TR/png/#11tEXt
 */
static void handle_text_chunk(const unsigned char *data, uint32_t length,
                               const char *zip_save_path)
{
    /* Find the null byte that splits key and value */
    const unsigned char *null_pos = memchr(data, 0, length);
    if (!null_pos) return;

    const char          *key     = (const char *)data;
    const unsigned char *val     = null_pos + 1;
    size_t               val_len = length - (size_t)(null_pos - data) - 1;

    if (val_len <= SHORT_VALUE_MAX) {
        printf("tEXt  key=\"%s\"  value=\"%.*s\"\n",
               key, (int)val_len, val);
        return;
    }

    printf("tEXt  key=\"%s\"  value=<%zu bytes>\n", key, val_len);

    if (!base64_looks_encoded(val, val_len)) return;

    printf("  -> Looks like base64, decoding...\n");

    unsigned char *decoded = NULL;
    size_t dec_len = 0;
    if (base64_decode(val, val_len, &decoded, &dec_len) != 0) {
        printf("  -> base64 decode failed\n");
        return;
    }

    printf("  -> Decoded: %zu bytes\n", dec_len);

    /*
     * ZIP files always start with 'P' and 'K' the initials of Phil Katz,
     * who created the format. This is the ZIP magic number.
     * See: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT
     */
    int is_zip = dec_len >= 2 && decoded[0] == 'P' && decoded[1] == 'K';
    if (is_zip) {
        printf("  -> It's a ZIP file!\n");
        zip_process(decoded, dec_len, zip_save_path);
    } else {
        printf("  -> Not a ZIP (unknown format)\n");
    }

    free(decoded);
}

void png_scan(const char *path, const char *zip_save_path)
{
    long file_size = 0;
    unsigned char *data = read_file(path, &file_size);
    if (!data) return;

    if (!is_valid_png(data)) {
        fprintf(stderr, "Not a valid PNG file: %s\n", path);
        free(data);
        return;
    }

    printf("=== Scanning PNG chunks ===\n\n");

    /*
     * A PNG file is a sequence of chunks. Each chunk has:
     *   4 bytes - data length
     *   4 bytes - chunk type (e.g. "tEXt", "IDAT", "IEND")
     *   N bytes - data
     *   4 bytes - CRC checksum
     *
     * We skip the first 8 bytes (the signature) and walk chunk by chunk.
     * See: https://www.w3.org/TR/png/#5Chunk-layout
     */
    long pos = 8;
    while (pos + 12 <= file_size) {
        uint32_t length  = read_u32_be(data + pos);
        char     type[5] = {0};
        memcpy(type, data + pos + 4, 4);

        const unsigned char *chunk_data = data + pos + 8;
        uint32_t stored_crc = read_u32_be(data + pos + 8 + length);

        verify_crc(type, chunk_data, length, stored_crc);

        if (strcmp(type, "tEXt") == 0 && length > 0)
            handle_text_chunk(chunk_data, length, zip_save_path);

        pos += 12 + length;

        /* IEND marks the end of the PNG file - no more chunks after this */
        if (strcmp(type, "IEND") == 0) break;
    }

    free(data);
}


#include "../include/zip_reader.h"
#include "../include/tensor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>

#define TENSOR_MAX_PRINT 20

static void save_zip_to_disk(const unsigned char *blob, size_t blob_len,
                              const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("fopen (zip save)");
        return;
    }
    fwrite(blob, 1, blob_len, f);
    fclose(f);
    printf("  -> ZIP saved to: %s\n", path);
}

/* Returns the index of the first entry whose name ends with "data/0", or -1 */
static zip_int64_t find_tensor_entry(zip_t *za) {
    zip_int64_t count = zip_get_num_entries(za, 0);

    printf("\n=== ZIP contents (%lld entries) ===\n", (long long)count);

    zip_int64_t result = -1;

    for (zip_int64_t i = 0; i < count; i++) {
        const char *name = zip_get_name(za, i, 0);
        if (!name) continue;

        struct zip_stat st;
        zip_stat_init(&st);
        zip_stat_index(za, i, 0, &st);

        printf("  [%2lld] %s  (%llu bytes)\n",
               (long long)i, name, (unsigned long long)st.size);

        /* Check if the name ends with "data/0" */
        size_t name_len = strlen(name);
        if (name_len >= 6 && strcmp(name + name_len - 6, "data/0") == 0) {
            printf("        ^ tensor data file\n");
            result = i;
        }
    }

    return result;
}

static unsigned char *read_zip_entry(zip_t *za, zip_int64_t index,
                                      size_t *out_size) {
    struct zip_stat st;
    zip_stat_init(&st);
    zip_stat_index(za, index, 0, &st);

    size_t size = (size_t)st.size;
    unsigned char *buf = malloc(size);
    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        return NULL;
    }

    zip_file_t *zf = zip_fopen_index(za, index, 0);
    if (!zf) {
        fprintf(stderr, "Could not open ZIP entry: %s\n", zip_strerror(za));
        free(buf);
        return NULL;
    }

    zip_fread(zf, buf, size);
    zip_fclose(zf);

    *out_size = size;
    return buf;
}

void zip_process(const unsigned char *blob, size_t blob_len,
                 const char *save_path) {
    if (save_path)
        save_zip_to_disk(blob, blob_len, save_path);

    zip_error_t err;
    zip_source_t *src = zip_source_buffer_create(blob, blob_len, 0, &err);
    if (!src) {
        fprintf(stderr, "zip_source_buffer_create failed: %s\n",
                zip_error_strerror(&err));
        return;
    }

    zip_t *za = zip_open_from_source(src, ZIP_RDONLY, &err);
    if (!za) {
        fprintf(stderr, "zip_open_from_source failed: %s\n",
                zip_error_strerror(&err));
        zip_source_free(src);
        return;
    }

    zip_int64_t tensor_index = find_tensor_entry(za);
    if (tensor_index < 0) {
        printf("\n  -> No data/0 entry found in this ZIP.\n");
        zip_close(za);
        return;
    }

    size_t tensor_size = 0;
    unsigned char *tensor_bytes = read_zip_entry(za, tensor_index, &tensor_size);
    zip_close(za);

    if (tensor_bytes) {
        tensor_print_floats(tensor_bytes, tensor_size, TENSOR_MAX_PRINT);
        free(tensor_bytes);
    }
}


#ifndef ZIP_READER_H
#define ZIP_READER_H

#include <stddef.h>

/*
 * Opens a ZIP archive from a memory buffer, lists all files inside,
 * and looks for an entry whose name ends with "data/0".
 *
 * PyTorch saves tensors as ZIP archives since version 1.6.
 * The raw tensor bytes are always stored in "data/0" inside the ZIP.
 *
 * See: https://pytorch.org/docs/stable/generated/torch.save.html
 *
 * If found, reads its raw bytes and passes them to tensor_print_floats().
 * If save_path is not NULL, also writes the ZIP to that file on disk.
 */
void zip_process(const unsigned char *blob, size_t blob_len,
                 const char *save_path);

#endif


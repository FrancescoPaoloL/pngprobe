#ifndef TENSOR_H
#define TENSOR_H

#include <stddef.h>

/*
 * Reads a buffer of raw bytes as 32-bit floats (little-endian)
 * and prints the first 'max_print' values to stdout.
 */
void tensor_print_floats(const unsigned char *bytes, size_t byte_count,
                         size_t max_print);

#endif


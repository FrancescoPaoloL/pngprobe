#include "../include/tensor.h"
#include <stdio.h>
#include <string.h>

/*
 * Prints the float values stored inside a PyTorch tensor file (data/0).
 * PyTorch saves tensors as ZIP archives. Inside the ZIP, the raw bytes
 * of the tensor are always stored in a file called "data/0".
 * This is the format used by torch.save() since PyTorch 1.6.
 *
 * See: https://pytorch.org/docs/stable/generated/torch.save.html
 */
void tensor_print_floats(const unsigned char *bytes, size_t byte_count,
                         size_t max_print) {
    size_t total = byte_count / 4; /* each float32 is 4 bytes */

    printf("\n=== Tensor: %zu float32 values ===\n\n", total);

    size_t count = total < max_print ? total : max_print;

    for (size_t i = 0; i < count; i++) {
        float value;
        /* Use memcpy to safely read 4 bytes into a float (antirez docet!) */
        memcpy(&value, bytes + i * 4, sizeof(float));
        printf("  [%4zu]  %+.6f\n", i, value);
    }

    if (total > max_print)
        printf("  ... (%zu more values)\n", total - max_print);
}


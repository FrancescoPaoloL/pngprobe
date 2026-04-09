#include "../include/pngscanner.h"

#include <stdio.h>


/* Usage: ./pngprobe input.png [output.zip]
 *   output.zip is optional — if provided, saves the extracted ZIP to disk.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.png [output.zip]\n", argv[0]);
        return 1;
    }

    const char *input_png   = argv[1];
    const char *zip_out     = argc >= 3 ? argv[2] : NULL;

    png_scan(input_png, zip_out);

    return 0;
}


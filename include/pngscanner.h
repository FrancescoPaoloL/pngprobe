#ifndef PNGSCANNER_H
#define PNGSCANNER_H

/*
 * Reads the PNG file at 'path', walks all its chunks,
 * and prints every tEXt chunk it finds.
 *
 * If a tEXt value looks like base64 and decodes to a ZIP,
 * the ZIP is processed automatically (tensor values printed).
 *
 * If zip_save_path is not NULL, any discovered ZIP is also saved to that path.
 */
void png_scan(const char *path, const char *zip_save_path);

#endif


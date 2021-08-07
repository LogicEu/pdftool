#ifndef PDFTOOL_H
#define PDFTOOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*************
 ===PDFTOOL===
 ************/

#include <stdlib.h>

char* file_string_get(const char* path, long* file_size);
char* pdf_string_get(const char* path, long* size);
void pdf_to_txt_file(const char* in_pdf, const char* out_txt);
size_t sub_string_find(char* buffer, const char* search, size_t buffersize);

#ifdef __cplusplus
}
#endif
#endif


#ifndef HANDLEBARS_TESTS_UTILS_H
#define HANDLEBARS_TESTS_UTILS_H

#include <stdlib.h>

typedef void (*scan_directory_cb)(char * filename);

int file_get_contents(const char * filename, char ** buf, size_t * len);
int scan_directory_callback(char * dirname, scan_directory_cb cb);

#endif

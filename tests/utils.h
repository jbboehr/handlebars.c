
#ifndef HANDLEBARS_TESTS_UTILS_H
#define HANDLEBARS_TESTS_UTILS_H

#include <stdlib.h>

#define REGISTER_TEST(s, name, title) \
  TCase * tc_ ## name = tcase_create(title); \
  tcase_add_test(tc_ ## name, name); \
  suite_add_tcase(s, tc_ ## name);
  
#define REGISTER_TEST_FIXTURE(s, name, title) \
  TCase * tc_ ## name = tcase_create(title); \
  tcase_add_checked_fixture(tc_ ## name, setup, teardown); \
  tcase_add_test(tc_ ## name, name); \
  suite_add_tcase(s, tc_ ## name);
  
typedef void (*scan_directory_cb)(char * filename);

int file_get_contents(const char * filename, char ** buf, size_t * len);
int scan_directory_callback(char * dirname, scan_directory_cb cb);

#endif

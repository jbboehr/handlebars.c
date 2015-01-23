
#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"
#include "utils.h"

static void setup(void)
{
//   handlebars_alloc_failure(0);
    ;
}

static void teardown(void)
{
  ;
}

START_TEST(test_token_print)
{
}
END_TEST

START_TEST(test_token_list_print)
{
}
END_TEST

Suite * parser_suite(void)
{
  Suite * s = suite_create("Token Printer");
  
  REGISTER_TEST_FIXTURE(s, test_token_print, "Print Token");
  REGISTER_TEST_FIXTURE(s, test_token_list_print, "Print Token List");
  
  return s;
}

int main(void)
{
  int number_failed;
  Suite * s = parser_suite();
  SRunner * sr = srunner_create(s);
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
  srunner_set_fork_status(sr, CK_NOFORK);
#endif
  //srunner_set_log(sr, "test_token.log");
  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

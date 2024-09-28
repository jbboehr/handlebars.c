/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <check.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "handlebars_memory.h"
#include "handlebars_scanners.h"
#include "utils.h"



START_TEST(test_scanner_next_whitespace)
{
    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\r", 0));
    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\rfoo\n", 0));
    ck_assert_int_eq(1, handlebars_scanner_next_whitespace(" \t\r\n", 0));

    ck_assert_int_eq(0, handlebars_scanner_next_whitespace(" \t\rz", 1));
    ck_assert_int_eq(1, handlebars_scanner_next_whitespace(" \t\r", 1));
}
END_TEST

START_TEST(test_scanner_prev_whitespace)
{
    ck_assert_int_eq(1, handlebars_scanner_prev_whitespace("\n \t\r", 0));
    ck_assert_int_eq(0, handlebars_scanner_prev_whitespace("\n foo", 0));

    ck_assert_int_eq(0, handlebars_scanner_prev_whitespace("foo \t\r", 1));
    ck_assert_int_eq(1, handlebars_scanner_prev_whitespace(" \t\r", 1));

}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Scanners");

	REGISTER_TEST_FIXTURE(s, test_scanner_next_whitespace, "Prev Whitespace");
	REGISTER_TEST_FIXTURE(s, test_scanner_prev_whitespace, "Next Whitespace");


    return s;
}

int main(void)
{
    return default_main(&suite);
}

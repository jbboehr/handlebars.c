/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_map.h"
#include "handlebars_value.h"
#include "utils.h"


char mkchar(unsigned long i) {
    return (char) (32 + (i % (126 - 32)));
}


START_TEST(test_map)
{
#define STRSIZE 128
    size_t i, j;
    size_t pos = 0;
    size_t count = 10000;
    char tmp[STRSIZE];
    struct handlebars_map * map = handlebars_map_ctor(context);
    struct handlebars_value * value;
    struct handlebars_map_entry * entry;
    struct handlebars_map_entry * tmp_entry;

    srand(0x5d0);

    for( i = 0; i < count; i++ ) {
        for( j = 0; j < (rand() % STRSIZE); j++ ) {
            tmp[j] = mkchar(rand());
        }
        tmp[j] = 0;

        if( !handlebars_map_str_find(map, tmp, strlen(tmp)) ) {
            value = talloc_steal(map, handlebars_value_ctor(context));
            handlebars_value_integer(value, pos++);
            handlebars_map_str_add(map, tmp, strlen(tmp), value);
        }
    }

    fprintf(stderr, "ENTRIES: %ld, TABLE SIZE: %ld, COLLISIONS: %ld\n", map->i, map->table_size, map->collisions);

    pos = 0;
    handlebars_map_foreach(map, entry, tmp_entry) {
        ck_assert_uint_eq(pos++, handlebars_value_get_intval(entry->value));
    }

    while( map->first ) {
        struct handlebars_value * entry = handlebars_map_find(map, map->first->key);
        ck_assert_ptr_ne(NULL, entry);
        handlebars_map_remove(map, map->first->key);
        ck_assert_uint_eq(--pos, map->i);
    }


    handlebars_map_dtor(map);
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Map");

    REGISTER_TEST_FIXTURE(s, test_map, "Map");

    return s;
}

int main(void)
{
    int number_failed;
    int memdebug;
    int error;

    talloc_set_log_stderr();

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }

    // Set up test suite
    Suite * s = parser_suite();
    SRunner * sr = srunner_create(s);
    if( IS_WIN || memdebug ) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    error = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

    // Generate report for memdebug
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }

    // Return
    return error;
}

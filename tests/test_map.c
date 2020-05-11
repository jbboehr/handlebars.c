 /**
 * Copyright (C) 2020 John Boehr
 *
 * This file is part of handlebars.c.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_value.h"
#include "utils.h"



char mkchar(unsigned long i) {
    return (char) (32 + (i % (126 - 32)));
}

START_TEST(test_map)
{
#define STRSIZE 128
    size_t i;
    size_t pos = 0;
    size_t count = 10000;
    struct handlebars_map * map = handlebars_map_ctor(context, 0);
    struct handlebars_value * value;
    struct handlebars_string ** strings = handlebars_talloc_array(context, struct handlebars_string *, count);
    struct handlebars_string * key;

    // Seed so it's determinisitic
    srand(0x5d0);

    // Generate a bunch of random strings
    for( i = 0; i < count; i++ ) {
        char tmp[STRSIZE];
        size_t l = (rand() % (STRSIZE - 4)) + 4;
        size_t j;
        for( j = 0; j < l; j++ ) {
            tmp[j] = mkchar(rand());
        }
        tmp[j] = 0;
        strings[i] = handlebars_string_ctor(context, tmp, j - 1);
    }

    // Add them all to the map
    for( i = 0; i < count; i++ ) {
        key = strings[i];

        // There can be duplicate strings - skip those
        if (handlebars_map_find(map, key)) {
            continue;
        }

        value = talloc_steal(map, handlebars_value_ctor(context));
        handlebars_value_integer(value, pos++);
        handlebars_map_add(map, key, value);
    }

    fprintf(
        stderr,
        "ENTRIES: %ld, "
        // "TABLE SIZE: %ld, "
        // "COLLISIONS: %ld, "
        "LOADFACTOR: %d\n",
        handlebars_map_count(map),
        // map->table_capacity,
        // map->collisions,
        handlebars_map_load_factor(map)
    );

    // Make sure we can iterate over the map in insertion order
    pos = 0;
    handlebars_map_foreach(map, index, key, value) {
        ck_assert_uint_eq(index, handlebars_value_get_intval(value));
    } handlebars_map_foreach_end();

    // Remove everything
    i = 0;
    pos = handlebars_map_count(map);
    handlebars_map_foreach(map, index, key, value) {
        ck_assert_ptr_ne(key, NULL);
        ck_assert_ptr_ne(value, NULL);

        handlebars_map_remove(map, key);

        // make sure the count of items in the map is accurate
        ck_assert_uint_eq(--pos, handlebars_map_count(map));

        // make sure the right element was removed
        ck_assert_int_eq(i++, handlebars_value_get_intval(value));
    } handlebars_map_foreach_end();

    // Make sure it's empty
    ck_assert_uint_eq(handlebars_map_count(map), 0);

    // Free
    handlebars_map_dtor(map);
}
END_TEST

int map_sort_test_compare(const struct handlebars_map_kv_pair * kv_pair1, const struct handlebars_map_kv_pair * kv_pair2)
{
    ck_assert_ptr_ne(kv_pair1, NULL);
    ck_assert_ptr_ne(kv_pair2, NULL);
    return handlebars_value_get_intval(kv_pair2->value) - handlebars_value_get_intval(kv_pair1->value);
}

static const void * COMPARE_R_ARG = (void *) 0x0F;

int map_sort_test_compare_r(const struct handlebars_map_kv_pair * kv_pair1, const struct handlebars_map_kv_pair * kv_pair2, const void * arg)
{
    ck_assert_ptr_ne(kv_pair1, NULL);
    ck_assert_ptr_ne(kv_pair2, NULL);
    ck_assert_ptr_eq(arg, COMPARE_R_ARG);
    return handlebars_value_get_intval(kv_pair1->value) - handlebars_value_get_intval(kv_pair2->value);
}

START_TEST(test_map_sort)
{
    size_t count = 33;
    size_t capacity = 64;
    size_t middle = count / 2;
    struct handlebars_map * map = handlebars_map_ctor(context, capacity);
    size_t i;

    for ( i = 0; i < count; i++ ) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp) - 1, "%lu", i);
        struct handlebars_string * key = handlebars_string_ctor(HBSCTX(context), tmp, strlen(tmp));

        struct handlebars_value * value = handlebars_value_ctor(context);
        handlebars_value_integer(value, i);
        ck_assert_ptr_ne(value, NULL);
        handlebars_map_add(map, key, value);
        handlebars_talloc_free(key);
    }

    do {
        char tmp[32];
        snprintf(tmp, sizeof(tmp) - 1, "%lu", middle);
        struct handlebars_string * key = handlebars_string_ctor(HBSCTX(context), tmp, strlen(tmp));
        handlebars_map_remove(map, key);
        handlebars_talloc_free(key);
    } while(0);

    map = handlebars_map_sort(map, map_sort_test_compare);

    size_t fudge = 0;
    for ( i = 0; i < count; i++ ) {
        if (i == middle) {
            fudge = 1;
            continue;
        }

        struct handlebars_string * key = handlebars_map_get_key_at_index(map, i - fudge);
        ck_assert_ptr_ne(key, NULL);

        char tmp[32];
        snprintf(tmp, sizeof(tmp) - 1, "%lu", count - i - 1);
        ck_assert_str_eq(tmp, hbs_str_val(key));

        struct handlebars_value * value = handlebars_map_find(map, key);
        ck_assert_ptr_ne(value, NULL);
        ck_assert_int_eq(count - i - 1, handlebars_value_get_intval(value));
    }

    map = handlebars_map_sort_r(map, map_sort_test_compare_r, COMPARE_R_ARG);

    fudge = 0;
    for ( i = 0; i < count; i++ ) {
        if (i == middle) {
            fudge = 1;
            continue;
        }

        struct handlebars_string * key = handlebars_map_get_key_at_index(map, i - fudge);
        ck_assert_ptr_ne(key, NULL);

        char tmp[32];
        snprintf(tmp, sizeof(tmp) - 1, "%lu", i);
        ck_assert_str_eq(tmp, hbs_str_val(key));

        struct handlebars_value * value = handlebars_map_find(map, key);
        ck_assert_ptr_ne(value, NULL);
        ck_assert_int_eq(i, handlebars_value_get_intval(value));
    }
}
END_TEST

Suite * parser_suite(void)
{
    Suite * s = suite_create("Map");

    REGISTER_TEST_FIXTURE(s, test_map, "Map");
#ifdef HAVE_QSORT_R
    REGISTER_TEST_FIXTURE(s, test_map_sort, "handlebars_map_sort");
#endif

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

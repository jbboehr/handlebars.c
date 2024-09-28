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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <check.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

#include "utils.h"



char mkchar(unsigned long i);
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
    struct handlebars_string ** strings = handlebars_talloc_array(context, struct handlebars_string *, count);

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
        struct handlebars_string * key = strings[i];

        // There can be duplicate strings - skip those
        if (handlebars_map_find(map, key)) {
            continue;
        }

        HANDLEBARS_VALUE_DECL(value);
        handlebars_value_integer(value, pos++);
        map = handlebars_map_add(map, key, value);
        HANDLEBARS_VALUE_UNDECL(value);
    }

    fprintf(
        stderr,
        "ENTRIES: %zu, "
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
    } handlebars_map_foreach_end(map);

    // Remove everything
    i = 0;
    pos = handlebars_map_count(map);
    handlebars_map_foreach(map, index, key, value) {
        ck_assert_ptr_ne(key, NULL);
        ck_assert_ptr_ne(value, NULL);

        long intval = handlebars_value_get_intval(value);

        map = handlebars_map_remove(map, key);

        // make sure the count of items in the map is accurate
        ck_assert_uint_eq(--pos, handlebars_map_count(map));

        // make sure the right element was removed
        ck_assert_int_eq(i++, intval);
    } handlebars_map_foreach_end(map);

    // Make sure it's empty
    ck_assert_uint_eq(handlebars_map_count(map), 0);

    // Free
    handlebars_map_dtor(map);
}
END_TEST

int map_sort_test_compare(const struct handlebars_map_kv_pair * kv_pair1, const struct handlebars_map_kv_pair * kv_pair2);
int map_sort_test_compare(const struct handlebars_map_kv_pair * kv_pair1, const struct handlebars_map_kv_pair * kv_pair2)
{
    ck_assert_ptr_ne(kv_pair1, NULL);
    ck_assert_ptr_ne(kv_pair2, NULL);
    return handlebars_value_get_intval(kv_pair2->value) - handlebars_value_get_intval(kv_pair1->value);
}

static const void * COMPARE_R_ARG = (void *) 0x0F;

int map_sort_test_compare_r(const struct handlebars_map_kv_pair * kv_pair1, const struct handlebars_map_kv_pair * kv_pair2, const void * arg);
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
        snprintf(tmp, sizeof(tmp) - 1, "%zu", i);
        struct handlebars_string * key = handlebars_string_ctor(HBSCTX(context), tmp, strlen(tmp));
        handlebars_string_addref(key);

        HANDLEBARS_VALUE_DECL(value);
        handlebars_value_integer(value, i);
        map = handlebars_map_add(map, key, value);
        handlebars_string_delref(key);
        HANDLEBARS_VALUE_UNDECL(value);
    }

    do {
        char tmp[32];
        snprintf(tmp, sizeof(tmp) - 1, "%zu", middle);
        struct handlebars_string * key = handlebars_string_ctor(HBSCTX(context), tmp, strlen(tmp));
        handlebars_string_addref(key);
        map = handlebars_map_remove(map, key);
        handlebars_string_delref(key);
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
        snprintf(tmp, sizeof(tmp) - 1, "%zu", count - i - 1);
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
        snprintf(tmp, sizeof(tmp) - 1, "%zu", i);
        ck_assert_str_eq(tmp, hbs_str_val(key));

        struct handlebars_value * value = handlebars_map_find(map, key);
        ck_assert_ptr_ne(value, NULL);
        ck_assert_int_eq(i, handlebars_value_get_intval(value));
    }
}
END_TEST

START_TEST(test_map_copy_ctor)
{
    struct handlebars_map * map;
    struct handlebars_map * map_copy;
    struct handlebars_string * str1;
    struct handlebars_string * str2;
    struct handlebars_string * str3;
    HANDLEBARS_VALUE_DECL(tmp);

    map = handlebars_map_ctor(context, 3);
    handlebars_map_addref(map);

    handlebars_value_integer(tmp, 1);
    str1 = handlebars_string_ctor(context, HBS_STRL("a"));
    handlebars_string_addref(str1);
    map = handlebars_map_update(map, str1, tmp);

    handlebars_value_integer(tmp, 2);
    str2 = handlebars_string_ctor(context, HBS_STRL("b"));
    handlebars_string_addref(str2);
    map = handlebars_map_update(map, str2, tmp);

    handlebars_value_integer(tmp, 3);
    str3 = handlebars_string_ctor(context, HBS_STRL("c"));
    handlebars_string_addref(str3);
    map = handlebars_map_update(map, str3, tmp);

    ck_assert_uint_eq(handlebars_map_count(map), 3);

    map_copy = handlebars_map_copy_ctor(map, 0);

    ck_assert_ptr_ne(map_copy, NULL);
    ck_assert_ptr_ne(map, map_copy);
    ck_assert_uint_eq(handlebars_map_count(map_copy), 3);

    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str1)), handlebars_value_get_intval(handlebars_map_find(map_copy, str1)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str2)), handlebars_value_get_intval(handlebars_map_find(map_copy, str2)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str3)), handlebars_value_get_intval(handlebars_map_find(map_copy, str3)));

    handlebars_string_delref(str1);
    handlebars_string_delref(str2);
    handlebars_string_delref(str3);

    handlebars_map_delref(map);
    handlebars_map_delref(map_copy);

    HANDLEBARS_VALUE_UNDECL(tmp);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifndef HANDLEBARS_NO_REFCOUNT
START_TEST(test_map_add_with_separation)
{
    struct handlebars_map * map;
    struct handlebars_map * map_copy;
    struct  handlebars_string * str1;
    struct  handlebars_string * str2;
    struct  handlebars_string * str3;
    HANDLEBARS_VALUE_DECL(tmp);

    map = handlebars_map_ctor(context, 3);
    handlebars_map_addref(map);

    handlebars_value_integer(tmp, 1);
    str1 = handlebars_string_ctor(context, HBS_STRL("a"));
    handlebars_string_addref(str1);
    map = handlebars_map_update(map, str1, tmp);

    handlebars_value_integer(tmp, 2);
    str2 = handlebars_string_ctor(context, HBS_STRL("b"));
    handlebars_string_addref(str2);
    map = handlebars_map_update(map, str2, tmp);

    handlebars_map_addref(map);
    map_copy = map;

    handlebars_value_integer(tmp, 3);
    str3 = handlebars_string_ctor(context, HBS_STRL("c"));
    handlebars_string_addref(str3);
    map = handlebars_map_update(map, str3, tmp);

    ck_assert_uint_eq(handlebars_map_count(map), 3);

    ck_assert_ptr_ne(map_copy, NULL);
    ck_assert_ptr_ne(map, map_copy);
    ck_assert_uint_eq(handlebars_map_count(map_copy), 2);

    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str1)), handlebars_value_get_intval(handlebars_map_find(map_copy, str1)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str2)), handlebars_value_get_intval(handlebars_map_find(map_copy, str2)));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, str3)), 3);

    handlebars_string_delref(str1);
    handlebars_string_delref(str2);
    handlebars_string_delref(str3);

    handlebars_map_delref(map);
    handlebars_map_delref(map_copy);

    HANDLEBARS_VALUE_UNDECL(tmp);
    ASSERT_INIT_BLOCKS();
}
END_TEST
#endif

START_TEST(test_map_sizeof)
{
    ck_assert_int_gt(handlebars_map_size_of(0), 0);
    ck_assert_int_gt(handlebars_map_size_of(100), handlebars_map_size_of(50));
}
END_TEST

START_TEST(test_map_remove_nonexist)
{
    struct handlebars_map * map;
    struct handlebars_string * str1;

    map = handlebars_map_ctor(context, 3);
    str1 = handlebars_string_ctor(context, HBS_STRL("a"));
    map = handlebars_map_remove(map, str1);
    handlebars_string_delref(str1);
    handlebars_map_delref(map);

    ASSERT_INIT_BLOCKS();
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Map");

    REGISTER_TEST_FIXTURE(s, test_map, "Map");
    tcase_set_timeout(tc_test_map, 30);
    REGISTER_TEST_FIXTURE(s, test_map_sort, "handlebars_map_sort");
    REGISTER_TEST_FIXTURE(s, test_map_copy_ctor, "Map copy constructor");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_map_add_with_separation, "Map add with separation");
#endif
    REGISTER_TEST_FIXTURE(s, test_map_sizeof, "Map sizeof");
    REGISTER_TEST_FIXTURE(s, test_map_remove_nonexist, "Map remove noexistent key");

    return s;
}

int main(void)
{
    return default_main(&suite);
}

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

static struct handlebars_map * make_full_map(void)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 4);
    HANDLEBARS_VALUE_DECL(value);

    for( long i = 0; i < 4; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(value, i + 1);
        map = handlebars_map_str_add(map, key, 1, value);
    }

    HANDLEBARS_VALUE_UNDECL(value);
    return map;
}

static struct handlebars_value * map_foreach_return_first(
    struct handlebars_map * map
)
{
    handlebars_map_foreach(map, index, key, value) {
        (void) index;
        (void) key;
        return value;
    } handlebars_map_foreach_end(map);

    return NULL;
}

static void map_foreach_goto_after_first(struct handlebars_map * map)
{
    handlebars_map_foreach(map, index, key, value) {
        (void) index;
        (void) key;
        (void) value;
        goto done;
    } handlebars_map_foreach_end(map);

done:
    return;
}

START_TEST(test_map_foreach_cleans_up_after_return)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_map * original = map;
    struct handlebars_value * value = map_foreach_return_first(map);

    ck_assert_ptr_nonnull(value);
    ck_assert_int_eq(handlebars_value_get_intval(value), 1);
    map = handlebars_map_rehash(map, true);
    ck_assert_ptr_ne(map, original);

    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_map_foreach_cleans_up_after_goto)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_map * original = map;

    map_foreach_goto_after_first(map);
    map = handlebars_map_rehash(map, true);
    ck_assert_ptr_ne(map, original);

    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_map_foreach_cleans_up_after_longjmp)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_map * original = map;
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_ptr_null(context->e->iterator_cleanup);
        clear_intentional_error();
        map = handlebars_map_rehash(map, true);
        ck_assert_ptr_ne(map, original);
        handlebars_map_delref(map);
        return;
    }

    handlebars_map_foreach(map, index, key, value) {
        (void) index;
        (void) key;
        (void) value;
        handlebars_throw(context, HANDLEBARS_ERROR, "Intentional map iterator failure");
    } handlebars_map_foreach_end(map);
    ck_abort_msg("Expected map iteration to throw");
}
END_TEST

START_TEST(test_map_iterator_api_skips_sparse_entries)
{
    struct handlebars_map_iterator iterator = {0};
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    struct handlebars_map * original;
    struct handlebars_string * key = (struct handlebars_string *) 1;
    struct handlebars_value * child = (struct handlebars_value *) 1;
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_integer(value, 1);
    map = handlebars_map_str_add(map, HBS_STRL("a"), value);
    handlebars_value_integer(value, 2);
    map = handlebars_map_str_add(map, HBS_STRL("b"), value);
    handlebars_value_integer(value, 3);
    map = handlebars_map_str_add(map, HBS_STRL("c"), value);
    map = handlebars_map_str_remove(map, HBS_STRL("b"));
    original = map;

    ck_assert(handlebars_map_iterator_init(&iterator, map));
    ck_assert(handlebars_map_iterator_next(&iterator, &key, &child));
    ck_assert_uint_eq(iterator.iterator.index, 0);
    ck_assert_hbs_str_eq_cstr(key, "a");
    ck_assert_int_eq(handlebars_value_get_intval(child), 1);
    ck_assert(handlebars_map_iterator_next(&iterator, &key, &child));
    ck_assert_uint_eq(iterator.iterator.index, 2);
    ck_assert_hbs_str_eq_cstr(key, "c");
    ck_assert_int_eq(handlebars_value_get_intval(child), 3);
    ck_assert(!handlebars_map_iterator_next(&iterator, &key, &child));
    ck_assert_ptr_null(key);
    ck_assert_ptr_null(child);
    map = handlebars_map_rehash(map, true);
    ck_assert_ptr_ne(map, original);
    handlebars_map_iterator_close(&iterator);

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_empty_map_iterator_api_is_closed)
{
    struct handlebars_map_iterator iterator = {0};
    struct handlebars_map * map = handlebars_map_ctor(context, 0);
    struct handlebars_string * key = (struct handlebars_string *) 1;
    struct handlebars_value * value = (struct handlebars_value *) 1;

    ck_assert(!handlebars_map_iterator_init(&iterator, map));
    ck_assert(!handlebars_map_iterator_next(&iterator, &key, &value));
    ck_assert_ptr_null(key);
    ck_assert_ptr_null(value);
    handlebars_map_iterator_close(&iterator);
    handlebars_map_iterator_close(&iterator);

    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_nested_map_iterators_keep_outer_lock)
{
    struct handlebars_map_iterator outer = {0};
    struct handlebars_map_iterator inner = {0};
    struct handlebars_map * map = make_full_map();
    struct handlebars_map * original = map;
    struct handlebars_string * key;
    struct handlebars_value * value;

    ck_assert(handlebars_map_iterator_init(&outer, map));
    ck_assert(handlebars_map_iterator_next(&outer, &key, &value));
    ck_assert(handlebars_map_iterator_init(&inner, map));
    ck_assert(handlebars_map_iterator_next(&inner, &key, &value));
    handlebars_map_iterator_close(&inner);

    map = handlebars_map_rehash(map, true);
    ck_assert_ptr_eq(map, original);
    ck_assert(handlebars_map_iterator_next(&outer, &key, &value));
    handlebars_map_iterator_close(&outer);

    map = handlebars_map_rehash(map, true);
    ck_assert_ptr_ne(map, original);
    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_nested_map_iterators_close_out_of_order_keep_inner_lock)
{
    struct handlebars_map_iterator outer = {0};
    struct handlebars_map_iterator inner = {0};
    struct handlebars_map * map = make_full_map();

    ck_assert(handlebars_map_iterator_init(&outer, map));
    ck_assert(handlebars_map_iterator_init(&inner, map));
    handlebars_map_iterator_close(&outer);

    ck_assert_ptr_eq(handlebars_map_rehash(map, true), map);

    handlebars_map_iterator_close(&inner);
    handlebars_map_dtor(map);
}
END_TEST

START_TEST(test_map_compaction_is_deferred_during_iteration)
{
    struct handlebars_map_iterator iterator = {0};
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    struct handlebars_string * key;
    struct handlebars_value * child;
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_integer(value, 1);
    map = handlebars_map_str_add(map, HBS_STRL("a"), value);
    handlebars_value_integer(value, 2);
    map = handlebars_map_str_add(map, HBS_STRL("b"), value);
    handlebars_value_integer(value, 3);
    map = handlebars_map_str_add(map, HBS_STRL("c"), value);
    map = handlebars_map_str_remove(map, HBS_STRL("a"));

    ck_assert(handlebars_map_iterator_init(&iterator, map));
    ck_assert(handlebars_map_iterator_next(&iterator, &key, &child));
    ck_assert_hbs_str_eq_cstr(key, "b");
    ck_assert_int_eq(handlebars_value_get_intval(child), 2);

    handlebars_map_sparse_array_compact(map);
    ck_assert(handlebars_map_is_sparse(map));
    ck_assert_hbs_str_eq_cstr(key, "b");
    ck_assert_int_eq(handlebars_value_get_intval(child), 2);
    ck_assert(handlebars_map_iterator_next(&iterator, &key, &child));
    ck_assert_hbs_str_eq_cstr(key, "c");
    ck_assert_int_eq(handlebars_value_get_intval(child), 3);
    handlebars_map_iterator_close(&iterator);

    handlebars_map_sparse_array_compact(map);
    ck_assert(!handlebars_map_is_sparse(map));
    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
}
END_TEST

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

START_TEST(test_map_sort_rejects_active_iterator)
{
    struct handlebars_map_iterator iterator = {0};
    struct handlebars_map * map = make_full_map();
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = previous;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_null(context->e->iterator_cleanup);
        clear_intentional_error();
        map = handlebars_map_sort(map, map_sort_test_compare);
        handlebars_map_delref(map);
        return;
    }

    ck_assert(handlebars_map_iterator_init(&iterator, map));
    map = handlebars_map_sort(map, map_sort_test_compare);
    context->e->jmp = previous;
    handlebars_map_iterator_close(&iterator);
    handlebars_map_delref(map);
    ck_abort_msg("Expected sorting during map iteration to throw");
}
END_TEST

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

START_TEST(test_map_string_apis_release_temporary_keys)
{
    size_t outer_blocks = talloc_total_blocks(context);
    struct handlebars_context * owner = handlebars_context_ctor_ex(context);
    struct handlebars_map * map = handlebars_map_ctor(owner, 8);
    HANDLEBARS_VALUE_DECL(value);

    size_t map_blocks = talloc_total_blocks(owner);
    handlebars_value_integer(value, 1);
    map = handlebars_map_str_add(map, HBS_STRL("present"), value);
    map = handlebars_map_str_add(map, HBS_STRL("second"), value);
    map = handlebars_map_str_add(map, HBS_STRL("third"), value);
    ck_assert_uint_eq(talloc_total_blocks(owner), map_blocks + 3);

    map_blocks = talloc_total_blocks(owner);
    for( size_t i = 0; i < 100; i++ ) {
        ck_assert_ptr_nonnull(handlebars_map_str_find(map, HBS_STRL("present")));
        ck_assert_ptr_null(handlebars_map_str_find(map, HBS_STRL("missing")));
        map = handlebars_map_str_update(map, HBS_STRL("present"), value);
        map = handlebars_map_str_remove(map, HBS_STRL("missing"));
    }
    ck_assert_uint_eq(talloc_total_blocks(owner), map_blocks);

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_dtor(map);
    handlebars_context_dtor(owner);
    ck_assert_uint_eq(talloc_total_blocks(context), outer_blocks);
}
END_TEST

START_TEST(test_map_remove_nonexist)
{
    struct handlebars_map * map;
    struct handlebars_map * original;
    struct handlebars_string * str1;
    size_t blocks;

    map = handlebars_map_ctor(context, 64);
    str1 = handlebars_string_ctor(context, HBS_STRL("a"));
    original = map;
    blocks = talloc_total_blocks(context);
    map = handlebars_map_remove(map, str1);
    ck_assert_ptr_eq(map, original);
    ck_assert_uint_eq(talloc_total_blocks(context), blocks);
    handlebars_string_delref(str1);
    handlebars_map_delref(map);

    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_remove_shrinks_low_load_map)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 64);
    HANDLEBARS_VALUE_DECL(value);
    size_t old_size;

    for( long i = 0; i < 4; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(value, i + 1);
        map = handlebars_map_str_add(map, key, 1, value);
    }

    old_size = talloc_get_size(map);
    map = handlebars_map_str_remove(map, HBS_STRL("a"));

    ck_assert_uint_eq(handlebars_map_count(map), 3);
    ck_assert_uint_lt(talloc_get_size(map), old_size);
    ck_assert_ptr_null(handlebars_map_str_find(map, HBS_STRL("a")));
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("b"))), 2);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("c"))), 3);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("d"))), 4);

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
    ASSERT_INIT_BLOCKS();
}
END_TEST

START_TEST(test_map_duplicate_after_rehash_preserves_original)
{
    struct handlebars_map * map = make_full_map();
    HANDLEBARS_VALUE_DECL(value);
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    handlebars_value_integer(value, 5);
    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_uint_eq(handlebars_map_count(map), 4);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
        HANDLEBARS_VALUE_UNDECL(value);
        handlebars_map_delref(map);
        return;
    }

    ck_assert_ptr_nonnull(handlebars_map_str_add(map, HBS_STRL("a"), value));
    context->e->jmp = prev;
    ck_abort_msg("Expected duplicate map key to be rejected");
}
END_TEST

START_TEST(test_map_add_while_iterating_full_map_preserves_original)
{
    struct handlebars_map * map = make_full_map();
    HANDLEBARS_VALUE_DECL(value);
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    handlebars_value_integer(value, 5);
    handlebars_map_set_is_in_iteration(map, true);
    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_map_set_is_in_iteration(map, false);
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_uint_eq(handlebars_map_count(map), 4);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
        HANDLEBARS_VALUE_UNDECL(value);
        handlebars_map_delref(map);
        return;
    }

    ck_assert_ptr_nonnull(handlebars_map_str_add(map, HBS_STRL("e"), value));
    handlebars_map_set_is_in_iteration(map, false);
    context->e->jmp = prev;
    ck_abort_msg("Expected insertion into an iteration-locked full map to fail");
}
END_TEST

START_TEST(test_map_add_preserves_aliased_value_during_rehash)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_value * source = handlebars_map_str_find(map, HBS_STRL("a"));

    ck_assert_ptr_nonnull(source);
    map = handlebars_map_str_add(map, HBS_STRL("e"), source);

    ck_assert_uint_eq(handlebars_map_count(map), 5);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("e"))), 1);
    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_map_update_preserves_aliased_value_during_rehash)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_value * source = handlebars_map_str_find(map, HBS_STRL("a"));

    ck_assert_ptr_nonnull(source);
    map = handlebars_map_str_update(map, HBS_STRL("e"), source);

    ck_assert_uint_eq(handlebars_map_count(map), 5);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("e"))), 1);
    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_map_rehash_preserves_aliased_reference_value)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 4);
    struct handlebars_value * source;
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("source")));
    map = handlebars_map_str_add(map, HBS_STRL("a"), value);
    for( long i = 0; i < 3; i++ ) {
        char key[2] = {(char) ('b' + i), '\0'};
        handlebars_value_integer(value, i + 2);
        map = handlebars_map_str_add(map, key, 1, value);
    }

    source = handlebars_map_str_find(map, HBS_STRL("a"));
    ck_assert_ptr_nonnull(source);
    map = handlebars_map_str_add(map, HBS_STRL("e"), source);

    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(handlebars_map_str_find(map, HBS_STRL("a"))), "source");
    ck_assert_hbs_str_eq_cstr(handlebars_value_get_string(handlebars_map_str_find(map, HBS_STRL("e"))), "source");

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
    ASSERT_INIT_BLOCKS();
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static bool map_mutation_with_alloc_failure(
    struct handlebars_map * map,
    struct handlebars_string * key,
    struct handlebars_value * value,
    struct handlebars_map ** result,
    int fail_at,
    bool update
)
{
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        return true;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    *result = update
        ? handlebars_map_update(map, key, value)
        : handlebars_map_add(map, key, value);
    handlebars_memory_fail_disable();
    context->e->jmp = previous;
    return false;
}

static void assert_map_growth_nomem_preserves_original(bool update)
{
    struct handlebars_map * map = make_full_map();
    struct handlebars_map * result = map;
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("e"));
    jmp_buf * previous = context->e->jmp;
    size_t blocks_before;
    int fail_at;
    HANDLEBARS_VALUE_DECL(value);

    handlebars_string_addref(key);
    handlebars_value_integer(value, 5);
    blocks_before = talloc_total_blocks(context);

    for( fail_at = 1; fail_at < 16; fail_at++ ) {
        result = map;
        bool failed = map_mutation_with_alloc_failure(
                map,
                key,
                value,
                &result,
                fail_at,
                update
            );
        ck_assert_ptr_eq(context->e->jmp, previous);
        if( !failed ) {
            map = result;
            break;
        }

        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_map_count(map), 4);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
        ck_assert_ptr_null(handlebars_map_find(map, key));
        handlebars_error_clear(context);
        ck_assert_msg(
            talloc_total_blocks(context) == blocks_before,
            "failed map %s leaked at allocation %d: before=%zu after=%zu",
            update ? "update" : "add",
            fail_at,
            blocks_before,
            talloc_total_blocks(context)
        );
    }
    ck_assert_int_lt(fail_at, 16);
    ck_assert_uint_eq(handlebars_map_count(map), 5);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_find(map, key)), 5);

    handlebars_string_delref(key);
    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
}

START_TEST(test_map_rehash_nomem_preserves_original)
{
    assert_map_growth_nomem_preserves_original(false);
    assert_map_growth_nomem_preserves_original(true);
}
END_TEST

enum map_string_test_mutation {
    MAP_STRING_TEST_REMOVE,
    MAP_STRING_TEST_ADD,
    MAP_STRING_TEST_UPDATE
};

static bool map_string_mutation_with_alloc_failure(
    struct handlebars_map * map,
    struct handlebars_value * value,
    struct handlebars_map ** result,
    int fail_at,
    enum map_string_test_mutation mutation
)
{
    jmp_buf * volatile previous = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        return true;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    switch( mutation ) {
        case MAP_STRING_TEST_REMOVE:
            *result = handlebars_map_str_remove(map, HBS_STRL("a"));
            break;
        case MAP_STRING_TEST_ADD:
            *result = handlebars_map_str_add(map, HBS_STRL("e"), value);
            break;
        case MAP_STRING_TEST_UPDATE:
            *result = handlebars_map_str_update(map, HBS_STRL("e"), value);
            break;
        default:
            ck_abort_msg("Invalid map string mutation");
    }
    handlebars_memory_fail_disable();
    context->e->jmp = previous;
    return false;
}

static void assert_map_string_growth_nomem_releases_temporary_key(bool update)
{
    jmp_buf * previous = context->e->jmp;
    int fail_at;

    for( fail_at = 1; fail_at < 16; fail_at++ ) {
        struct handlebars_map * map = make_full_map();
        struct handlebars_map * result = map;
        size_t blocks_before = talloc_total_blocks(context);
        bool failed;
        HANDLEBARS_VALUE_DECL(value);

        handlebars_value_integer(value, 5);
        failed = map_string_mutation_with_alloc_failure(
            map,
            value,
            &result,
            fail_at,
            update ? MAP_STRING_TEST_UPDATE : MAP_STRING_TEST_ADD
        );
        ck_assert_ptr_eq(context->e->jmp, previous);
        if( !failed ) {
            map = result;
            ck_assert_uint_eq(handlebars_map_count(map), 5);
            ck_assert_int_eq(
                handlebars_value_get_intval(
                    handlebars_map_str_find(map, HBS_STRL("e"))
                ),
                5
            );
            HANDLEBARS_VALUE_UNDECL(value);
            handlebars_map_delref(map);
            break;
        }

        handlebars_memory_fail_disable();
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_map_count(map), 4);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
        ck_assert_ptr_null(handlebars_map_str_find(map, HBS_STRL("e")));
        handlebars_error_clear(context);
        ck_assert_msg(
            talloc_total_blocks(context) == blocks_before,
            "failed map string %s leaked its temporary key: before=%zu after=%zu",
            update ? "update" : "add",
            blocks_before,
            talloc_total_blocks(context)
        );
        HANDLEBARS_VALUE_UNDECL(value);
        handlebars_map_delref(map);
    }
    ck_assert_int_lt(fail_at, 16);
}

START_TEST(test_map_str_add_nomem_releases_temporary_key)
{
    assert_map_string_growth_nomem_releases_temporary_key(false);
}
END_TEST

START_TEST(test_map_str_update_nomem_releases_temporary_key)
{
    assert_map_string_growth_nomem_releases_temporary_key(true);
}
END_TEST

START_TEST(test_map_str_remove_nomem_releases_temporary_key)
{
    jmp_buf * previous = context->e->jmp;
    int fail_at;

    for( fail_at = 1; fail_at < 16; fail_at++ ) {
        struct handlebars_map * map = handlebars_map_ctor(context, 64);
        struct handlebars_map * result = map;
        size_t blocks_before;
        bool failed;
        HANDLEBARS_VALUE_DECL(value);

        for( long i = 0; i < 4; i++ ) {
            char key[2] = {(char) ('a' + i), '\0'};
            handlebars_value_integer(value, i + 1);
            map = handlebars_map_str_add(map, key, 1, value);
        }
        result = map;
        blocks_before = talloc_total_blocks(context);
        failed = map_string_mutation_with_alloc_failure(
            map,
            NULL,
            &result,
            fail_at,
            MAP_STRING_TEST_REMOVE
        );
        ck_assert_ptr_eq(context->e->jmp, previous);
        if( !failed ) {
            map = result;
            ck_assert_uint_eq(handlebars_map_count(map), 3);
            ck_assert_ptr_null(handlebars_map_str_find(map, HBS_STRL("a")));
            ck_assert_int_eq(
                handlebars_value_get_intval(
                    handlebars_map_str_find(map, HBS_STRL("d"))
                ),
                4
            );
            HANDLEBARS_VALUE_UNDECL(value);
            handlebars_map_delref(map);
            break;
        }

        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_map_count(map), 4);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("a"))), 1);
        ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("d"))), 4);
        handlebars_error_clear(context);
        ck_assert_msg(
            talloc_total_blocks(context) == blocks_before,
            "failed map string remove leaked its temporary key: before=%zu after=%zu",
            blocks_before,
            talloc_total_blocks(context)
        );
        HANDLEBARS_VALUE_UNDECL(value);
        handlebars_map_delref(map);
    }
    ck_assert_int_lt(fail_at, 16);
}
END_TEST
#endif

START_TEST(test_map_sparse_array_compact_multiple_tombstones)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 8);
    HANDLEBARS_VALUE_DECL(value);

    for( long i = 0; i < 4; i++ ) {
        char key[2] = {(char) ('a' + i), '\0'};
        handlebars_value_integer(value, i);
        map = handlebars_map_str_add(map, key, 1, value);
    }

    map = handlebars_map_str_remove(map, HBS_STRL("a"));
    map = handlebars_map_str_remove(map, HBS_STRL("c"));
    ck_assert(handlebars_map_is_sparse(map));

    handlebars_map_sparse_array_compact(map);

    ck_assert(!handlebars_map_is_sparse(map));
    ck_assert_uint_eq(handlebars_map_count(map), 2);
    ck_assert_hbs_str_eq_cstr(handlebars_map_get_key_at_index(map, 0), "b");
    ck_assert_hbs_str_eq_cstr(handlebars_map_get_key_at_index(map, 1), "d");
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("b"))), 1);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("d"))), 3);

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
}
END_TEST

START_TEST(test_map_distinguishes_hash_collisions)
{
    struct handlebars_map * map = handlebars_map_ctor(context, 2);
    HANDLEBARS_VALUE_DECL(value);

    handlebars_value_integer(value, 1);
    map = handlebars_map_str_add(map, HBS_STRL("lwaaaa"), value);
    handlebars_value_integer(value, 2);
    map = handlebars_map_str_add(map, HBS_STRL("tnsaaa"), value);

    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("lwaaaa"))), 1);
    ck_assert_int_eq(handlebars_value_get_intval(handlebars_map_str_find(map, HBS_STRL("tnsaaa"))), 2);

    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_map_delref(map);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Map");

    REGISTER_TEST_FIXTURE(s, test_map, "Map");
    tcase_set_timeout(tc_test_map, 30);
    REGISTER_TEST_FIXTURE(s, test_map_foreach_cleans_up_after_return, "Map foreach cleans up after return");
    REGISTER_TEST_FIXTURE(s, test_map_foreach_cleans_up_after_goto, "Map foreach cleans up after goto");
    REGISTER_TEST_FIXTURE(s, test_map_foreach_cleans_up_after_longjmp, "Map foreach cleans up after longjmp");
    REGISTER_TEST_FIXTURE(s, test_map_iterator_api_skips_sparse_entries, "Map iterator API skips sparse entries");
    REGISTER_TEST_FIXTURE(s, test_empty_map_iterator_api_is_closed, "Empty map iterator API is closed");
    REGISTER_TEST_FIXTURE(s, test_nested_map_iterators_keep_outer_lock, "Nested map iterators keep the outer lock");
    REGISTER_TEST_FIXTURE(s, test_nested_map_iterators_close_out_of_order_keep_inner_lock, "Out-of-order nested iterator close keeps the inner lock");
    REGISTER_TEST_FIXTURE(s, test_map_compaction_is_deferred_during_iteration, "Map compaction is deferred during iteration");
    REGISTER_TEST_FIXTURE(s, test_map_sort_rejects_active_iterator, "Map sorting rejects an active iterator");
    REGISTER_TEST_FIXTURE(s, test_map_sort, "handlebars_map_sort");
    REGISTER_TEST_FIXTURE(s, test_map_copy_ctor, "Map copy constructor");
#ifndef HANDLEBARS_NO_REFCOUNT
    REGISTER_TEST_FIXTURE(s, test_map_add_with_separation, "Map add with separation");
#endif
    REGISTER_TEST_FIXTURE(s, test_map_sizeof, "Map sizeof");
    REGISTER_TEST_FIXTURE(s, test_map_string_apis_release_temporary_keys, "Map string APIs release temporary keys");
    REGISTER_TEST_FIXTURE(s, test_map_remove_nonexist, "Map remove nonexistent key");
    REGISTER_TEST_FIXTURE(s, test_map_remove_shrinks_low_load_map, "Map removal shrinks a low-load map");
    REGISTER_TEST_FIXTURE(s, test_map_duplicate_after_rehash_preserves_original, "Duplicate insertion preserves map after rehash");
    REGISTER_TEST_FIXTURE(s, test_map_add_while_iterating_full_map_preserves_original, "Insertion into an iteration-locked full map preserves the map");
    REGISTER_TEST_FIXTURE(s, test_map_add_preserves_aliased_value_during_rehash, "Map add preserves aliased values during rehash");
    REGISTER_TEST_FIXTURE(s, test_map_update_preserves_aliased_value_during_rehash, "Map update preserves aliased values during rehash");
    REGISTER_TEST_FIXTURE(s, test_map_rehash_preserves_aliased_reference_value, "Map rehash preserves aliased reference values");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_map_rehash_nomem_preserves_original, "Map remains usable after rehash allocation failure");
    REGISTER_TEST_FIXTURE(s, test_map_str_add_nomem_releases_temporary_key, "Map string add releases its key after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_map_str_update_nomem_releases_temporary_key, "Map string update releases its key after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_map_str_remove_nomem_releases_temporary_key, "Map string remove releases its key after allocation failure");
#endif
    REGISTER_TEST_FIXTURE(s, test_map_sparse_array_compact_multiple_tombstones, "Compact multiple sparse map entries");
    REGISTER_TEST_FIXTURE(s, test_map_distinguishes_hash_collisions, "Map distinguishes hash collisions");

    return s;
}

int main(void)
{
    return default_main(&suite);
}

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
#include <stdio.h>
#include <talloc.h>

#ifndef YY_NO_UNISTD_H
#include <unistd.h>
#endif

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_json.h"
#include "handlebars_map.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_helpers.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

#include "handlebars_cache_private.h"



struct cache_test_ctx {
    struct handlebars_string * tmpl;
    struct handlebars_compiler * compiler;
    struct handlebars_module * module;
};

char lmdb_db_file[] = "./handlebars-lmdb-cache-test.mdb";
char lmdb_db_lock_file[] = "./handlebars-lmdb-cache-test.mdb-lock";

static const char * tmpls[] = {
    "{{foo}}", "{{bar}}", "{{baz}}"
};

static struct cache_test_ctx * make_cache_test_ctx(int i, struct handlebars_cache * cache)
{
    struct cache_test_ctx * ctx = handlebars_talloc(context, struct cache_test_ctx);
    ctx->tmpl = handlebars_string_ctor(context, tmpls[i], strlen(tmpls[i]));
    ctx->compiler = handlebars_compiler_ctor(context);
    struct handlebars_module * module = handlebars_talloc_zero(context, struct handlebars_module);
    module->size = sizeof(struct handlebars_module);
    handlebars_cache_add(cache, ctx->tmpl, module);
    ctx->module = module;
    return ctx;
}



START_TEST(test_cache_gc_entries)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    size_t expected_size = sizeof(struct handlebars_module);

    struct cache_test_ctx * ctx0 = make_cache_test_ctx(0, cache);
    ctx0->module->ts = 3;
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_size, expected_size);

    struct cache_test_ctx * ctx1 = make_cache_test_ctx(1, cache);
    ctx1->module->ts = 2;
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_size, expected_size * 2);

    struct cache_test_ctx * ctx2 = make_cache_test_ctx(2, cache);
    ctx2->module->ts = 1;
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_size, expected_size * 3);

    // Garbage collection
    cache->max_entries = 1;
    handlebars_cache_gc(cache);

    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_size, expected_size);
    ck_assert_ptr_ne(NULL, handlebars_cache_find(cache, ctx0->tmpl));
    ck_assert_ptr_eq(NULL, handlebars_cache_find(cache, ctx1->tmpl));
    ck_assert_ptr_eq(NULL, handlebars_cache_find(cache, ctx2->tmpl));
}
END_TEST

static void execute_gc_test(struct handlebars_cache * cache)
{
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(helpers);

    handlebars_value_init_json_string(context, value, "{\"bar\": \"baz\"}");
    handlebars_value_convert(value);

    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{bar}}")));

    do {
        struct handlebars_map * tmp_map = handlebars_map_ctor(context, 0);
        tmp_map = handlebars_map_str_add(tmp_map, HBS_STRL("foo"), partial);
        handlebars_value_map(partials, tmp_map);
    } while (0);

    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(context, HBS_STRL("{{>foo}}")), 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);

    struct handlebars_module * module = handlebars_program_serialize(context, program);

    handlebars_value_map(helpers, handlebars_map_ctor(context, 0));
    handlebars_vm_set_helpers(vm, helpers);

    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, value);
    ck_assert_str_eq(hbs_str_val(buffer), "baz");

    int i;
    for( i = 0; i < 10; i++ ) {
        buffer = handlebars_vm_execute(vm, module, value);
        if (context->e->msg) {
            ck_abort_msg("ERROR: %s\n", context->e->msg);
        }
        ck_assert_str_eq(hbs_str_val(buffer), "baz");
    }

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 10);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // Test GC
    cache->max_age = 0;
    handlebars_cache_gc(cache);

    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(value);

    // @todo fixme
    //ck_assert_int_eq(0, handlebars_cache_stat(cache).current_entries);
}

static void execute_reset_test(struct handlebars_cache * cache)
{
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);
    HANDLEBARS_VALUE_DECL(helpers);
    struct handlebars_string * buffer;

    HANDLEBARS_VALUE_DECL(value);
    handlebars_value_init_json_string(context, value, "{\"bar\": \"baz\"}");
    handlebars_value_convert(value);

    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{bar}}")));

    do {
        struct handlebars_map * tmp_map = handlebars_map_ctor(context, 0);
        tmp_map = handlebars_map_str_add(tmp_map, HBS_STRL("foo"), partial);
        handlebars_value_map(partials, tmp_map);
    } while (0);

    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(context, HBS_STRL("{{>foo}}")), 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);

    struct handlebars_module * module = handlebars_program_serialize(context, program);

    handlebars_value_map(helpers, handlebars_map_ctor(context, 0));
    handlebars_vm_set_helpers(vm, helpers);

    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    // This shouldn't use the cache
    buffer = handlebars_vm_execute(vm, module, value);
    if (context->e->msg) {
        ck_abort_msg("ERROR: %s\n", context->e->msg);
    }
    ck_assert_str_eq(hbs_str_val(buffer), "baz");

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 0);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // This should use the cache
    buffer = handlebars_vm_execute(vm, module, value);
    if (context->e->msg) {
        ck_abort_msg("ERROR: %s\n", context->e->msg);
    }
    ck_assert_str_eq(hbs_str_val(buffer), "baz");

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 1);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // Reset
    handlebars_cache_reset(cache);

    // This shouldn't use the cache
    buffer = handlebars_vm_execute(vm, module, value);
    if (context->e->msg) {
        ck_abort_msg("ERROR: %s\n", context->e->msg);
    }
    ck_assert_str_eq(hbs_str_val(buffer), "baz");

    HANDLEBARS_VALUE_UNDECL(value);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 0);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);
}

START_TEST(test_simple_cache_gc)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_simple_cache_reset)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    execute_reset_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST

#ifdef HANDLEBARS_HAVE_LMDB
START_TEST(test_lmdb_cache_gc)
{
    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_lmdb_cache_reset)
{
    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    execute_reset_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST
#endif

#ifdef HANDLEBARS_HAVE_PTHREAD
START_TEST(test_mmap_cache_gc)
{
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_mmap_cache_reset)
{
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST
#endif

static Suite * suite(void);
static Suite * suite(void)
{
    const char * title = "Handlebars Spec";
    Suite * s = suite_create(title);

    REGISTER_TEST_FIXTURE(s, test_cache_gc_entries, "Garbage Collection");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc, "Simple Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset, "Simple Cache (Reset)");
#ifdef HANDLEBARS_HAVE_LMDB
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc, "LMDB Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset, "LMDB Cache (Reset)");
#endif
#ifdef HANDLEBARS_HAVE_PTHREAD
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_gc, "MMAP Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_reset, "MMAP Cache (Reset)");
#endif

    return s;
}

int main(void)
{
    unlink(lmdb_db_file);
    unlink(lmdb_db_lock_file);
    int exit_code = default_main(&suite);
    unlink(lmdb_db_file);
    unlink(lmdb_db_lock_file);
    return exit_code;
}

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
#include <stdio.h>
#include <talloc.h>
#include <unistd.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#include <src/handlebars_value.h>

#endif

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_helpers.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#include "utils.h"

static int memdebug;
static const char * tmpl1 = "{{foo}}";
static const char * tmpl2 = "{{bar}}";
static const char * tmpl3 = "{{baz}}";


struct cache_test_ctx {
    struct handlebars_string * tmpl;
    struct handlebars_compiler * compiler;
    struct handlebars_map_entry * map_entry;
    struct handlebars_module * module;
};

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
    struct handlebars_cache * cache = handlebars_cache_ctor(context);
    struct handlebars_compiler * compiler = handlebars_compiler_ctor(context);
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
    //struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"foo\": {\"bar\": \"baz\"}}");
    struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"bar\": \"baz\"}");
    handlebars_value_convert(value);

    struct handlebars_value * partial = handlebars_value_ctor(context);
    handlebars_value_stringl(partial, HBS_STRL("{{bar}}"));

    struct handlebars_value * partials = handlebars_value_ctor(context);
    handlebars_value_map_init(partials);
    handlebars_map_str_add(partials->v.map, HBS_STRL("foo"), partial);

    parser->tmpl = handlebars_string_ctor(context, HBS_STRL("{{>foo}}"));
    handlebars_parse(parser);
    handlebars_compiler_compile(compiler, parser->program);

    struct handlebars_module * module = handlebars_program_serialize(context, compiler->program);

    vm->helpers = handlebars_value_ctor(context);
    handlebars_value_map_init(vm->helpers);
    vm->partials = partials;

    vm->cache = cache;

    handlebars_vm_execute(vm, module, value);
    ck_assert_str_eq(vm->buffer->val, "baz");

    int i;
    for( i = 0; i < 10; i++ ) {
        handlebars_vm_execute(vm, module, value);
        ck_assert_str_eq(vm->buffer->val, "baz");
    }

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 10);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // Test GC
    cache->max_age = 0;
    cache->gc(cache);

    // @todo fixme
    //ck_assert_int_eq(0, handlebars_cache_stat(cache).current_entries);
}

static void execute_reset_test(struct handlebars_cache * cache)
{
    //struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"foo\": {\"bar\": \"baz\"}}");
    struct handlebars_value * value = handlebars_value_from_json_string(context, "{\"bar\": \"baz\"}");
    handlebars_value_convert(value);

    struct handlebars_value * partial = handlebars_value_ctor(context);
    handlebars_value_stringl(partial, HBS_STRL("{{bar}}"));

    struct handlebars_value * partials = handlebars_value_ctor(context);
    handlebars_value_map_init(partials);
    handlebars_map_str_add(partials->v.map, HBS_STRL("foo"), partial);

    parser->tmpl = handlebars_string_ctor(context, HBS_STRL("{{>foo}}"));
    handlebars_parse(parser);
    handlebars_compiler_compile(compiler, parser->program);

    struct handlebars_module * module = handlebars_program_serialize(context, compiler->program);

    vm->helpers = handlebars_value_ctor(context);
    handlebars_value_map_init(vm->helpers);
    vm->partials = partials;

    vm->cache = cache;

    // This shouldn't use the cache
    handlebars_vm_execute(vm, module, value);
    ck_assert_str_eq(vm->buffer->val, "baz");

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 0);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // This should use the cache
    handlebars_vm_execute(vm, module, value);
    ck_assert_str_eq(vm->buffer->val, "baz");

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 1);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);

    // Reset
    handlebars_cache_reset(cache);

    // This shouldn't use the cache
    handlebars_vm_execute(vm, module, value);
    ck_assert_str_eq(vm->buffer->val, "baz");

    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 0);
    ck_assert_int_le(handlebars_cache_stat(cache).misses, 1);
}

START_TEST(test_simple_cache_gc)
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
END_TEST

START_TEST(test_simple_cache_reset)
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    execute_reset_test(cache);
    handlebars_cache_dtor(cache);
END_TEST

#ifdef HAVE_LIBLMDB
START_TEST(test_lmdb_cache_gc)
    char tmp[256];
    snprintf(tmp, 256, "%s/%s", getenv("TMPDIR") ?: "/tmp", "handlebars-lmdb-cache-test.mdb");
    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, tmp);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
END_TEST

START_TEST(test_lmdb_cache_reset)
    char tmp[256];
    snprintf(tmp, 256, "%s/%s", getenv("TMPDIR") ?: "/tmp", "handlebars-lmdb-cache-test.mdb");
    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, tmp);
    execute_reset_test(cache);
    handlebars_cache_dtor(cache);
END_TEST
#else
START_TEST(test_lmdb_cache_error)
    jmp_buf buf;
    char tmp[256];
    snprintf(tmp, 256, "%s/%s", getenv("TMPDIR") ?: "/tmp", "handlebars-lmdb-cache-test.mdb");

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert(1);
        return;
    }

    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, tmp);
    ck_assert(0);
END_TEST
#endif

START_TEST(test_mmap_cache_gc)
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
END_TEST

START_TEST(test_mmap_cache_reset)
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    execute_gc_test(cache);
    handlebars_cache_dtor(cache);
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Spec";
    Suite * s = suite_create(title);

    REGISTER_TEST_FIXTURE(s, test_cache_gc_entries, "Garbage Collection");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc, "Simple Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset, "Simple Cache (Reset)");
#ifdef HAVE_LIBLMDB
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc, "LMDB Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset, "LMDB Cache (Reset)");
#else
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_error, "LMDB Cache (Error)");
#endif
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_gc, "MMAP Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_reset, "MMAP Cache (Reset)");

    return s;
}

int main(void)
{
    int number_failed;
    int error;

    // Check if memdebug enabled
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    root = talloc_new(NULL);

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

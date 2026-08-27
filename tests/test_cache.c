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
#include <stdint.h>
#include <stdio.h>
#include <talloc.h>

#ifdef HANDLEBARS_HAVE_PTHREAD
#include <pthread.h>
#endif

#ifndef YY_NO_UNISTD_H
#include <unistd.h>
#endif

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_json.h"
#include "handlebars_map.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_opcodes.h"
#include "handlebars_parser.h"
#include "handlebars_helpers.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

#include "handlebars_cache_private.h"

#ifdef HANDLEBARS_HAVE_LMDB
#include <lmdb.h>
#endif



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

static struct handlebars_module * serialize_template_with_flags(
    const char * tmpl,
    unsigned long flags
)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
    struct handlebars_ast_node * ast = handlebars_parse_ex(
        local_parser,
        handlebars_string_ctor(context, tmpl, strlen(tmpl)),
        flags
    );
    handlebars_compiler_set_flags(local_compiler, flags);
    struct handlebars_program * program = handlebars_compiler_compile_ex(local_compiler, ast);
    return handlebars_program_serialize(context, program);
}

static struct handlebars_module * serialize_template(const char * tmpl)
{
    return serialize_template_with_flags(tmpl, 0);
}

static bool simple_cache_module_destroyed;

static int simple_cache_module_dtor(struct handlebars_module * module)
{
    (void) module;
    simple_cache_module_destroyed = true;
    return 0;
}

#ifdef HANDLEBARS_HAVE_LMDB
static void reset_lmdb_test_files(void)
{
    unlink(lmdb_db_file);
    unlink(lmdb_db_lock_file);
}

static struct handlebars_module * serialize_template_for_lmdb(const char * tmpl)
{
    struct handlebars_module * module = serialize_template(tmpl);
    struct handlebars_module * copy = handlebars_talloc_size(context, module->size);

    ck_assert_ptr_nonnull(copy);
    memcpy(copy, module, module->size);
    handlebars_module_patch_pointers(copy);
    handlebars_module_normalize_pointers(copy, NULL);
    handlebars_module_generate_hash(copy);
    return copy;
}

static void lmdb_put_raw(const char * key_string, const void * bytes, size_t size)
{
    MDB_env * env;
    MDB_txn * txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    int err;

    err = mdb_env_create(&env);
    ck_assert_int_eq(err, 0);
    err = mdb_env_open(env, lmdb_db_file, MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOSUBDIR, 0644);
    ck_assert_int_eq(err, 0);
    err = mdb_txn_begin(env, NULL, 0, &txn);
    ck_assert_int_eq(err, 0);
    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    ck_assert_int_eq(err, 0);

    key.mv_size = strlen(key_string) + 1;
    key.mv_data = (void *) key_string;
    data.mv_size = size;
    data.mv_data = (void *) bytes;
    err = mdb_put(txn, dbi, &key, &data, 0);
    ck_assert_int_eq(err, 0);
    err = mdb_txn_commit(txn);
    ck_assert_int_eq(err, 0);
    mdb_env_close(env);
}

static void lmdb_put_misaligned_module(
    struct handlebars_module * module,
    char * selected_key,
    size_t selected_key_size
) {
    MDB_env * env;
    MDB_txn * txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    char candidate[32];
    bool found = false;
    int err;

    err = mdb_env_create(&env);
    ck_assert_int_eq(err, 0);
    err = mdb_env_open(env, lmdb_db_file, MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOSUBDIR, 0644);
    ck_assert_int_eq(err, 0);
    err = mdb_txn_begin(env, NULL, 0, &txn);
    ck_assert_int_eq(err, 0);
    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    ck_assert_int_eq(err, 0);

    data.mv_size = module->size;
    data.mv_data = module;
    for( size_t length = 1; length < sizeof(candidate); length++ ) {
        memset(candidate, 'm', length);
        candidate[length] = '\0';
        key.mv_size = length + 1;
        key.mv_data = candidate;
        err = mdb_put(txn, dbi, &key, &data, 0);
        ck_assert_int_eq(err, 0);
    }
    err = mdb_txn_commit(txn);
    ck_assert_int_eq(err, 0);

    err = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    ck_assert_int_eq(err, 0);
    for( size_t length = 1; length < sizeof(candidate); length++ ) {
        memset(candidate, 'm', length);
        candidate[length] = '\0';
        key.mv_size = length + 1;
        key.mv_data = candidate;
        err = mdb_get(txn, dbi, &key, &data);
        ck_assert_int_eq(err, 0);
        if( (uintptr_t) data.mv_data % sizeof(void *) != 0 ) {
            ck_assert_uint_lt(length, selected_key_size);
            memcpy(selected_key, candidate, length + 1);
            found = true;
            break;
        }
    }
    ck_assert_msg(found, "Expected LMDB to expose at least one unaligned value");
    mdb_txn_abort(txn);
    mdb_env_close(env);
}
#endif



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

START_TEST(test_simple_cache_owns_key)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_context * key_context = handlebars_context_ctor_ex(context);
    struct handlebars_string * key = handlebars_string_ctor(key_context, HBS_STRL("temporary key"));
    struct handlebars_module * module = serialize_template("test");

    handlebars_cache_add(cache, key, module);
    handlebars_context_dtor(key_context);

    key = handlebars_string_ctor(context, HBS_STRL("temporary key"));
    ck_assert_ptr_ne(handlebars_cache_find(cache, key), NULL);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_simple_cache_refuses_entry_over_capacity)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key1 = handlebars_string_ctor(context, HBS_STRL("simple-limit-one"));
    struct handlebars_string * key2 = handlebars_string_ctor(context, HBS_STRL("simple-limit-two"));
    struct handlebars_module * module1 = serialize_template("one");
    struct handlebars_module * module2 = serialize_template("two");
    void * module2_parent = talloc_parent(module2);

    cache->max_entries = 1;
    handlebars_cache_add(cache, key1, module1);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);

    handlebars_cache_add(cache, key2, module2);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    ck_assert_ptr_eq(handlebars_cache_find(cache, key1), module1);
    ck_assert_ptr_null(handlebars_cache_find(cache, key2));
    ck_assert_ptr_eq(talloc_parent(module2), module2_parent);

    handlebars_cache_dtor(cache);
    ck_assert_ptr_eq(talloc_parent(module2), module2_parent);
}
END_TEST

START_TEST(test_simple_cache_does_not_evict_executing_module)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * parent_key = handlebars_string_ctor(context, HBS_STRL("simple-active-parent"));
    struct handlebars_string * partial_key = handlebars_string_ctor(context, HBS_STRL("Y"));
    struct handlebars_module * parent = serialize_template("foo={{>bar}}X");
    struct handlebars_module * cached_parent;
    struct handlebars_string * output;
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("Y")));
    do {
        struct handlebars_map * map = handlebars_map_ctor(context, 0);
        map = handlebars_map_str_add(map, HBS_STRL("bar"), partial);
        handlebars_value_map(partials, map);
    } while( 0 );

    cache->max_entries = 1;
    handlebars_cache_add(cache, parent_key, parent);
    handlebars_vm_set_cache(vm, cache);
    handlebars_vm_set_partials(vm, partials);

    cached_parent = handlebars_cache_find(cache, parent_key);
    ck_assert_ptr_eq(cached_parent, parent);
    output = handlebars_vm_execute(vm, cached_parent, input);

    ck_assert_hbs_str_eq_cstr(output, "foo=YX");
    ck_assert_ptr_eq(handlebars_cache_find(cache, parent_key), parent);
    ck_assert_ptr_null(handlebars_cache_find(cache, partial_key));
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_simple_cache_rejects_oversized_module_without_taking_ownership)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("simple-oversized"));
    struct handlebars_module * module = serialize_template("oversized");
    void * parent = talloc_parent(module);

    cache->max_size = module->size - 1;
    handlebars_cache_add(cache, key, module);

    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);
    ck_assert_ptr_null(handlebars_cache_find(cache, key));
    ck_assert_ptr_eq(talloc_parent(module), parent);

    handlebars_cache_dtor(cache);
    ck_assert_ptr_eq(talloc_parent(module), parent);
}
END_TEST

START_TEST(test_simple_cache_duplicate_preserves_module_ownership)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("simple-duplicate"));
    struct handlebars_module * original = serialize_template("original");
    struct handlebars_module * duplicate = serialize_template("duplicate");
    void * duplicate_parent = talloc_parent(duplicate);
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    handlebars_cache_add(cache, key, original);
    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_ptr_eq(talloc_parent(duplicate), duplicate_parent);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
        ck_assert_ptr_eq(handlebars_cache_find(cache, key), original);
        handlebars_cache_dtor(cache);
        ck_assert_ptr_eq(talloc_parent(duplicate), duplicate_parent);
        return;
    }

    handlebars_cache_add(cache, key, duplicate);
    context->e->jmp = prev;
    ck_abort_msg("Expected duplicate simple-cache key to be rejected");
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_simple_cache_add_nomem_preserves_module_ownership)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("simple-add-nomem"));
    struct handlebars_module * module = serialize_template("add nomem");
    void * module_parent = talloc_parent(module);
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_ptr_eq(talloc_parent(module), module_parent);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

        handlebars_cache_add(cache, key, module);
        ck_assert_ptr_eq(handlebars_cache_find(cache, key), module);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    handlebars_cache_add(cache, key, module);
    handlebars_memory_fail_disable();
    context->e->jmp = prev;
    ck_abort_msg("Expected simple-cache allocation to fail");
}
END_TEST

static void assert_simple_cache_consistent(
    struct handlebars_cache * cache,
    struct handlebars_string ** keys,
    size_t key_count
) {
    struct handlebars_cache_stat stat = handlebars_cache_stat(cache);
    size_t current_entries = 0;
    size_t current_size = 0;

    for( size_t i = 0; i < key_count; i++ ) {
        struct handlebars_module * module = handlebars_cache_find(cache, keys[i]);
        if( module ) {
            current_entries++;
            current_size += module->size;
        }
    }

    ck_assert_uint_eq(stat.current_entries, current_entries);
    ck_assert_uint_eq(stat.current_size, current_size);
}

static void execute_simple_cache_gc_nomem_test(int fail_at)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * keys[20];
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    for( size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++ ) {
        char name[32];
        snprintf(name, sizeof(name), "simple-gc-%zu", i);
        keys[i] = handlebars_string_ctor(context, name, strlen(name));
        struct handlebars_module * module = serialize_template(name);
        handlebars_cache_add(cache, keys[i], module);
        module->ts = (time_t) i;
    }
    cache->max_entries = 1;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        assert_simple_cache_consistent(cache, keys, sizeof(keys) / sizeof(keys[0]));
        handlebars_cache_gc(cache);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    (void) handlebars_cache_gc(cache);
    handlebars_memory_fail_disable();
    context->e->jmp = prev;

    assert_simple_cache_consistent(cache, keys, sizeof(keys) / sizeof(keys[0]));
    handlebars_cache_gc(cache);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    handlebars_cache_dtor(cache);
}

START_TEST(test_simple_cache_gc_nomem_keeps_cache_consistent)
{
    for( int fail_at = 1; fail_at <= 3; fail_at++ ) {
        execute_simple_cache_gc_nomem_test(fail_at);
    }
}
END_TEST

START_TEST(test_simple_cache_reset_nomem_preserves_entries)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("simple-reset-nomem"));
    struct handlebars_module * module = serialize_template("reset nomem");
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    handlebars_cache_add(cache, key, module);
    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
        ck_assert_ptr_eq(handlebars_cache_find(cache, key), module);

        handlebars_cache_reset(cache);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    handlebars_cache_reset(cache);
    handlebars_memory_fail_disable();
    context->e->jmp = prev;
    ck_abort_msg("Expected simple-cache reset allocation to fail");
}
END_TEST
#endif

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

START_TEST(test_simple_cache_reset_releases_entries)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("simple-reset-release"));
    struct handlebars_module * module = serialize_template("reset release");

    simple_cache_module_destroyed = false;
    talloc_set_destructor(module, simple_cache_module_dtor);
    handlebars_cache_add(cache, key, module);
    ck_assert(!simple_cache_module_destroyed);

    handlebars_cache_reset(cache);
    ck_assert(simple_cache_module_destroyed);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

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

START_TEST(test_lmdb_cache_reset_removes_entries)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("reset");
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("lmdb-reset"));

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);

    handlebars_cache_reset(cache);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);
    ck_assert_int_eq(handlebars_cache_stat(cache).hits, 0);
    ck_assert_int_eq(handlebars_cache_stat(cache).misses, 0);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_lmdb_cache_does_not_hash_oversized_keys)
{
    struct handlebars_cache * cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    char key_buf[1024];
    memset(key_buf, 'a', sizeof(key_buf));
    struct handlebars_string * key = handlebars_string_ctor(context, key_buf, sizeof(key_buf));
    struct handlebars_module * module = serialize_template("test");

    handlebars_cache_add(cache, key, module);
    ck_assert_ptr_eq(handlebars_cache_find(cache, key), NULL);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_lmdb_cache_rejects_invalid_records)
{
    static const char truncated_key[] = "lmdb-truncated";
    static const char corrupt_key[] = "lmdb-corrupt";
    static const char malformed_key[] = "lmdb-malformed";
    struct handlebars_module * truncated = serialize_template_for_lmdb("truncated");
    struct handlebars_module * corrupt = serialize_template_for_lmdb("corrupt");
    struct handlebars_module * malformed = serialize_template_for_lmdb("malformed");
    struct handlebars_cache * cache;
    struct handlebars_string * key;

    reset_lmdb_test_files();
    corrupt->hash ^= 1;
    handlebars_module_patch_pointers(malformed);
    malformed->programs[0].opcode_count = 0;
    handlebars_module_normalize_pointers(malformed, NULL);
    handlebars_module_generate_hash(malformed);

    lmdb_put_raw(truncated_key, truncated, truncated->size - 1);
    lmdb_put_raw(corrupt_key, corrupt, corrupt->size);
    lmdb_put_raw(malformed_key, malformed, malformed->size);

    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);

    key = handlebars_string_ctor(context, HBS_STRL(truncated_key));
    ck_assert_ptr_null(handlebars_cache_find(cache, key));
    key = handlebars_string_ctor(context, HBS_STRL(corrupt_key));
    ck_assert_ptr_null(handlebars_cache_find(cache, key));
    key = handlebars_string_ctor(context, HBS_STRL(malformed_key));
    ck_assert_ptr_null(handlebars_cache_find(cache, key));
    ck_assert_int_eq(handlebars_cache_stat(cache).misses, 3);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 3);

    ck_assert_int_eq(handlebars_cache_gc(cache), 3);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_lmdb_cache_gc_expires_zero_age_records)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("expired");
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("lmdb-expired"));

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);
    cache->max_age = 0;

    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    ck_assert_int_eq(handlebars_cache_gc(cache), 1);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

    handlebars_cache_dtor(cache);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_lmdb_cache_add_nomem_leaves_cache_usable)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("add nomem");
    struct handlebars_module * found;
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("lmdb-add-nomem"));
    jmp_buf buf;

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

        handlebars_cache_add(cache, key, module);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
        found = handlebars_cache_find(cache, key);
        ck_assert_ptr_nonnull(found);
        handlebars_cache_release(cache, key, found);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    handlebars_cache_add(cache, key, module);
    handlebars_memory_fail_disable();
    ck_abort_msg("Expected LMDB cache add allocation to fail");
}
END_TEST

START_TEST(test_lmdb_cache_find_nomem_closes_transaction)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("find nomem");
    struct handlebars_module * found;
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("lmdb-find-nomem"));
    jmp_buf buf;

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        found = handlebars_cache_find(cache, key);
        ck_assert_ptr_nonnull(found);
        handlebars_cache_release(cache, key, found);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    found = handlebars_cache_find(cache, key);
    (void) found;
    handlebars_memory_fail_disable();
    ck_abort_msg("Expected LMDB cache lookup allocation to fail");
}
END_TEST

START_TEST(test_lmdb_cache_gc_nomem_closes_transaction)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("gc nomem");
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("lmdb-gc-nomem"));
    jmp_buf buf;

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);
    cache->max_age = 0;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
        ck_assert_int_eq(handlebars_cache_gc(cache), 1);
        handlebars_cache_dtor(cache);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_enable();
    (void) handlebars_cache_gc(cache);
    handlebars_memory_fail_disable();
    ck_abort_msg("Expected LMDB cache GC allocation to fail");
}
END_TEST
#endif

START_TEST(test_lmdb_cache_copies_unaligned_records)
{
    struct handlebars_module * module = serialize_template_for_lmdb("unaligned");
    struct handlebars_cache * cache;
    struct handlebars_module * found;
    struct handlebars_string * key;
    char selected_key[32];

    reset_lmdb_test_files();
    lmdb_put_misaligned_module(module, selected_key, sizeof(selected_key));

    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    key = handlebars_string_ctor(context, selected_key, strlen(selected_key));
    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_nonnull(found);
    ck_assert_uint_eq((uintptr_t) found % sizeof(void *), 0);

    handlebars_cache_release(cache, key, found);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_lmdb_cache_round_trips_inline_partial_module)
{
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"p\"}}string{{/inline}}"
        "{{#*inline 123}}scalar{{/inline}}"
        "{{#*inline \"withArgs\" \"ignored\"}}positional{{/inline}}"
        "{{> p}}-{{> 123}}-{{> withArgs}}"
    );
    struct handlebars_module * found;
    struct handlebars_string * key = handlebars_string_ctor(
        context,
        HBS_STRL("lmdb-inline-partial")
    );
    HANDLEBARS_VALUE_DECL(input);

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);

    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_nonnull(found);

    struct handlebars_string * output = handlebars_vm_execute(vm, found, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "string-scalar-positional");

    handlebars_cache_release(cache, key, found);
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(input);
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
    execute_reset_test(cache);
    handlebars_cache_dtor(cache);
}
END_TEST

struct mmap_cache_stress_context {
    struct handlebars_cache * cache;
    struct handlebars_string * key;
    struct handlebars_module * modules[2];
    size_t module_sizes[2];
    uint32_t module_checksums[2];
    pthread_mutex_t start_lock;
    pthread_cond_t start_cond;
    size_t ready;
    bool start;
    size_t invalid_modules;
};

static void mmap_cache_stress_wait(struct mmap_cache_stress_context * ctx)
{
    pthread_mutex_lock(&ctx->start_lock);
    ctx->ready++;
    pthread_cond_broadcast(&ctx->start_cond);
    while( !ctx->start ) {
        pthread_cond_wait(&ctx->start_cond, &ctx->start_lock);
    }
    pthread_mutex_unlock(&ctx->start_lock);
}

static void * mmap_cache_find_stress(void * arg)
{
    struct mmap_cache_stress_context * ctx = arg;

    mmap_cache_stress_wait(ctx);
    for( size_t i = 0; i < 20000; i++ ) {
        struct handlebars_module * module = handlebars_cache_find(ctx->cache, ctx->key);
        if( module ) {
            size_t size = module->size;
            bool valid = module->addr == module;
            if( size == ctx->module_sizes[0] ) {
                valid = valid && adler32((unsigned char *) module, size) == ctx->module_checksums[0];
            } else if( size == ctx->module_sizes[1] ) {
                valid = valid && adler32((unsigned char *) module, size) == ctx->module_checksums[1];
            } else {
                valid = false;
            }
            if( !valid ) {
                ctx->invalid_modules++;
            }
            handlebars_cache_release(ctx->cache, ctx->key, module);
        }
    }
    return NULL;
}

static void * mmap_cache_reset_stress(void * arg)
{
    struct mmap_cache_stress_context * ctx = arg;

    mmap_cache_stress_wait(ctx);
    for( size_t i = 0; i < 2000; i++ ) {
        handlebars_cache_reset(ctx->cache);
        handlebars_cache_add(ctx->cache, ctx->key, ctx->modules[i % 2]);
    }
    return NULL;
}

START_TEST(test_mmap_cache_concurrent_reset_and_find)
{
    struct mmap_cache_stress_context ctx = {0};
    pthread_t find_thread;
    pthread_t reset_thread;
    struct handlebars_module * found;

    ctx.cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    ctx.key = handlebars_string_ctor(context, HBS_STRL("mmap-concurrent"));
    ctx.modules[0] = serialize_template("{{foo}}");
    ctx.modules[1] = serialize_template("{{#if foo}}a longer replacement template{{/if}}");
    ck_assert_int_eq(pthread_mutex_init(&ctx.start_lock, NULL), 0);
    ck_assert_int_eq(pthread_cond_init(&ctx.start_cond, NULL), 0);
    for( size_t i = 0; i < 2; i++ ) {
        handlebars_cache_reset(ctx.cache);
        handlebars_cache_add(ctx.cache, ctx.key, ctx.modules[i]);
        found = handlebars_cache_find(ctx.cache, ctx.key);
        ck_assert_ptr_nonnull(found);
        ctx.module_sizes[i] = found->size;
        ctx.module_checksums[i] = adler32((unsigned char *) found, found->size);
        handlebars_cache_release(ctx.cache, ctx.key, found);
    }
    handlebars_cache_reset(ctx.cache);
    handlebars_cache_add(ctx.cache, ctx.key, ctx.modules[0]);

    ck_assert_int_eq(pthread_create(&find_thread, NULL, mmap_cache_find_stress, &ctx), 0);
    ck_assert_int_eq(pthread_create(&reset_thread, NULL, mmap_cache_reset_stress, &ctx), 0);

    ck_assert_int_eq(pthread_mutex_lock(&ctx.start_lock), 0);
    while( ctx.ready < 2 ) {
        ck_assert_int_eq(pthread_cond_wait(&ctx.start_cond, &ctx.start_lock), 0);
    }
    ctx.start = true;
    ck_assert_int_eq(pthread_cond_broadcast(&ctx.start_cond), 0);
    ck_assert_int_eq(pthread_mutex_unlock(&ctx.start_lock), 0);

    ck_assert_int_eq(pthread_join(find_thread, NULL), 0);
    ck_assert_int_eq(pthread_join(reset_thread, NULL), 0);
    ck_assert_uint_eq(ctx.invalid_modules, 0);
    ck_assert_int_eq(handlebars_cache_stat(ctx.cache).refcount, 0);

    handlebars_cache_reset(ctx.cache);
    handlebars_cache_add(ctx.cache, ctx.key, ctx.modules[0]);
    found = handlebars_cache_find(ctx.cache, ctx.key);
    ck_assert_ptr_nonnull(found);
    ck_assert_uint_eq(
        adler32((unsigned char *) found, found->size),
        ctx.module_checksums[0]
    );
    handlebars_cache_release(ctx.cache, ctx.key, found);

    ck_assert_int_eq(pthread_cond_destroy(&ctx.start_cond), 0);
    ck_assert_int_eq(pthread_mutex_destroy(&ctx.start_lock), 0);
    handlebars_cache_dtor(ctx.cache);
}
END_TEST

START_TEST(test_mmap_cache_expired_find_is_a_miss)
{
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    struct handlebars_string * key = handlebars_string_ctor(context, HBS_STRL("{{foo}}"));
    struct handlebars_module * module = serialize_template("{{foo}}");
    handlebars_cache_add(cache, key, module);
    cache->max_age = 0;

    ck_assert_ptr_eq(handlebars_cache_find(cache, key), NULL);
    ck_assert_int_eq(handlebars_cache_stat(cache).refcount, 0);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_mmap_cache_hash_collision_is_a_miss)
{
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    struct handlebars_string * key1 = handlebars_string_ctor(context, HBS_STRL("lwaaaa"));
    struct handlebars_string * key2 = handlebars_string_ctor(context, HBS_STRL("tnsaaa"));
    struct handlebars_module * module1 = serialize_template("one");
    struct handlebars_module * module2 = serialize_template("two");
    handlebars_cache_add(cache, key1, module1);
    handlebars_cache_add(cache, key2, module2);

    struct handlebars_module * found = handlebars_cache_find(cache, key1);
    ck_assert_ptr_ne(found, NULL);
    handlebars_cache_release(cache, key1, found);
    ck_assert_ptr_eq(handlebars_cache_find(cache, key2), NULL);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_mmap_cache_rejects_invalid_geometry)
{
    jmp_buf buf;
    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        return;
    }

    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 4096, 0);
    (void) cache;
    ck_abort_msg("Expected zero-entry mmap cache to be rejected");
}
END_TEST
#endif

START_TEST(test_compat_partial_cache_uses_processed_template_key)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template("{{>foo}}");
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_init_json_string(context, input, "{\"bar\": \"baz\"}");
    handlebars_value_convert(input);
    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{bar}}")));
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(partial_map, HBS_STRL("foo"), partial);
    handlebars_value_map(partials, partial_map);

    handlebars_vm_set_flags(vm, handlebars_compiler_flag_compat);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    struct handlebars_string * buffer = handlebars_vm_execute(vm, module, input);
    ck_assert_hbs_str_eq_cstr(buffer, "baz");
    buffer = handlebars_vm_execute(vm, module, input);
    ck_assert_hbs_str_eq_cstr(buffer, "baz");
    ck_assert_int_ge(handlebars_cache_stat(cache).hits, 1);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_vm_rejects_empty_opcode_range)
{
    struct handlebars_module * module = serialize_template("test");
    module->programs[0].opcode_count = 0;
    HANDLEBARS_VALUE_DECL(input);
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_str_eq(handlebars_error_msg(context), "Invalid opcode range for program: 0");
        HANDLEBARS_VALUE_UNDECL(input);
        return;
    }

    (void) handlebars_vm_execute(vm, module, input);
    ck_abort_msg("Expected malformed opcode range to be rejected");
}
END_TEST

START_TEST(test_vm_error_returns_null_without_outer_handler)
{
    struct handlebars_module * module = serialize_template("test");
    module->programs[0].opcode_count = 0;
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_eq(output, NULL);
    ck_assert_str_eq(handlebars_error_msg(context), "Invalid opcode range for program: 0");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_rejects_empty_lookup_path)
{
    struct handlebars_module * module = serialize_template("{{foo}}");
    bool changed = false;
    for( size_t i = 0; i < module->opcode_count; i++ ) {
        if( module->opcodes[i].type == handlebars_opcode_type_lookup_on_context ) {
            module->opcodes[i].op1.data.array.count = 0;
            changed = true;
            break;
        }
    }
    ck_assert(changed);

    HANDLEBARS_VALUE_DECL(input);
    jmp_buf buf;
    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_str_eq(handlebars_error_msg(context), "Invalid lookup_on_context operands");
        HANDLEBARS_VALUE_UNDECL(input);
        return;
    }

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    (void) output;
    ck_abort_msg("Expected empty lookup path to be rejected");
}
END_TEST

START_TEST(test_vm_rejects_invalid_partial_block_program)
{
    struct handlebars_module * module = serialize_template(
        "{{#> missing}}body{{/missing}}"
    );
    bool changed = false;

    for( size_t i = 0; i < module->opcode_count; i++ ) {
        if( module->opcodes[i].type == handlebars_opcode_type_push_program
                && module->opcodes[i].op1.type == handlebars_operand_type_long ) {
            module->opcodes[i].op1.data.longval = (long) module->program_count;
            changed = true;
            break;
        }
    }
    ck_assert(changed);

    HANDLEBARS_VALUE_DECL(input);
    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_null(output);
    ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(HBSCTX(vm)), "Invalid program"));

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_hash_rehash)
{
    struct handlebars_module * module = serialize_template(
        "{{#if true a=1 b=2 c=3 d=4 e=5}}yes{{/if}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_ne(output, NULL);
    ck_assert_hbs_str_eq_cstr(output, "yes");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_partial_block_preserves_lexical_block_params)
{
    struct handlebars_module * module = serialize_template(
        "{{#with value as |x|}}{{#> p}}{{x}}{{/p}}{{/with}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_init_json_string(context, input, "{\"value\": \"ok\"}");
    handlebars_value_convert(input);
    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("{{#if true}}{{> @partial-block}}{{/if}}"))
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(partial_map, HBS_STRL("p"), partial);
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_ne(output, NULL);
    ck_assert_hbs_str_eq_cstr(output, "ok");

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_partial_block_preserves_all_lexical_block_params)
{
    struct handlebars_module * module = serialize_template(
        "{{#with outer as |a|}}{{#with inner as |b|}}"
        "{{#> p}}{{a.name}}-{{b}}{{/p}}"
        "{{/with}}{{/with}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_init_json_string(
        context,
        input,
        "{\"outer\": {\"name\": \"A\", \"inner\": \"B\"}}"
    );
    handlebars_value_convert(input);
    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("{{#if true}}{{> @partial-block}}{{/if}}"))
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(partial_map, HBS_STRL("p"), partial);
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_ptr_ne(output, NULL);
    ck_assert_hbs_str_eq_cstr(output, "A-B");

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_definition)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"myPartial\"}}success{{/inline}}{{> myPartial}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_scalar_name)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline 123}}integer{{/inline}}"
        "{{#*inline 0}}zero{{/inline}}"
        "{{#*inline true}}boolean{{/inline}}"
        "{{#*inline false}}false{{/inline}}"
        "{{#*inline 1.5}}number{{/inline}}"
        "{{#*inline null}}null{{/inline}}"
        "{{#*inline undefined}}undefined{{/inline}}"
        "{{> 123}}-{{> 0}}-{{> true}}-{{> false}}-{{> 1.5}}-"
        "{{> null}}-{{> undefined}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(
        output,
        "integer-zero-boolean-false-number-null-undefined"
    );

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_partial_block_installs_inline_partials)
{
    struct handlebars_module * module = serialize_template(
        "{{#> dude}}{{#*inline \"myPartial\"}}success{{/inline}}{{/dude}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("{{> myPartial}}"))
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(partial_map, HBS_STRL("dude"), partial);
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_partial_block_inline_partial_preserves_caller_depths)
{
    struct handlebars_module * module = serialize_template(
        "{{#> layout child}}"
        "{{#*inline \"p\"}}{{../x}}:{{x}}{{/inline}}"
        "{{/layout}}"
    );
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_init_json_string(
        context,
        input,
        "{\"x\":\"root\",\"child\":{\"x\":\"child\"}}"
    );
    handlebars_value_convert(input);
    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("{{> p}}"))
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(partial_map, HBS_STRL("layout"), partial);
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "root:child");

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_with_alternate_decorators)
{
    struct handlebars_module * module = serialize_template_with_flags(
        "{{#*inline \"myPartial\"}}success{{/inline}}{{> myPartial}}",
        handlebars_compiler_flag_alternate_decorators
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_statement_with_alternate_decorators)
{
    struct handlebars_module * module = serialize_template_with_flags(
        "{{*inline \"p\"}}ok",
        handlebars_compiler_flag_alternate_decorators
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "ok");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_with_hash_arguments)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"myPartial\" unused=1}}success{{/inline}}{{> myPartial}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_with_positional_arguments)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"myPartial\" \"extra\" value}}success{{/inline}}"
        "{{> myPartial}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_with_nested_hash_arguments)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"myPartial\" unused=(helper nested=1)}}"
        "success"
        "{{/inline}}{{> myPartial}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "success");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_many_inline_partial_definitions_share_scope)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_module * module;
    HANDLEBARS_VALUE_DECL(input);

    for( unsigned int i = 0; i < 128; i++ ) {
        tmpl = handlebars_string_asprintf_append(
            context,
            tmpl,
            "{{#*inline \"partial%u\"}}%u{{/inline}}",
            i,
            i
        );
    }
    tmpl = handlebars_string_append(context, tmpl, HBS_STRL("{{> partial127}}"));
    module = serialize_template(hbs_str_val(tmpl));

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "127");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_parent_depth_uses_invocation_context)
{
    struct handlebars_module * module = serialize_template(
        "{{#*inline \"p\"}}{{../x}}{{/inline}}"
        "{{#with child}}{{> p}}{{/with}}"
    );
    HANDLEBARS_VALUE_DECL(input);

    handlebars_value_init_json_string(
        context,
        input,
        "{\"x\":\"root\",\"child\":{\"y\":true}}"
    );
    handlebars_value_convert(input);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "");

    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_inline_partial_reuses_full_captured_block_param_stack)
{
    struct handlebars_module * module = serialize_template("{{> recurse start}}");
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(node);
    HANDLEBARS_VALUE_DECL(recurse_partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_map(node, handlebars_map_ctor(context, 0));
    for( unsigned int i = 0; i < 47; i++ ) {
        struct handlebars_map * parent = handlebars_map_ctor(context, 1);
        parent = handlebars_map_str_add(parent, HBS_STRL("next"), node);
        handlebars_value_map(node, parent);
    }
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("start"), node);
    handlebars_value_map(input, input_map);

    handlebars_value_str(
        recurse_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL(
                "{{#if next}}"
                "{{#with next as |x|}}{{> recurse}}{{/with}}"
                "{{else}}"
                "{{#if true}}{{#if true}}{{#if true}}"
                "{{#*inline \"p\"}}ok{{/inline}}{{> p}}{{> p}}"
                "{{/if}}{{/if}}{{/if}}"
                "{{/if}}"
            )
        )
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 1);
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("recurse"),
        recurse_partial
    );
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_msg(
        hbs_str_eq_strl(output, HBS_STRL("okok")),
        "expected okok, got '%s' (error %d: %s)",
        hbs_str_val(output),
        handlebars_error_num(HBSCTX(vm)),
        handlebars_error_msg(HBSCTX(vm))
    );
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(recurse_partial);
    HANDLEBARS_VALUE_UNDECL(node);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_partial_block_grows_captured_block_param_stack)
{
    struct handlebars_module * module = serialize_template("{{> recurse start}}");
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(node);
    HANDLEBARS_VALUE_DECL(recurse_partial);
    HANDLEBARS_VALUE_DECL(layout_partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_map(node, handlebars_map_ctor(context, 0));
    for( unsigned int i = 0; i < 47; i++ ) {
        struct handlebars_map * parent = handlebars_map_ctor(context, 1);
        parent = handlebars_map_str_add(parent, HBS_STRL("next"), node);
        handlebars_value_map(node, parent);
    }
    struct handlebars_map * input_map = handlebars_map_ctor(context, 1);
    input_map = handlebars_map_str_add(input_map, HBS_STRL("start"), node);
    handlebars_value_map(input, input_map);

    handlebars_value_str(
        recurse_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL(
                "{{#if next}}"
                "{{#with next as |x|}}{{> recurse}}{{/with}}"
                "{{else}}"
                "{{#if true}}{{#if true}}{{#if true}}"
                "{{#> layout}}ok{{/layout}}"
                "{{/if}}{{/if}}{{/if}}"
                "{{/if}}"
            )
        )
    );
    handlebars_value_str(
        layout_partial,
        handlebars_string_ctor(
            context,
            HBS_STRL("{{> @partial-block}}{{> @partial-block}}")
        )
    );
    struct handlebars_map * partial_map = handlebars_map_ctor(context, 2);
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("recurse"),
        recurse_partial
    );
    partial_map = handlebars_map_str_add(
        partial_map,
        HBS_STRL("layout"),
        layout_partial
    );
    handlebars_value_map(partials, partial_map);
    handlebars_vm_set_partials(vm, partials);

    struct handlebars_string * output = handlebars_vm_execute(vm, module, input);
    ck_assert_msg(output != NULL, "%s", handlebars_error_msg(HBSCTX(vm)));
    ck_assert_hbs_str_eq_cstr(output, "okok");

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(layout_partial);
    HANDLEBARS_VALUE_UNDECL(recurse_partial);
    HANDLEBARS_VALUE_UNDECL(node);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    const char * title = "Handlebars Spec";
    Suite * s = suite_create(title);

    REGISTER_TEST_FIXTURE(s, test_cache_gc_entries, "Garbage Collection");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_owns_key, "Simple cache owns key");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_refuses_entry_over_capacity, "Simple cache refuses entries over capacity");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_does_not_evict_executing_module, "Simple cache keeps executing modules alive");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_rejects_oversized_module_without_taking_ownership, "Simple cache rejects oversized modules");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_duplicate_preserves_module_ownership, "Simple cache duplicate preserves ownership");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_simple_cache_add_nomem_preserves_module_ownership, "Simple cache add preserves ownership after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc_nomem_keeps_cache_consistent, "Simple cache GC remains consistent after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset_nomem_preserves_entries, "Simple cache reset preserves entries after allocation failure");
#endif
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc, "Simple Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset, "Simple Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset_releases_entries, "Simple cache reset releases entries");
#ifdef HANDLEBARS_HAVE_LMDB
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc, "LMDB Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset, "LMDB Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset_removes_entries, "LMDB reset removes entries");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_does_not_hash_oversized_keys, "LMDB skips oversized keys");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_rejects_invalid_records, "LMDB rejects invalid records");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc_expires_zero_age_records, "LMDB GC expires zero-age records");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_add_nomem_leaves_cache_usable, "LMDB add remains usable after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_find_nomem_closes_transaction, "LMDB find cleans up after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc_nomem_closes_transaction, "LMDB GC cleans up after allocation failure");
#endif
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_copies_unaligned_records, "LMDB copies unaligned records");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_round_trips_inline_partial_module, "LMDB round-trips inline-partial modules");
#endif
#ifdef HANDLEBARS_HAVE_PTHREAD
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_gc, "MMAP Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_reset, "MMAP Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_concurrent_reset_and_find, "MMAP concurrent reset and find");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_expired_find_is_a_miss, "MMAP expired find is a miss");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_hash_collision_is_a_miss, "MMAP hash collision is a miss");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_rejects_invalid_geometry, "MMAP rejects invalid geometry");
#endif
    REGISTER_TEST_FIXTURE(s, test_compat_partial_cache_uses_processed_template_key, "Compat partial cache key");
    REGISTER_TEST_FIXTURE(s, test_vm_rejects_empty_opcode_range, "VM rejects empty opcode range");
    REGISTER_TEST_FIXTURE(s, test_vm_error_returns_null_without_outer_handler, "VM error returns NULL without outer handler");
    REGISTER_TEST_FIXTURE(s, test_vm_rejects_empty_lookup_path, "VM rejects empty lookup path");
    REGISTER_TEST_FIXTURE(s, test_vm_rejects_invalid_partial_block_program, "VM rejects invalid partial block programs");
    REGISTER_TEST_FIXTURE(s, test_vm_hash_rehash, "VM rehashes helper hashes safely");
    REGISTER_TEST_FIXTURE(s, test_partial_block_preserves_lexical_block_params, "Partial blocks preserve lexical block parameters");
    REGISTER_TEST_FIXTURE(s, test_partial_block_preserves_all_lexical_block_params, "Partial blocks preserve all lexical block parameters");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_definition, "Inline partial definitions");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_scalar_name, "Inline partial scalar names");
    REGISTER_TEST_FIXTURE(s, test_partial_block_installs_inline_partials, "Partial blocks install inline partials");
    REGISTER_TEST_FIXTURE(s, test_partial_block_inline_partial_preserves_caller_depths, "Partial-block inline partials preserve caller depths");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_with_alternate_decorators, "Inline partials with alternate decorators");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_statement_with_alternate_decorators, "Inline partial statements with alternate decorators");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_with_hash_arguments, "Inline partials with hash arguments");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_with_positional_arguments, "Inline partials with positional arguments");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_with_nested_hash_arguments, "Inline partials with nested hash arguments");
    REGISTER_TEST_FIXTURE(s, test_many_inline_partial_definitions_share_scope, "Inline partial definitions share one lexical scope");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_parent_depth_uses_invocation_context, "Inline partial parent depths use the invocation context");
    REGISTER_TEST_FIXTURE(s, test_inline_partial_reuses_full_captured_block_param_stack, "Inline partials reuse a full captured block parameter stack");
    REGISTER_TEST_FIXTURE(s, test_partial_block_grows_captured_block_param_stack, "Partial blocks grow captured block parameter stacks safely");

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

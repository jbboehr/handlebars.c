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
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <talloc.h>
#include <time.h>

#ifdef HANDLEBARS_HAVE_PTHREAD
#include <pthread.h>
#ifdef HANDLEBARS_TESTING_EXPORTS
#include <sys/mman.h>
#endif
#endif

#ifndef YY_NO_UNISTD_H
#include <sys/wait.h>
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
#include "handlebars_private.h"
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

static struct handlebars_value * clear_vm_cache_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 0);
    handlebars_vm_set_cache(callback_vm, NULL);
    handlebars_value_null(rv);
    return rv;
}

static struct handlebars_value * set_vm_error_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 0);
    handlebars_error_set(
        HBSCTX(callback_vm),
        HANDLEBARS_ERROR,
        "Intentional non-throwing helper failure"
    );
    handlebars_value_null(rv);
    return rv;
}

static struct handlebars_value * stale_error_lambda_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 1);
    handlebars_value_str(
        rv,
        handlebars_string_ctor(HBSCTX(callback_vm), HBS_STRL("lambda ok"))
    );
    return rv;
}

static handlebars_cache_release_func active_cache_original_release;
static unsigned int active_cache_release_count;

static void track_active_cache_release(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
)
{
    active_cache_release_count++;
    active_cache_original_release(cache, tmpl, module);
}

#ifdef HANDLEBARS_HAVE_PTHREAD
static struct handlebars_cache * cache_transition_old;
static struct handlebars_cache * cache_transition_new;
static size_t cache_transition_observed_old_refcount;
static size_t cache_transition_observed_new_refcount;

static struct handlebars_value * replace_vm_cache_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    ck_assert_int_eq(argc, 0);
    handlebars_vm_set_cache(callback_vm, cache_transition_new);
    handlebars_value_null(rv);
    return rv;
}

static struct handlebars_value * observe_transition_caches_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    (void) callback_vm;
    ck_assert_int_eq(argc, 0);
    cache_transition_observed_old_refcount =
        handlebars_cache_stat(cache_transition_old).refcount;
    cache_transition_observed_new_refcount =
        handlebars_cache_stat(cache_transition_new).refcount;
    handlebars_value_null(rv);
    return rv;
}

static struct handlebars_value * replace_vm_cache_and_throw_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    (void) argv;
    (void) options;
    (void) rv;
    ck_assert_int_eq(argc, 0);
    handlebars_vm_set_cache(callback_vm, cache_transition_new);
    handlebars_throw(
        HBSCTX(callback_vm),
        HANDLEBARS_ERROR,
        "Intentional cache transition failure"
    );
}
#endif

static unsigned int borrowed_cache_destroy_count;

static int borrowed_cache_dtor(struct handlebars_cache * cache)
{
    (void) cache;
    borrowed_cache_destroy_count++;
    return 0;
}

enum vm_cache_fault_operation {
    vm_cache_fault_op_none,
    vm_cache_fault_op_find,
    vm_cache_fault_op_add,
    vm_cache_fault_op_release
};

static const struct handlebars_cache_handlers * vm_cache_fault_original_handlers;
static enum vm_cache_fault_operation vm_cache_fault_current;

static const char * vm_cache_fault_name(enum vm_cache_fault_operation operation)
{
    switch( operation ) {
        case vm_cache_fault_op_find:
            return "find";
        case vm_cache_fault_op_add:
            return "add";
        case vm_cache_fault_op_release:
            return "release";
        case vm_cache_fault_op_none:
        default:
            return "none";
    }
}

static void vm_cache_fault_throw(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    enum vm_cache_fault_operation operation
)
{
    struct handlebars_locinfo loc = {
        .first_line = 100 + operation,
        .first_column = (int) hbs_str_len(tmpl),
        .last_line = 200 + operation,
        .last_column = (int) hbs_str_len(tmpl) + 1
    };

    handlebars_throw_ex(
        HBSCTX(cache),
        HANDLEBARS_ERROR,
        &loc,
        "Injected cache %s failure: %.*s",
        vm_cache_fault_name(operation),
        (int) hbs_str_len(tmpl),
        hbs_str_val(tmpl)
    );
}

static struct handlebars_module * vm_cache_fault_find(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl
)
{
    if( vm_cache_fault_current == vm_cache_fault_op_find ) {
        vm_cache_fault_throw(cache, tmpl, vm_cache_fault_op_find);
    }
    return vm_cache_fault_original_handlers->find(cache, tmpl);
}

static void vm_cache_fault_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
)
{
    if( vm_cache_fault_current == vm_cache_fault_op_add ) {
        vm_cache_fault_throw(cache, tmpl, vm_cache_fault_op_add);
    }
    vm_cache_fault_original_handlers->add(cache, tmpl, module);
}

static void vm_cache_fault_release(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
)
{
    if( vm_cache_fault_current == vm_cache_fault_op_release ) {
        vm_cache_fault_throw(cache, tmpl, vm_cache_fault_op_release);
    }
    vm_cache_fault_original_handlers->release(cache, tmpl, module);
}

static struct handlebars_value * vm_cache_primary_error_helper(
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * callback_vm,
    struct handlebars_value * rv
)
{
    struct handlebars_locinfo loc = {
        .first_line = 701,
        .first_column = 702,
        .last_line = 703,
        .last_column = 704
    };

    (void) argv;
    (void) options;
    (void) rv;
    ck_assert_int_eq(argc, 0);
    handlebars_throw_ex(
        HBSCTX(callback_vm),
        HANDLEBARS_ERROR,
        &loc,
        "Primary VM failure"
    );
}

static void vm_cache_set_string_partial(
    struct handlebars_context * owner,
    struct handlebars_vm * target_vm,
    const char * source
)
{
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(owner, source, strlen(source))
    );
    map = handlebars_map_ctor(owner, 1);
    map = handlebars_map_str_add(map, HBS_STRL("cached"), partial);
    handlebars_value_map(partials, map);
    handlebars_vm_set_partials(target_vm, partials);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
}

static void vm_cache_assert_operation_failure(
    struct handlebars_vm * target_vm,
    struct handlebars_module * module,
    struct handlebars_value * input,
    const char * source,
    enum vm_cache_fault_operation operation
)
{
    struct handlebars_string * output = (void *) 1;
    struct handlebars_locinfo loc;
    enum handlebars_error_type error;
    char expected[160];

    vm_cache_set_string_partial(HBSCTX(target_vm), target_vm, source);
    vm_cache_fault_current = operation;
    error = handlebars_vm_execute_try(target_vm, module, input, &output);

    snprintf(
        expected,
        sizeof(expected),
        "Injected cache %s failure: %s",
        vm_cache_fault_name(operation),
        source
    );
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_str_eq(handlebars_error_msg(HBSCTX(target_vm)), expected);
    loc = handlebars_error_loc(HBSCTX(target_vm));
    ck_assert_int_eq(loc.first_line, 100 + operation);
    ck_assert_int_eq(loc.first_column, (int) strlen(source));
    ck_assert_int_eq(loc.last_line, 200 + operation);
    ck_assert_int_eq(loc.last_column, (int) strlen(source) + 1);
}

static bool cache_test_module_destroyed;

static int cache_test_module_dtor(struct handlebars_module * module)
{
    (void) module;
    cache_test_module_destroyed = true;
    return 0;
}

#ifdef HANDLEBARS_HAVE_PTHREAD
struct cache_try_concurrency_state {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    handlebars_cache_find_func original_find;
    struct handlebars_string * first_key;
    bool first_entered;
    bool allow_first_complete;
    bool probe_completed;
    bool probe_acquired;
    enum handlebars_error_type probe_error;
    enum handlebars_error_type nested_error;
};

struct cache_try_concurrency_arg {
    struct handlebars_cache * cache;
    struct handlebars_string * key;
    bool probe_guard;
    enum handlebars_error_type error;
};

static struct cache_try_concurrency_state * cache_try_concurrency_current_state;

static struct handlebars_module * cache_try_concurrency_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
) {
    struct cache_try_concurrency_state * state = cache_try_concurrency_current_state;
    struct handlebars_cache_stat stat;

    ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
    if( key == state->first_key ) {
        ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);
        state->nested_error = handlebars_cache_stat_try(cache, &stat);
        ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
        state->first_entered = true;
        ck_assert_int_eq(pthread_cond_broadcast(&state->cond), 0);
        while( !state->allow_first_complete ) {
            ck_assert_int_eq(pthread_cond_wait(&state->cond, &state->lock), 0);
        }
    }
    ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);
    return state->original_find(cache, key);
}

static void * cache_try_concurrency_thread(void * opaque)
{
    struct cache_try_concurrency_arg * arg = opaque;
    struct cache_try_concurrency_state * state = cache_try_concurrency_current_state;
    struct handlebars_module * found = (void *) 1;

    if( arg->probe_guard ) {
        struct handlebars_cache_try_guard guard;
        bool acquired;

        state->probe_error = handlebars_cache_try_guard_try_begin(
            HBSCTX(arg->cache)->e,
            &guard,
            &acquired
        );
        state->probe_acquired = acquired;
        if( acquired ) {
            enum handlebars_error_type end_error = handlebars_cache_try_guard_end(
                &guard
            );
            if( state->probe_error == HANDLEBARS_SUCCESS ) {
                state->probe_error = end_error;
            }
        }
        ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
        state->probe_completed = true;
        ck_assert_int_eq(pthread_cond_broadcast(&state->cond), 0);
        ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);
    }
    arg->error = handlebars_cache_find_try(arg->cache, arg->key, &found);
    ck_assert_ptr_null(found);
    return NULL;
}

START_TEST(test_cache_try_concurrent_calls_restore_jump_target)
{
    struct cache_try_concurrency_state state = {0};
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(context, 2097152, 2053);
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers ordered_handlers = *original_handlers;
    struct cache_try_concurrency_arg first = {0};
    struct cache_try_concurrency_arg second = {0};
    struct handlebars_cache_stat stat;
    pthread_t first_thread;
    pthread_t second_thread;
    jmp_buf * previous = context->e->jmp;
    jmp_buf * observed;
    jmp_buf outer;

    state.original_find = original_handlers->find;
    state.first_key = handlebars_string_ctor(context, HBS_STRL("cache-try-first"));
    ordered_handlers.find = &cache_try_concurrency_find;
    cache->hnd = &ordered_handlers;
    cache_try_concurrency_current_state = &state;
    first.cache = cache;
    first.key = state.first_key;
    second.cache = cache;
    second.key = handlebars_string_ctor(context, HBS_STRL("cache-try-second"));
    second.probe_guard = true;

    ck_assert_int_eq(pthread_mutex_init(&state.lock, NULL), 0);
    ck_assert_int_eq(pthread_cond_init(&state.cond, NULL), 0);

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A concurrent cache try API escaped through longjmp");
    }
    context->e->jmp = &outer;

    ck_assert_int_eq(pthread_create(&first_thread, NULL, cache_try_concurrency_thread, &first), 0);
    ck_assert_int_eq(pthread_mutex_lock(&state.lock), 0);
    while( !state.first_entered ) {
        ck_assert_int_eq(pthread_cond_wait(&state.cond, &state.lock), 0);
    }
    ck_assert_int_eq(pthread_mutex_unlock(&state.lock), 0);

    ck_assert_int_eq(pthread_create(&second_thread, NULL, cache_try_concurrency_thread, &second), 0);
    ck_assert_int_eq(pthread_mutex_lock(&state.lock), 0);
    while( !state.probe_completed ) {
        ck_assert_int_eq(pthread_cond_wait(&state.cond, &state.lock), 0);
    }
    state.allow_first_complete = true;
    ck_assert_int_eq(pthread_cond_broadcast(&state.cond), 0);
    ck_assert_int_eq(pthread_mutex_unlock(&state.lock), 0);
    ck_assert_int_eq(pthread_join(first_thread, NULL), 0);
    ck_assert_int_eq(pthread_join(second_thread, NULL), 0);

    observed = context->e->jmp;
    context->e->jmp = previous;
    cache->hnd = original_handlers;
    cache_try_concurrency_current_state = NULL;
    stat = handlebars_cache_stat(cache);

    ck_assert_int_eq(pthread_cond_destroy(&state.cond), 0);
    ck_assert_int_eq(pthread_mutex_destroy(&state.lock), 0);
    handlebars_cache_dtor(cache);

    ck_assert_int_eq(first.error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(second.error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(state.nested_error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(state.probe_error, HANDLEBARS_SUCCESS);
    ck_assert(!state.probe_acquired);
    ck_assert_uint_eq(stat.misses, 2);
    ck_assert_int_eq(stat.refcount, 0);
    ck_assert_ptr_eq(observed, &outer);
}
END_TEST

struct cache_try_independent_context_state {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    struct handlebars_cache * cache;
    struct handlebars_string * key;
    bool start;
    bool completed;
    bool timed_out;
    enum handlebars_error_type error;
};

static struct cache_try_independent_context_state * cache_try_independent_context_current_state;

static void * cache_try_independent_context_thread(void * opaque)
{
    struct cache_try_independent_context_state * state = opaque;
    struct handlebars_module * found = (void *) 1;

    ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
    while( !state->start ) {
        ck_assert_int_eq(pthread_cond_wait(&state->cond, &state->lock), 0);
    }
    ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);

    state->error = handlebars_cache_find_try(state->cache, state->key, &found);
    ck_assert_ptr_null(found);

    ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
    state->completed = true;
    ck_assert_int_eq(pthread_cond_broadcast(&state->cond), 0);
    ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);
    return NULL;
}

static int cache_try_independent_context_module_dtor(
    struct handlebars_module * module
)
{
    struct cache_try_independent_context_state * state = cache_try_independent_context_current_state;
    struct timespec deadline;
    int rc = 0;

    (void) module;
    ck_assert_int_eq(clock_gettime(CLOCK_REALTIME, &deadline), 0);
    deadline.tv_sec += 2;

    ck_assert_int_eq(pthread_mutex_lock(&state->lock), 0);
    state->start = true;
    ck_assert_int_eq(pthread_cond_broadcast(&state->cond), 0);
    while( !state->completed && rc != ETIMEDOUT ) {
        rc = pthread_cond_timedwait(&state->cond, &state->lock, &deadline);
        ck_assert(rc == 0 || rc == ETIMEDOUT);
    }
    state->timed_out = !state->completed;
    ck_assert_int_eq(pthread_mutex_unlock(&state->lock), 0);
    return 0;
}

START_TEST(test_cache_try_independent_contexts_progress_during_reset_destructor)
{
    struct cache_try_independent_context_state state = {0};
    struct handlebars_context * other_context = handlebars_context_ctor_ex(root);
    struct handlebars_cache * reset_cache = handlebars_cache_simple_ctor(context);
    struct handlebars_cache * find_cache = handlebars_cache_simple_ctor(other_context);
    struct handlebars_string * reset_key = handlebars_string_ctor(
        context,
        HBS_STRL("cache-try-reset")
    );
    struct handlebars_module * module = serialize_template("cache try reset");
    pthread_t worker;
    enum handlebars_error_type error;

    state.cache = find_cache;
    state.key = handlebars_string_ctor(
        other_context,
        HBS_STRL("cache-try-independent")
    );
    cache_try_independent_context_current_state = &state;
    ck_assert_int_eq(pthread_mutex_init(&state.lock, NULL), 0);
    ck_assert_int_eq(pthread_cond_init(&state.cond, NULL), 0);
    ck_assert_int_eq(pthread_create(
        &worker,
        NULL,
        cache_try_independent_context_thread,
        &state
    ), 0);

    talloc_set_destructor(module, cache_try_independent_context_module_dtor);
    handlebars_cache_add(reset_cache, reset_key, module);
    error = handlebars_cache_reset_try(reset_cache);

    ck_assert_int_eq(pthread_join(worker, NULL), 0);
    cache_try_independent_context_current_state = NULL;
    ck_assert_int_eq(pthread_cond_destroy(&state.cond), 0);
    ck_assert_int_eq(pthread_mutex_destroy(&state.lock), 0);

    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(state.error, HANDLEBARS_SUCCESS);
    ck_assert(state.completed);
    ck_assert(!state.timed_out);

    handlebars_cache_dtor(reset_cache);
    handlebars_cache_dtor(find_cache);
    handlebars_context_dtor(other_context);
}
END_TEST
#endif

START_TEST(test_simple_cache_try_api)
{
    struct handlebars_cache * cache = (void *) 1;
    struct handlebars_string * key;
    struct handlebars_module * module;
    struct handlebars_module * duplicate;
    struct handlebars_module * found = (void *) 1;
    struct handlebars_cache_stat stat = { .name = "unchanged" };
    enum handlebars_error_type error;
    int removed = -1;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("A cache try API escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_cache_simple_ctor_try(context, &cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    key = handlebars_string_ctor(context, HBS_STRL("simple-try"));
    module = serialize_template("simple try");
    duplicate = serialize_template("duplicate");

    error = handlebars_cache_add_try(cache, key, module);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_eq(found, module);

    error = handlebars_cache_release_try(cache, key, found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    error = handlebars_cache_stat_try(cache, &stat);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_str_eq(stat.name, "simple");
    ck_assert_uint_eq(stat.current_entries, 1);

    error = handlebars_cache_add_try(cache, key, duplicate);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Duplicate cache key"));
    ck_assert_ptr_eq(context->e->jmp, &outer);

    found = (void *) 1;
    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_eq(found, module);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(handlebars_error_msg(context));

    cache->max_age = 0;
    error = handlebars_cache_gc_try(cache, &removed);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(removed, 1);

    found = (void *) 1;
    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(found);

    error = handlebars_cache_reset_try(cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    context->e->jmp = previous;
    handlebars_cache_dtor(cache);
}
END_TEST

#ifdef HANDLEBARS_HAVE_LMDB
START_TEST(test_lmdb_cache_try_constructor_reports_errors)
{
    struct handlebars_cache * cache = (void *) 1;
    enum handlebars_error_type error;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("LMDB cache try constructor escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_cache_lmdb_ctor_try(
        context,
        "./handlebars-lmdb-cache-missing/cache.mdb",
        &cache
    );

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(handlebars_error_msg(context));

    context->e->jmp = previous;
}
END_TEST
#endif

#ifdef HANDLEBARS_HAVE_PTHREAD
START_TEST(test_mmap_cache_try_constructor_reports_errors)
{
    struct handlebars_cache * cache = (void *) 1;
    enum handlebars_error_type error;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;

    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("MMAP cache try constructor escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_cache_mmap_ctor_try(context, 4096, 0, &cache);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Invalid number"));

    context->e->jmp = previous;
}
END_TEST

#ifdef HANDLEBARS_TESTING_EXPORTS
static int cache_try_mprotect_fail_reads;
static int cache_try_mprotect_read_calls;

static int cache_try_mprotect_injected(
    void * address,
    size_t length,
    int protection
)
{
    if( !(protection & PROT_WRITE) ) {
        cache_try_mprotect_read_calls++;
        if( cache_try_mprotect_fail_reads > 0 ) {
            cache_try_mprotect_fail_reads--;
            errno = EACCES;
            return -1;
        }
    }
    return mprotect(address, length, protection);
}

START_TEST(test_mmap_cache_try_reprotect_failure_recovers_or_poison)
{
    struct handlebars_cache * cache = NULL;
    struct handlebars_string * key;
    struct handlebars_module * module;
    struct handlebars_module * found = NULL;
    struct handlebars_cache_stat stat = { .name = "unchanged" };
    int (*original_mprotect)(void *, size_t, int) = handlebars_cache_mmap_mprotect;
    enum handlebars_error_type error;
    int read_calls_before;

    error = handlebars_cache_mmap_ctor_try(context, 2097152, 2053, &cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(cache);
    key = handlebars_string_ctor(context, HBS_STRL("cache-try-mprotect"));
    module = serialize_template("cache try mprotect");

    handlebars_cache_mmap_mprotect = &cache_try_mprotect_injected;

    /* A transient failure is recovered internally: read-only protection is
     * restored and the completed add is reported as successful. */
    cache_try_mprotect_fail_reads = 1;
    read_calls_before = cache_try_mprotect_read_calls;
    error = handlebars_cache_add_try(cache, key, module);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_int_eq(cache_try_mprotect_read_calls - read_calls_before, 2);

    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(found);
    error = handlebars_cache_release_try(cache, key, found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    /* If both the original transition and recovery fail, later operations
     * reject the poisoned cache instead of publishing writable mmap data. */
    cache_try_mprotect_fail_reads = 2;
    read_calls_before = cache_try_mprotect_read_calls;
    error = handlebars_cache_reset_try(cache);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_int_eq(cache_try_mprotect_read_calls - read_calls_before, 2);

    cache_try_mprotect_fail_reads = 0;
    error = handlebars_cache_stat_try(cache, &stat);
    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_str_eq(stat.name, "unchanged");

    handlebars_cache_mmap_mprotect = original_mprotect;
    handlebars_cache_dtor(cache);
}
END_TEST

#ifndef YY_NO_UNISTD_H
static int run_vm_cache_error_context_case(bool separate_error_context)
{
    struct handlebars_context * cache_context = context;
    struct handlebars_context * foreign_context = NULL;
    struct handlebars_cache * cache;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    int (*original_mprotect)(void *, size_t, int) =
        handlebars_cache_mmap_mprotect;
    enum handlebars_error_type error;
    int result = EXIT_FAILURE;
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    if( separate_error_context ) {
        foreign_context = handlebars_context_ctor();
        if( foreign_context == NULL ) {
            goto done;
        }
        cache_context = foreign_context;
    }
    cache = handlebars_cache_mmap_ctor(cache_context, 2097152, 2053);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("cached body"))
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("cached"), partial);
    handlebars_value_map(partials, map);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    cache_try_mprotect_fail_reads = 2;
    cache_try_mprotect_read_calls = 0;
    handlebars_cache_mmap_mprotect = &cache_try_mprotect_injected;

    error = handlebars_vm_execute_try(vm, module, input, &output);

    if( error == HANDLEBARS_ERROR
            && output == NULL
            && handlebars_error_msg(HBSCTX(vm)) != NULL
            && strstr(handlebars_error_msg(HBSCTX(vm)), "mprotect error") != NULL ) {
        result = EXIT_SUCCESS;
    }

    handlebars_cache_mmap_mprotect = original_mprotect;
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(cache);
    if( output != NULL ) {
        handlebars_string_delref(output);
    }

done:
    if( foreign_context != NULL ) {
        handlebars_context_dtor(foreign_context);
    }
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    return result;
}

START_TEST(test_vm_execute_try_catches_foreign_cache_errors)
{
    pid_t pid;
    int status;

    pid = fork();
    ck_assert_int_ge(pid, 0);
    if( pid == 0 ) {
        _exit(run_vm_cache_error_context_case(false));
    }
    ck_assert_int_eq(waitpid(pid, &status, 0), pid);
    ck_assert_msg(WIFEXITED(status), "shared-context cache child was signaled");
    ck_assert_int_eq(WEXITSTATUS(status), EXIT_SUCCESS);

    pid = fork();
    ck_assert_int_ge(pid, 0);
    if( pid == 0 ) {
        _exit(run_vm_cache_error_context_case(true));
    }
    ck_assert_int_eq(waitpid(pid, &status, 0), pid);
    ck_assert_msg(
        WIFEXITED(status),
        "foreign-context cache child terminated by signal %d",
        WIFSIGNALED(status) ? WTERMSIG(status) : 0
    );
    ck_assert_int_eq(WEXITSTATUS(status), EXIT_SUCCESS);
}
END_TEST
#endif
#endif
#endif

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

START_TEST(test_vm_cache_can_be_cleared)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template("{{> foo}}");
    struct handlebars_string * output;
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("ok"))
    );
    do {
        struct handlebars_map * map = handlebars_map_ctor(context, 1);
        map = handlebars_map_str_add(map, HBS_STRL("foo"), partial);
        handlebars_value_map(partials, map);
    } while( 0 );

    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(cache);

    output = handlebars_vm_execute(vm, module, input);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);

    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_cache_clear_during_cached_partial_releases_lookup_cache)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers tracking_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> foo}}");
    struct handlebars_string * output;
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    active_cache_original_release = original_handlers->release;
    active_cache_release_count = 0;
    tracking_handlers.release = track_active_cache_release;
    cache->hnd = &tracking_handlers;

    handlebars_value_helper(helper, clear_vm_cache_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("clearCache"), helper);
    handlebars_value_map(helpers, map);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(context, HBS_STRL("{{clearCache}}ok"))
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("foo"), partial);
    handlebars_value_map(partials, map);

    handlebars_vm_set_helpers(vm, helpers);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    output = handlebars_vm_execute(vm, module, input);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);

    handlebars_vm_set_cache(vm, cache);
    output = handlebars_vm_execute(vm, module, input);
    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);
    ck_assert_uint_ge(handlebars_cache_stat(cache).hits, 1);
    ck_assert_uint_eq(active_cache_release_count, 1);

    cache->hnd = original_handlers;
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_vm_cache_release_preserves_nonthrowing_vm_error)
{
    const char * partial_source = "{{setError}}";
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template("{{> failing}}");
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    enum handlebars_error_type error;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_cache_add(
        cache,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        ),
        serialize_template(partial_source)
    );

    handlebars_value_helper(helper, set_vm_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("setError"), helper);
    handlebars_value_map(helpers, map);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        )
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("failing"), partial);
    handlebars_value_map(partials, map);

    handlebars_vm_set_helpers(vm, helpers);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    error = handlebars_vm_execute_try(vm, module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_nonnull(strstr(
        handlebars_error_msg(HBSCTX(vm)),
        "Intentional non-throwing helper failure"
    ));
    ck_assert_uint_eq(handlebars_cache_stat(cache).hits, 1);

    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_vm_cache_lookup_preserves_nonthrowing_vm_error)
{
    const char * partial_source = "ok";
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template(
        "{{setError}}{{> cached}}"
    );
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    enum handlebars_error_type error;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_cache_add(
        cache,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        ),
        serialize_template(partial_source)
    );

    handlebars_value_helper(helper, set_vm_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("setError"), helper);
    handlebars_value_map(helpers, map);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        )
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("cached"), partial);
    handlebars_value_map(partials, map);

    handlebars_vm_set_helpers(vm, helpers);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);

    error = handlebars_vm_execute_try(vm, module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_nonnull(strstr(
        handlebars_error_msg(HBSCTX(vm)),
        "Intentional non-throwing helper failure"
    ));
    ck_assert_uint_eq(handlebars_cache_stat(cache).hits, 1);

    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_vm_cache_stale_error_allows_dynamic_partial)
{
    const char * partial_source = "ok";
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output;
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_cache_add(
        cache,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        ),
        serialize_template(partial_source)
    );
    handlebars_value_str(
        partial,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        )
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("cached"), partial);
    handlebars_value_map(partials, map);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, cache);
    handlebars_error_set(
        HBSCTX(vm),
        HANDLEBARS_ERROR,
        "Stale error from a previous legacy render"
    );

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_hbs_str_eq_cstr(output, "ok");
    ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
    ck_assert_str_eq(
        handlebars_error_msg(HBSCTX(vm)),
        "Stale error from a previous legacy render"
    );
    ck_assert_uint_eq(handlebars_cache_stat(cache).hits, 1);

    handlebars_string_delref(output);
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_cache_stale_error_does_not_mask_find_failure)
{
    const char * partial_source = "fresh cache failure";
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * caches[2] = {
        handlebars_cache_simple_ctor(context),
        handlebars_cache_simple_ctor(foreign_context)
    };
    const struct handlebars_cache_handlers * original_handlers =
        caches[0]->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output;
    struct handlebars_locinfo loc;
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(caches[0]);
    ck_assert_ptr_nonnull(caches[1]);
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_find;
    fault_handlers.find = vm_cache_fault_find;
    vm_cache_set_string_partial(context, vm, partial_source);

    for( size_t i = 0; i < 2; i++ ) {
        caches[i]->hnd = &fault_handlers;
        handlebars_vm_set_cache(vm, caches[i]);
        handlebars_error_set(
            HBSCTX(vm),
            HANDLEBARS_TYPE_ERROR,
            "Stale error from a previous legacy render"
        );

        output = handlebars_vm_execute(vm, module, input);

        ck_assert_hbs_str_eq_cstr(output, "");
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Injected cache find failure: fresh cache failure"
        );
        loc = handlebars_error_loc(HBSCTX(vm));
        ck_assert_int_eq(loc.first_line, 100 + vm_cache_fault_op_find);
        ck_assert_int_eq(loc.first_column, (int) strlen(partial_source));
        ck_assert_int_eq(loc.last_line, 200 + vm_cache_fault_op_find);
        ck_assert_int_eq(loc.last_column, (int) strlen(partial_source) + 1);
        handlebars_string_delref(output);
        caches[i]->hnd = original_handlers;
    }

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(caches[1]);
    handlebars_cache_dtor(caches[0]);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_cache_stale_error_does_not_mask_add_failure)
{
    const char * partial_source = "fresh add failure";
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * caches[2] = {
        handlebars_cache_simple_ctor(context),
        handlebars_cache_simple_ctor(foreign_context)
    };
    const struct handlebars_cache_handlers * original_handlers =
        caches[0]->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output;
    struct handlebars_locinfo loc;
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(caches[0]);
    ck_assert_ptr_nonnull(caches[1]);
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_add;
    fault_handlers.add = vm_cache_fault_add;
    vm_cache_set_string_partial(context, vm, partial_source);

    for( size_t i = 0; i < 2; i++ ) {
        caches[i]->hnd = &fault_handlers;
        handlebars_vm_set_cache(vm, caches[i]);
        handlebars_error_set(
            HBSCTX(vm),
            HANDLEBARS_TYPE_ERROR,
            "Stale error from a previous legacy render"
        );

        output = handlebars_vm_execute(vm, module, input);

        ck_assert_hbs_str_eq_cstr(output, "");
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Injected cache add failure: fresh add failure"
        );
        loc = handlebars_error_loc(HBSCTX(vm));
        ck_assert_int_eq(loc.first_line, 100 + vm_cache_fault_op_add);
        ck_assert_int_eq(loc.first_column, (int) strlen(partial_source));
        ck_assert_int_eq(loc.last_line, 200 + vm_cache_fault_op_add);
        ck_assert_int_eq(loc.last_column, (int) strlen(partial_source) + 1);
        ck_assert_uint_eq(
            handlebars_cache_stat(caches[i]).current_entries,
            0
        );
        handlebars_string_delref(output);
        caches[i]->hnd = original_handlers;
    }

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(caches[1]);
    handlebars_cache_dtor(caches[0]);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_cache_stale_error_does_not_mask_release_failure)
{
    const char * partial_source = "fresh release failure";
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * caches[2] = {
        handlebars_cache_simple_ctor(context),
        handlebars_cache_simple_ctor(foreign_context)
    };
    const struct handlebars_cache_handlers * original_handlers =
        caches[0]->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output;
    struct handlebars_locinfo loc;
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(caches[0]);
    ck_assert_ptr_nonnull(caches[1]);
    vm_cache_fault_original_handlers = original_handlers;
    fault_handlers.release = vm_cache_fault_release;
    vm_cache_set_string_partial(context, vm, partial_source);

    for( size_t i = 0; i < 2; i++ ) {
        handlebars_cache_add(
            caches[i],
            handlebars_string_ctor(
                context,
                partial_source,
                strlen(partial_source)
            ),
            serialize_template(partial_source)
        );
        caches[i]->hnd = &fault_handlers;
        handlebars_vm_set_cache(vm, caches[i]);
        handlebars_error_set(
            HBSCTX(vm),
            HANDLEBARS_TYPE_ERROR,
            "Stale error from a previous legacy render"
        );
        vm_cache_fault_current = vm_cache_fault_op_release;

        output = handlebars_vm_execute(vm, module, input);

        ck_assert_ptr_null(output);
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Injected cache release failure: fresh release failure"
        );
        loc = handlebars_error_loc(HBSCTX(vm));
        ck_assert_int_eq(loc.first_line, 100 + vm_cache_fault_op_release);
        ck_assert_int_eq(loc.first_column, (int) strlen(partial_source));
        ck_assert_int_eq(loc.last_line, 200 + vm_cache_fault_op_release);
        ck_assert_int_eq(loc.last_column, (int) strlen(partial_source) + 1);
        caches[i]->hnd = original_handlers;
    }

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(caches[1]);
    handlebars_cache_dtor(caches[0]);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST

START_TEST(test_vm_current_error_wins_later_cache_find_failure)
{
    const char * partial_source = "later cache failure";
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * caches[2] = {
        handlebars_cache_simple_ctor(context),
        handlebars_cache_simple_ctor(foreign_context)
    };
    const struct handlebars_cache_handlers * original_handlers =
        caches[0]->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template(
        "{{setError}}{{> cached}}"
    );
    struct handlebars_string * output;
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(caches[0]);
    ck_assert_ptr_nonnull(caches[1]);
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_find;
    fault_handlers.find = vm_cache_fault_find;
    vm_cache_set_string_partial(context, vm, partial_source);

    handlebars_value_helper(helper, set_vm_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("setError"), helper);
    handlebars_value_map(helpers, map);
    handlebars_vm_set_helpers(vm, helpers);

    for( size_t i = 0; i < 2; i++ ) {
        caches[i]->hnd = &fault_handlers;
        handlebars_vm_set_cache(vm, caches[i]);

        output = handlebars_vm_execute(vm, module, input);

        ck_assert_hbs_str_eq_cstr(output, "");
        handlebars_string_delref(output);
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Intentional non-throwing helper failure"
        );
        caches[i]->hnd = original_handlers;
        handlebars_error_clear(HBSCTX(vm));
    }

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(caches[1]);
    handlebars_cache_dtor(caches[0]);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
START_TEST(test_vm_stale_error_allocation_failure_restores_execution_boundary)
{
    const char * partial_source = "allocation retry body";
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    bool observed_failure = false;
    bool reached_success = false;
    HANDLEBARS_VALUE_DECL(input);

    fault_handlers.find = vm_cache_fault_find;
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_set_string_partial(context, vm, partial_source);
    handlebars_vm_set_cache(vm, cache);

    for( int fail_at = 1; fail_at <= 256; fail_at++ ) {
        struct handlebars_string * output;

        handlebars_cache_reset(cache);
        handlebars_error_clear(HBSCTX(vm));
        handlebars_error_set(
            HBSCTX(vm),
            HANDLEBARS_TYPE_ERROR,
            "Stale error before allocation failure"
        );

        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        output = handlebars_vm_execute(vm, module, input);
        handlebars_memory_fail_disable();

        if( handlebars_error_num(HBSCTX(vm)) == HANDLEBARS_TYPE_ERROR ) {
            ck_assert_hbs_str_eq_cstr(output, partial_source);
            handlebars_string_delref(output);
            ck_assert_str_eq(
                handlebars_error_msg(HBSCTX(vm)),
                "Stale error before allocation failure"
            );
            reached_success = true;
            break;
        }

        observed_failure = true;
        ck_assert_int_eq(
            handlebars_error_num(HBSCTX(vm)),
            HANDLEBARS_NOMEM
        );
        ck_assert_ptr_nonnull(strstr(
            handlebars_error_msg(HBSCTX(vm)),
            "Out of memory"
        ));
        if( output != NULL ) {
            handlebars_string_delref(output);
        }

        cache->hnd = &fault_handlers;
        vm_cache_fault_current = vm_cache_fault_op_find;
        output = handlebars_vm_execute(vm, module, input);

        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Injected cache find failure: allocation retry body"
        );
        if( output != NULL ) {
            handlebars_string_delref(output);
        }

        cache->hnd = original_handlers;
        vm_cache_fault_current = vm_cache_fault_op_none;
        output = handlebars_vm_execute(vm, module, input);

        ck_assert_hbs_str_eq_cstr(output, partial_source);
        handlebars_string_delref(output);
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Injected cache find failure: allocation retry body"
        );
    }

    ck_assert(observed_failure);
    ck_assert(reached_success);
    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_memory_fail_disable();
    handlebars_error_clear(HBSCTX(vm));
    handlebars_vm_set_cache(vm, NULL);
    cache->hnd = original_handlers;
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(input);
}
END_TEST
#endif

START_TEST(test_vm_cache_stale_error_allows_lambda_retries)
{
    const unsigned long flags =
        handlebars_compiler_flag_mustache_style_lambdas;
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * caches[2] = {
        handlebars_cache_simple_ctor(context),
        handlebars_cache_simple_ctor(foreign_context)
    };
    struct handlebars_module * failing_module = serialize_template(
        "{{setError}}"
    );
    struct handlebars_module * lambda_module = serialize_template_with_flags(
        "{{lambda}}",
        flags
    );
    struct handlebars_string * output;
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(caches[0]);
    ck_assert_ptr_nonnull(caches[1]);

    handlebars_value_helper(helper, set_vm_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("setError"), helper);
    handlebars_value_map(helpers, map);
    handlebars_vm_set_helpers(vm, helpers);

    output = handlebars_vm_execute(vm, failing_module, input);
    ck_assert_hbs_str_eq_cstr(output, "");
    handlebars_string_delref(output);
    ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
    ck_assert_str_eq(
        handlebars_error_msg(HBSCTX(vm)),
        "Intentional non-throwing helper failure"
    );

    handlebars_value_helper(helper, stale_error_lambda_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("lambda"), helper);
    handlebars_value_map(input, map);
    handlebars_vm_set_flags(vm, flags);

    for( size_t i = 0; i < 2; i++ ) {
        handlebars_vm_set_cache(vm, caches[i]);

        output = handlebars_vm_execute(vm, lambda_module, input);
        ck_assert_hbs_str_eq_cstr(output, "lambda ok");
        handlebars_string_delref(output);
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Intentional non-throwing helper failure"
        );
        ck_assert_uint_eq(
            handlebars_cache_stat(caches[i]).current_entries,
            1
        );

        output = handlebars_vm_execute(vm, lambda_module, input);
        ck_assert_hbs_str_eq_cstr(output, "lambda ok");
        handlebars_string_delref(output);
        ck_assert_int_eq(handlebars_error_num(HBSCTX(vm)), HANDLEBARS_ERROR);
        ck_assert_str_eq(
            handlebars_error_msg(HBSCTX(vm)),
            "Intentional non-throwing helper failure"
        );
        ck_assert_uint_eq(handlebars_cache_stat(caches[i]).hits, 1);
    }

    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_dtor(caches[1]);
    handlebars_cache_dtor(caches[0]);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_vm_same_context_cache_release_failure_preserves_primary_error)
{
    const char * partial_source = "{{primaryCacheFailure}}";
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    struct handlebars_locinfo loc;
    enum handlebars_error_type error;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);

    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_none;
    fault_handlers.release = vm_cache_fault_release;
    cache->hnd = &fault_handlers;
    handlebars_vm_set_cache(vm, cache);
    handlebars_cache_add(
        cache,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        ),
        serialize_template(partial_source)
    );
    vm_cache_set_string_partial(context, vm, partial_source);

    handlebars_value_helper(helper, vm_cache_primary_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(
        map,
        HBS_STRL("primaryCacheFailure"),
        helper
    );
    handlebars_value_map(helpers, map);
    handlebars_vm_set_helpers(vm, helpers);
    vm_cache_fault_current = vm_cache_fault_op_release;

    error = handlebars_vm_execute_try(vm, module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_str_eq(handlebars_error_msg(HBSCTX(vm)), "Primary VM failure");
    loc = handlebars_error_loc(HBSCTX(vm));
    ck_assert_int_eq(loc.first_line, 701);
    ck_assert_int_eq(loc.first_column, 702);
    ck_assert_int_eq(loc.last_line, 703);
    ck_assert_int_eq(loc.last_column, 704);
    ck_assert_uint_eq(handlebars_cache_stat(cache).hits, 1);

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    cache->hnd = original_handlers;
    handlebars_cache_dtor(cache);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

#ifdef HANDLEBARS_HAVE_PTHREAD
START_TEST(test_vm_cache_replacement_preserves_nested_active_hits)
{
    const char * outer_source = "{{replaceCache}}{{> inner}}";
    const char * inner_source = "{{observeCaches}}ok";
    struct handlebars_cache * old_cache = handlebars_cache_mmap_ctor(
        context,
        2097152,
        2053
    );
    struct handlebars_cache * new_cache = handlebars_cache_mmap_ctor(
        context,
        2097152,
        2053
    );
    struct handlebars_module * module = serialize_template("{{> outer}}");
    struct handlebars_string * output;
    struct handlebars_map * map;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(inner_partial);
    HANDLEBARS_VALUE_DECL(outer_partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_cache_add(
        old_cache,
        handlebars_string_ctor(context, outer_source, strlen(outer_source)),
        serialize_template(outer_source)
    );
    handlebars_cache_add(
        new_cache,
        handlebars_string_ctor(context, inner_source, strlen(inner_source)),
        serialize_template(inner_source)
    );

    handlebars_value_helper(helper, replace_vm_cache_helper);
    map = handlebars_map_ctor(context, 2);
    map = handlebars_map_str_add(map, HBS_STRL("replaceCache"), helper);
    handlebars_value_helper(helper, observe_transition_caches_helper);
    map = handlebars_map_str_add(map, HBS_STRL("observeCaches"), helper);
    handlebars_value_map(helpers, map);

    handlebars_value_str(
        outer_partial,
        handlebars_string_ctor(context, outer_source, strlen(outer_source))
    );
    handlebars_value_str(
        inner_partial,
        handlebars_string_ctor(context, inner_source, strlen(inner_source))
    );
    map = handlebars_map_ctor(context, 2);
    map = handlebars_map_str_add(map, HBS_STRL("outer"), outer_partial);
    map = handlebars_map_str_add(map, HBS_STRL("inner"), inner_partial);
    handlebars_value_map(partials, map);

    cache_transition_old = old_cache;
    cache_transition_new = new_cache;
    cache_transition_observed_old_refcount = 0;
    cache_transition_observed_new_refcount = 0;
    handlebars_vm_set_helpers(vm, helpers);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, old_cache);

    output = handlebars_vm_execute(vm, module, input);

    ck_assert_hbs_str_eq_cstr(output, "ok");
    handlebars_string_delref(output);
    ck_assert_uint_eq(cache_transition_observed_old_refcount, 1);
    ck_assert_uint_eq(cache_transition_observed_new_refcount, 1);
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).hits, 1);
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).refcount, 0);
    ck_assert_uint_eq(handlebars_cache_stat(new_cache).hits, 1);
    ck_assert_uint_eq(handlebars_cache_stat(new_cache).refcount, 0);

    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_reset(old_cache);
    handlebars_cache_reset(new_cache);
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).current_entries, 0);
    ck_assert_uint_eq(handlebars_cache_stat(new_cache).current_entries, 0);
    handlebars_cache_dtor(new_cache);
    handlebars_cache_dtor(old_cache);
    cache_transition_old = NULL;
    cache_transition_new = NULL;
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(outer_partial);
    HANDLEBARS_VALUE_UNDECL(inner_partial);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

START_TEST(test_vm_cache_replacement_releases_hit_after_helper_error)
{
    const char * partial_source = "{{replaceCacheAndThrow}}";
    struct handlebars_cache * old_cache = handlebars_cache_mmap_ctor(
        context,
        2097152,
        2053
    );
    struct handlebars_cache * new_cache = handlebars_cache_simple_ctor(context);
    struct handlebars_module * module = serialize_template("{{> failing}}");
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    enum handlebars_error_type error;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);
    HANDLEBARS_VALUE_DECL(partial);
    HANDLEBARS_VALUE_DECL(partials);

    handlebars_cache_add(
        old_cache,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        ),
        serialize_template(partial_source)
    );

    handlebars_value_helper(helper, replace_vm_cache_and_throw_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(
        map,
        HBS_STRL("replaceCacheAndThrow"),
        helper
    );
    handlebars_value_map(helpers, map);

    handlebars_value_str(
        partial,
        handlebars_string_ctor(
            context,
            partial_source,
            strlen(partial_source)
        )
    );
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("failing"), partial);
    handlebars_value_map(partials, map);

    cache_transition_old = old_cache;
    cache_transition_new = new_cache;
    handlebars_vm_set_helpers(vm, helpers);
    handlebars_vm_set_partials(vm, partials);
    handlebars_vm_set_cache(vm, old_cache);

    error = handlebars_vm_execute_try(vm, module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_ptr_nonnull(strstr(
        handlebars_error_msg(HBSCTX(vm)),
        "Intentional cache transition failure"
    ));
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).hits, 1);
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).refcount, 0);
    ck_assert_uint_eq(handlebars_cache_stat(new_cache).hits, 0);
    ck_assert_uint_eq(handlebars_cache_stat(new_cache).refcount, 0);

    handlebars_vm_set_cache(vm, NULL);
    handlebars_cache_reset(old_cache);
    ck_assert_uint_eq(handlebars_cache_stat(old_cache).current_entries, 0);
    handlebars_cache_dtor(new_cache);
    handlebars_cache_dtor(old_cache);
    cache_transition_old = NULL;
    cache_transition_new = NULL;
    HANDLEBARS_VALUE_UNDECL(partials);
    HANDLEBARS_VALUE_UNDECL(partial);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST
#endif

START_TEST(test_vm_cache_setters_borrow_without_reparenting_or_retaining)
{
    struct handlebars_context * first_owner = handlebars_context_ctor_ex(context);
    struct handlebars_context * second_owner = handlebars_context_ctor_ex(context);
    struct handlebars_cache * first_cache = handlebars_cache_simple_ctor(
        first_owner
    );
    struct handlebars_cache * second_cache = handlebars_cache_simple_ctor(
        second_owner
    );

    borrowed_cache_destroy_count = 0;
    talloc_set_destructor(first_cache, borrowed_cache_dtor);
    talloc_set_destructor(second_cache, borrowed_cache_dtor);

    handlebars_vm_set_cache(vm, first_cache);
    ck_assert_ptr_eq(talloc_parent(first_cache), first_owner);
    ck_assert_uint_eq(borrowed_cache_destroy_count, 0);

    handlebars_vm_set_cache(vm, second_cache);
    ck_assert_ptr_eq(talloc_parent(first_cache), first_owner);
    ck_assert_ptr_eq(talloc_parent(second_cache), second_owner);
    ck_assert_uint_eq(borrowed_cache_destroy_count, 0);

    handlebars_context_dtor(first_owner);
    ck_assert_uint_eq(borrowed_cache_destroy_count, 1);

    handlebars_vm_set_cache(vm, NULL);
    ck_assert_ptr_eq(talloc_parent(second_cache), second_owner);
    ck_assert_uint_eq(borrowed_cache_destroy_count, 1);

    handlebars_context_dtor(second_owner);
    ck_assert_uint_eq(borrowed_cache_destroy_count, 2);
}
END_TEST

START_TEST(test_vm_foreign_cache_propagates_operation_errors)
{
    const char * release_source = "release cache body";
    const char * primary_source = "{{primaryCacheFailure}}";
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(
        foreign_context
    );
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_string * key;
    struct handlebars_string * output = (void *) 1;
    struct handlebars_map * map;
    struct handlebars_locinfo loc;
    enum handlebars_error_type error;
    HANDLEBARS_VALUE_DECL(helper);
    HANDLEBARS_VALUE_DECL(helpers);
    HANDLEBARS_VALUE_DECL(input);

    ck_assert_ptr_nonnull(foreign_context);
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_none;
    fault_handlers.find = vm_cache_fault_find;
    fault_handlers.add = vm_cache_fault_add;
    fault_handlers.release = vm_cache_fault_release;
    cache->hnd = &fault_handlers;
    handlebars_vm_set_cache(vm, cache);

    vm_cache_assert_operation_failure(
        vm,
        module,
        input,
        "find cache body",
        vm_cache_fault_op_find
    );
    vm_cache_assert_operation_failure(
        vm,
        module,
        input,
        "add cache body",
        vm_cache_fault_op_add
    );

    vm_cache_fault_current = vm_cache_fault_op_none;
    key = handlebars_string_ctor(
        context,
        release_source,
        strlen(release_source)
    );
    handlebars_cache_add(cache, key, serialize_template(release_source));
    vm_cache_assert_operation_failure(
        vm,
        module,
        input,
        release_source,
        vm_cache_fault_op_release
    );

    handlebars_value_helper(helper, vm_cache_primary_error_helper);
    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(
        map,
        HBS_STRL("primaryCacheFailure"),
        helper
    );
    handlebars_value_map(helpers, map);
    handlebars_vm_set_helpers(vm, helpers);

    vm_cache_fault_current = vm_cache_fault_op_none;
    key = handlebars_string_ctor(
        context,
        primary_source,
        strlen(primary_source)
    );
    handlebars_cache_add(cache, key, serialize_template(primary_source));
    vm_cache_set_string_partial(context, vm, primary_source);
    vm_cache_fault_current = vm_cache_fault_op_release;

    error = handlebars_vm_execute_try(vm, module, input, &output);

    ck_assert_int_eq(error, HANDLEBARS_ERROR);
    ck_assert_ptr_null(output);
    ck_assert_str_eq(handlebars_error_msg(HBSCTX(vm)), "Primary VM failure");
    loc = handlebars_error_loc(HBSCTX(vm));
    ck_assert_int_eq(loc.first_line, 701);
    ck_assert_int_eq(loc.first_column, 702);
    ck_assert_int_eq(loc.last_line, 703);
    ck_assert_int_eq(loc.last_column, 704);
    ck_assert_str_eq(
        handlebars_error_msg(HBSCTX(cache)),
        "Injected cache release failure: {{primaryCacheFailure}}"
    );

    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_set_cache(vm, NULL);
    cache->hnd = original_handlers;
    handlebars_cache_dtor(cache);
    handlebars_context_dtor(foreign_context);
    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(helpers);
    HANDLEBARS_VALUE_UNDECL(helper);
}
END_TEST

#ifdef HANDLEBARS_HAVE_PTHREAD
struct vm_cache_concurrent_error_start {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned int ready;
    bool start;
};

struct vm_cache_concurrent_error_arg {
    struct vm_cache_concurrent_error_start * start;
    struct handlebars_vm * vm;
    struct handlebars_module * module;
    const char * source;
    unsigned int failures;
};

static void * vm_cache_concurrent_error_thread(void * opaque)
{
    struct vm_cache_concurrent_error_arg * arg = opaque;
    char expected[160];
    HANDLEBARS_VALUE_DECL(input);

    if( pthread_mutex_lock(&arg->start->lock) != 0 ) {
        arg->failures++;
        goto done;
    }
    arg->start->ready++;
    pthread_cond_broadcast(&arg->start->cond);
    while( !arg->start->start ) {
        if( pthread_cond_wait(&arg->start->cond, &arg->start->lock) != 0 ) {
            arg->failures++;
            pthread_mutex_unlock(&arg->start->lock);
            goto done;
        }
    }
    pthread_mutex_unlock(&arg->start->lock);

    snprintf(
        expected,
        sizeof(expected),
        "Injected cache find failure: %s",
        arg->source
    );
    for( unsigned int i = 0; i < 200; i++ ) {
        struct handlebars_string * output = (void *) 1;
        struct handlebars_locinfo loc;
        enum handlebars_error_type error = handlebars_vm_execute_try(
            arg->vm,
            arg->module,
            input,
            &output
        );
        const char * message = handlebars_error_msg(HBSCTX(arg->vm));

        loc = handlebars_error_loc(HBSCTX(arg->vm));
        if( error != HANDLEBARS_ERROR
                || output != NULL
                || message == NULL
                || strcmp(message, expected) != 0
                || loc.first_line != 100 + vm_cache_fault_op_find
                || loc.first_column != (int) strlen(arg->source)
                || loc.last_line != 200 + vm_cache_fault_op_find
                || loc.last_column != (int) strlen(arg->source) + 1 ) {
            arg->failures++;
        }
        if( output != NULL ) {
            handlebars_string_delref(output);
        }
    }

done:
    HANDLEBARS_VALUE_UNDECL(input);
    return NULL;
}

START_TEST(test_concurrent_vms_isolate_foreign_cache_errors)
{
    struct handlebars_context * foreign_context = handlebars_context_ctor();
    struct handlebars_context * first_context = handlebars_context_ctor();
    struct handlebars_context * second_context = handlebars_context_ctor();
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(
        foreign_context
    );
    const struct handlebars_cache_handlers * original_handlers = cache->hnd;
    struct handlebars_cache_handlers fault_handlers = *original_handlers;
    struct handlebars_module * module = serialize_template("{{> cached}}");
    struct handlebars_vm * first_vm = handlebars_vm_ctor(first_context);
    struct handlebars_vm * second_vm = handlebars_vm_ctor(second_context);
    struct vm_cache_concurrent_error_start start = {0};
    struct vm_cache_concurrent_error_arg first = {
        .start = &start,
        .vm = first_vm,
        .module = module,
        .source = "first concurrent cache diagnostic"
    };
    struct vm_cache_concurrent_error_arg second = {
        .start = &start,
        .vm = second_vm,
        .module = module,
        .source = "second cache diagnostic"
    };
    pthread_t first_thread;
    pthread_t second_thread;

    ck_assert_ptr_nonnull(foreign_context);
    ck_assert_ptr_nonnull(first_context);
    ck_assert_ptr_nonnull(second_context);
    vm_cache_fault_original_handlers = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_find;
    fault_handlers.find = vm_cache_fault_find;
    cache->hnd = &fault_handlers;

    vm_cache_set_string_partial(first_context, first_vm, first.source);
    vm_cache_set_string_partial(second_context, second_vm, second.source);
    handlebars_vm_set_cache(first_vm, cache);
    handlebars_vm_set_cache(second_vm, cache);

    ck_assert_int_eq(pthread_mutex_init(&start.lock, NULL), 0);
    ck_assert_int_eq(pthread_cond_init(&start.cond, NULL), 0);
    ck_assert_int_eq(pthread_create(
        &first_thread,
        NULL,
        vm_cache_concurrent_error_thread,
        &first
    ), 0);
    ck_assert_int_eq(pthread_create(
        &second_thread,
        NULL,
        vm_cache_concurrent_error_thread,
        &second
    ), 0);

    ck_assert_int_eq(pthread_mutex_lock(&start.lock), 0);
    while( start.ready < 2 ) {
        ck_assert_int_eq(pthread_cond_wait(&start.cond, &start.lock), 0);
    }
    start.start = true;
    ck_assert_int_eq(pthread_cond_broadcast(&start.cond), 0);
    ck_assert_int_eq(pthread_mutex_unlock(&start.lock), 0);

    ck_assert_int_eq(pthread_join(first_thread, NULL), 0);
    ck_assert_int_eq(pthread_join(second_thread, NULL), 0);
    ck_assert_uint_eq(first.failures, 0);
    ck_assert_uint_eq(second.failures, 0);
    ck_assert_int_eq(pthread_cond_destroy(&start.cond), 0);
    ck_assert_int_eq(pthread_mutex_destroy(&start.lock), 0);

    handlebars_vm_set_cache(first_vm, NULL);
    handlebars_vm_set_cache(second_vm, NULL);
    cache->hnd = original_handlers;
    vm_cache_fault_current = vm_cache_fault_op_none;
    vm_cache_fault_original_handlers = NULL;
    handlebars_vm_dtor(first_vm);
    handlebars_vm_dtor(second_vm);
    handlebars_cache_dtor(cache);
    handlebars_context_dtor(first_context);
    handlebars_context_dtor(second_context);
    handlebars_context_dtor(foreign_context);
}
END_TEST
#endif

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
START_TEST(test_simple_cache_try_handles_allocation_failures)
{
    struct handlebars_cache * cache = NULL;
    struct handlebars_cache_try_guard guard;
    struct handlebars_string * key;
    struct handlebars_module * module;
    struct handlebars_locinfo loc;
    void * module_parent;
    enum handlebars_error_type error;
    int removed = 99;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;

    if( setjmp(outer) != 0 ) {
        handlebars_memory_fail_disable();
        context->e->jmp = previous;
        ck_abort_msg("A cache try API escaped an allocation failure");
    }
    context->e->jmp = &outer;

    context->e->num = HANDLEBARS_ERROR;
    context->e->msg = handlebars_talloc_strdup(context->e, "stale guard error");
    context->e->loc.first_line = 7;
    context->e->loc.first_column = 8;
    context->e->loc.last_line = 9;
    context->e->loc.last_column = 10;

    /* The first allocation creates the persistent per-error try guard. */
    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    error = handlebars_cache_simple_ctor_try(context, &cache);
    handlebars_memory_fail_disable();

    ck_assert_int_eq(error, HANDLEBARS_NOMEM);
    ck_assert_ptr_null(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);
    ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
    ck_assert_ptr_nonnull(handlebars_error_msg(context));
    ck_assert_ptr_nonnull(strstr(handlebars_error_msg(context), "Out of memory"));
    loc = handlebars_error_loc(context);
    ck_assert_int_eq(loc.first_line, 0);
    ck_assert_int_eq(loc.first_column, 0);
    ck_assert_int_eq(loc.last_line, 0);
    ck_assert_int_eq(loc.last_column, 0);

    /* Prewarm the persistent guard so the next failure reaches the actual
     * constructor allocation rather than guard setup. */
    error = handlebars_cache_try_guard_begin(context->e, &guard);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    error = handlebars_cache_try_guard_end(&guard);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    error = handlebars_cache_simple_ctor_try(context, &cache);
    handlebars_memory_fail_disable();

    ck_assert_int_eq(error, HANDLEBARS_NOMEM);
    ck_assert_ptr_null(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    error = handlebars_cache_simple_ctor_try(context, &cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(cache);

    key = handlebars_string_ctor(context, HBS_STRL("simple-try-nomem"));
    module = serialize_template("simple try nomem");
    module_parent = talloc_parent(module);

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    error = handlebars_cache_add_try(cache, key, module);
    handlebars_memory_fail_disable();

    ck_assert_int_eq(error, HANDLEBARS_NOMEM);
    ck_assert_ptr_eq(talloc_parent(module), module_parent);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    error = handlebars_cache_add_try(cache, key, module);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    cache->max_age = 0;

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    error = handlebars_cache_gc_try(cache, &removed);
    handlebars_memory_fail_disable();

    ck_assert_int_eq(error, HANDLEBARS_NOMEM);
    ck_assert_int_eq(removed, 99);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(1);
    error = handlebars_cache_reset_try(cache);
    handlebars_memory_fail_disable();

    ck_assert_int_eq(error, HANDLEBARS_NOMEM);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    context->e->jmp = previous;
    handlebars_cache_dtor(cache);
}
END_TEST

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

    cache_test_module_destroyed = false;
    talloc_set_destructor(module, cache_test_module_dtor);
    handlebars_cache_add(cache, key, module);
    ck_assert(!cache_test_module_destroyed);

    handlebars_cache_reset(cache);
    ck_assert(cache_test_module_destroyed);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 0);

    handlebars_cache_dtor(cache);
}
END_TEST

START_TEST(test_simple_cache_release_preserves_owned_entry)
{
    struct handlebars_cache * cache = handlebars_cache_simple_ctor(context);
    struct handlebars_string * key = handlebars_string_ctor(
        context,
        HBS_STRL("simple-find-release")
    );
    struct handlebars_module * module = serialize_template("find release");
    struct handlebars_module * found;

    cache_test_module_destroyed = false;
    talloc_set_destructor(module, cache_test_module_dtor);
    handlebars_cache_add(cache, key, module);

    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_eq(found, module);
    handlebars_cache_release(cache, key, found);
    ck_assert(!cache_test_module_destroyed);

    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_eq(found, module);
    handlebars_cache_release(cache, key, found);

    handlebars_cache_dtor(cache);
    ck_assert(cache_test_module_destroyed);
}
END_TEST

#ifdef HANDLEBARS_HAVE_LMDB
START_TEST(test_lmdb_cache_try_api)
{
    struct handlebars_cache * cache = (void *) 1;
    struct handlebars_string * key = handlebars_string_ctor(
        context,
        HBS_STRL("lmdb-try")
    );
    struct handlebars_module * module = serialize_template("lmdb try");
    struct handlebars_module * found = (void *) 1;
    struct handlebars_cache_stat stat = { .name = "unchanged" };
    enum handlebars_error_type error;
    jmp_buf * previous = context->e->jmp;
    jmp_buf outer;

    reset_lmdb_test_files();
    if( setjmp(outer) != 0 ) {
        context->e->jmp = previous;
        ck_abort_msg("An LMDB cache try API escaped through longjmp");
    }
    context->e->jmp = &outer;

    error = handlebars_cache_lmdb_ctor_try(context, lmdb_db_file, &cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(cache);
    ck_assert_ptr_eq(context->e->jmp, &outer);

    error = handlebars_cache_add_try(cache, key, module);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_nonnull(found);

    error = handlebars_cache_release_try(cache, key, found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    error = handlebars_cache_stat_try(cache, &stat);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_str_eq(stat.name, "lmdb");
    ck_assert_uint_eq(stat.current_entries, 1);

    error = handlebars_cache_reset_try(cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);

    found = (void *) 1;
    error = handlebars_cache_find_try(cache, key, &found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    ck_assert_ptr_null(found);

    context->e->jmp = previous;
    handlebars_cache_dtor(cache);
    reset_lmdb_test_files();
}
END_TEST

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

START_TEST(test_lmdb_cache_release_frees_lookup_copy)
{
    struct handlebars_cache * cache;
    struct handlebars_string * key = handlebars_string_ctor(
        context,
        HBS_STRL("lmdb-find-release")
    );
    struct handlebars_module * module = serialize_template("find release");
    struct handlebars_module * found;

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, key, module);

    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_nonnull(found);
    ck_assert_ptr_ne(found, module);
    cache_test_module_destroyed = false;
    talloc_set_destructor(found, cache_test_module_dtor);

    handlebars_cache_release(cache, key, found);
    ck_assert(cache_test_module_destroyed);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 1);

    handlebars_cache_dtor(cache);
    reset_lmdb_test_files();
}
END_TEST

START_TEST(test_lmdb_cache_distinguishes_binary_and_empty_keys)
{
    static const char embedded_key[] = {'a', '\0', 'b'};
    struct handlebars_cache * cache;
    struct handlebars_string * empty = handlebars_string_ctor(context, HBS_STRL(""));
    struct handlebars_string * prefix = handlebars_string_ctor(context, HBS_STRL("a"));
    struct handlebars_string * embedded = handlebars_string_ctor(
        context,
        embedded_key,
        sizeof(embedded_key)
    );
    struct handlebars_module * empty_module = serialize_template("empty");
    struct handlebars_module * prefix_module = serialize_template("prefix");
    struct handlebars_module * embedded_module = serialize_template("embedded");
    struct handlebars_module * found;

    reset_lmdb_test_files();
    cache = handlebars_cache_lmdb_ctor(context, lmdb_db_file);
    handlebars_cache_add(cache, empty, empty_module);
    handlebars_cache_add(cache, prefix, prefix_module);
    handlebars_cache_add(cache, embedded, embedded_module);
    ck_assert_uint_eq(handlebars_cache_stat(cache).current_entries, 3);

    handlebars_talloc_free(empty);
    handlebars_talloc_free(prefix);
    handlebars_talloc_free(embedded);
    empty = handlebars_string_ctor(context, HBS_STRL(""));
    prefix = handlebars_string_ctor(context, HBS_STRL("a"));
    embedded = handlebars_string_ctor(context, embedded_key, sizeof(embedded_key));

    found = handlebars_cache_find(cache, empty);
    ck_assert_ptr_nonnull(found);
    handlebars_cache_release(cache, empty, found);
    found = handlebars_cache_find(cache, prefix);
    ck_assert_ptr_nonnull(found);
    handlebars_cache_release(cache, prefix, found);
    found = handlebars_cache_find(cache, embedded);
    ck_assert_ptr_nonnull(found);
    handlebars_cache_release(cache, embedded, found);

    handlebars_cache_dtor(cache);
    reset_lmdb_test_files();
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

START_TEST(test_mmap_cache_release_allows_deferred_reset)
{
    struct handlebars_cache * cache = handlebars_cache_mmap_ctor(
        context,
        2097152,
        2053
    );
    struct handlebars_string * key = handlebars_string_ctor(
        context,
        HBS_STRL("mmap-find-release")
    );
    struct handlebars_module * module = serialize_template("find release");
    struct handlebars_module * found;
    struct handlebars_cache_stat stat;
    enum handlebars_error_type error;

    handlebars_cache_add(cache, key, module);
    found = handlebars_cache_find(cache, key);
    ck_assert_ptr_nonnull(found);

    stat = handlebars_cache_stat(cache);
    ck_assert_uint_eq(stat.current_entries, 1);
    ck_assert_int_eq(stat.refcount, 1);

    error = handlebars_cache_reset_try(cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    stat = handlebars_cache_stat(cache);
    ck_assert_uint_eq(stat.current_entries, 1);
    ck_assert_int_eq(stat.refcount, 1);

    error = handlebars_cache_release_try(cache, key, found);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    stat = handlebars_cache_stat(cache);
    ck_assert_uint_eq(stat.current_entries, 1);
    ck_assert_int_eq(stat.refcount, 0);

    error = handlebars_cache_reset_try(cache);
    ck_assert_int_eq(error, HANDLEBARS_SUCCESS);
    stat = handlebars_cache_stat(cache);
    ck_assert_uint_eq(stat.current_entries, 0);
    ck_assert_int_eq(stat.refcount, 0);
    ck_assert_ptr_null(handlebars_cache_find(cache, key));

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
#ifdef HANDLEBARS_HAVE_PTHREAD
    REGISTER_TEST_FIXTURE(s, test_cache_try_concurrent_calls_restore_jump_target, "Cache try API concurrent jump target restoration");
    REGISTER_TEST_FIXTURE(s, test_cache_try_independent_contexts_progress_during_reset_destructor, "Independent cache try contexts progress during reset destructor");
    REGISTER_TEST_FIXTURE(s, test_concurrent_vms_isolate_foreign_cache_errors, "Concurrent VMs isolate foreign cache errors");
#if defined(HANDLEBARS_TESTING_EXPORTS) && !defined(YY_NO_UNISTD_H)
    REGISTER_TEST_FIXTURE(s, test_vm_execute_try_catches_foreign_cache_errors, "VM try execution catches foreign cache errors");
#endif
#endif
    REGISTER_TEST_FIXTURE(s, test_simple_cache_try_api, "Simple cache try API");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_owns_key, "Simple cache owns key");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_can_be_cleared, "VM cache can be cleared");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_clear_during_cached_partial_releases_lookup_cache, "VM cache changes preserve active cache hits");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_release_preserves_nonthrowing_vm_error, "VM cache release preserves non-throwing VM errors");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_lookup_preserves_nonthrowing_vm_error, "VM cache lookup preserves non-throwing VM errors");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_stale_error_allows_dynamic_partial, "VM stale errors do not suppress dynamic partials");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_stale_error_does_not_mask_find_failure, "VM stale errors do not mask cache failures");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_stale_error_does_not_mask_add_failure, "VM stale errors do not mask cache add failures");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_stale_error_does_not_mask_release_failure, "VM stale errors do not mask cache release failures");
    REGISTER_TEST_FIXTURE(s, test_vm_current_error_wins_later_cache_find_failure, "VM current errors win later cache failures");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_vm_stale_error_allocation_failure_restores_execution_boundary, "VM allocation failures restore stale-error execution boundaries");
#endif
    REGISTER_TEST_FIXTURE(s, test_vm_cache_stale_error_allows_lambda_retries, "VM stale errors do not suppress lambda retries");
    REGISTER_TEST_FIXTURE(s, test_vm_same_context_cache_release_failure_preserves_primary_error, "VM primary errors survive same-context cache release failures");
#ifdef HANDLEBARS_HAVE_PTHREAD
    REGISTER_TEST_FIXTURE(s, test_vm_cache_replacement_preserves_nested_active_hits, "VM cache replacement preserves nested active hits");
    REGISTER_TEST_FIXTURE(s, test_vm_cache_replacement_releases_hit_after_helper_error, "VM cache replacement releases hits after helper errors");
#endif
    REGISTER_TEST_FIXTURE(s, test_vm_cache_setters_borrow_without_reparenting_or_retaining, "VM cache setters borrow without retaining");
    REGISTER_TEST_FIXTURE(s, test_vm_foreign_cache_propagates_operation_errors, "VM propagates foreign cache operation errors");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_refuses_entry_over_capacity, "Simple cache refuses entries over capacity");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_does_not_evict_executing_module, "Simple cache keeps executing modules alive");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_rejects_oversized_module_without_taking_ownership, "Simple cache rejects oversized modules");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_duplicate_preserves_module_ownership, "Simple cache duplicate preserves ownership");
#ifdef HANDLEBARS_MEMORY
    REGISTER_TEST_FIXTURE(s, test_simple_cache_try_handles_allocation_failures, "Simple cache try API handles allocation failures");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_add_nomem_preserves_module_ownership, "Simple cache add preserves ownership after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc_nomem_keeps_cache_consistent, "Simple cache GC remains consistent after allocation failure");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset_nomem_preserves_entries, "Simple cache reset preserves entries after allocation failure");
#endif
    REGISTER_TEST_FIXTURE(s, test_simple_cache_gc, "Simple Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset, "Simple Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_reset_releases_entries, "Simple cache reset releases entries");
    REGISTER_TEST_FIXTURE(s, test_simple_cache_release_preserves_owned_entry, "Simple cache release preserves owned entry");
#ifdef HANDLEBARS_HAVE_LMDB
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_try_constructor_reports_errors, "LMDB cache try constructor errors");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_try_api, "LMDB cache try API");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_gc, "LMDB Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset, "LMDB Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_reset_removes_entries, "LMDB reset removes entries");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_release_frees_lookup_copy, "LMDB cache release frees lookup copy");
    REGISTER_TEST_FIXTURE(s, test_lmdb_cache_distinguishes_binary_and_empty_keys, "LMDB distinguishes binary and empty keys");
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
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_try_constructor_reports_errors, "MMAP cache try constructor errors");
#ifdef HANDLEBARS_TESTING_EXPORTS
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_try_reprotect_failure_recovers_or_poison, "MMAP cache try reprotection failures");
#endif
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_gc, "MMAP Cache (GC)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_reset, "MMAP Cache (Reset)");
    REGISTER_TEST_FIXTURE(s, test_mmap_cache_release_allows_deferred_reset, "MMAP cache release allows deferred reset");
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

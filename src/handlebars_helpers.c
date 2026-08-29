/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"
#include "handlebars_vm_private.h"

#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#include "handlebars_helpers_ht.h"
#pragma GCC diagnostic pop



const size_t HANDLEBARS_OPTIONS_SIZE = sizeof(struct handlebars_options);

#undef CONTEXT
#define CONTEXT HBSCTX(vm)

void handlebars_options_deinit(struct handlebars_options * options)
{
    if (options->data) {
        options->data = NULL;
    }
    if (options->hash) {
        handlebars_value_dtor(options->hash);
    }
    if (options->scope) {
        handlebars_value_dtor(options->scope);
    }
}

struct handlebars_each_call_state {
    int argc;
    struct handlebars_options * options;
    struct handlebars_value * rv;
    struct handlebars_value * context;
    size_t iteration;
    struct handlebars_value rv2;
    struct handlebars_value index;
    struct handlebars_value key;
    struct handlebars_value first;
    struct handlebars_value last;
    struct handlebars_value data;
    struct handlebars_value block_params;
    struct handlebars_value lambda_argv[1];
    struct handlebars_string * result;
    struct handlebars_string * nested_result;
    struct handlebars_map * data_map;
    struct handlebars_vm_call_checkpoint checkpoint;
};

static void handlebars_each_call_state_deinit(
    struct handlebars_each_call_state * state
)
{
    if( state->nested_result != NULL ) {
        handlebars_string_delref(state->nested_result);
    }
    if( state->result != NULL ) {
        handlebars_string_delref(state->result);
    }
    if( state->data_map != NULL ) {
        handlebars_map_delref(state->data_map);
    }
    handlebars_value_dtor(&state->lambda_argv[0]);
    handlebars_value_dtor(&state->block_params);
    handlebars_value_dtor(&state->data);
    handlebars_value_dtor(&state->last);
    handlebars_value_dtor(&state->first);
    handlebars_value_dtor(&state->key);
    handlebars_value_dtor(&state->index);
    handlebars_value_dtor(&state->rv2);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void handlebars_builtin_each_guarded(
    struct handlebars_vm * vm,
    struct handlebars_each_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    short use_data;
    size_t len;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    if( state->argc < 1 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Must pass iterator to #each");
    }

    state->result = handlebars_string_ctor(CONTEXT, HBS_STRL(""));
    use_data = (state->options->data != NULL);

    if( handlebars_value_is_callable(state->context) ) {
        const int argc2 = 1;
        handlebars_value_value(
            &state->lambda_argv[0],
            state->options->scope
        );
        state->context = handlebars_value_call(
            state->context,
            argc2,
            state->lambda_argv,
            state->options,
            vm,
            &state->rv2
        );
    }

    if( handlebars_value_get_type(state->context) != HANDLEBARS_VALUE_TYPE_MAP
            && handlebars_value_get_type(state->context) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
        goto whoopsie;
    }

    if( use_data ) {
        handlebars_value_array(
            &state->block_params,
            handlebars_stack_ctor(CONTEXT, 2)
        );

        if( handlebars_value_get_type(state->options->data) == HANDLEBARS_VALUE_TYPE_MAP ) {
            state->data_map = handlebars_map_ctor(
                CONTEXT,
                handlebars_value_count(state->options->data) + 4
            );
            HANDLEBARS_VALUE_FOREACH_KV(state->options->data, options_key, child) {
                state->data_map = handlebars_map_update(
                    state->data_map,
                    options_key,
                    child
                );
            } HANDLEBARS_VALUE_FOREACH_END();
        } else {
            state->data_map = handlebars_map_ctor(CONTEXT, 4);
        }
        handlebars_map_addref(state->data_map);
    }

    len = handlebars_value_count(state->context);
    if (len <= 0) goto whoopsie;
    len--;

    HANDLEBARS_VALUE_FOREACH_IDX_KV(state->context, it_index, it_key, it_child) {
        // Disabled for Regressions - Undefined helper context
        // if( it.current->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        //     i++;
        //     continue;
        // }

        if( it_key /*it->value->type == HANDLEBARS_VALUE_TYPE_MAP*/ ) {
            handlebars_value_str(&state->key, it_key);
        } else {
            handlebars_value_integer(&state->key, it_index);
        }

        if( use_data && state->data_map ) {
            if( it_index ) {
                handlebars_value_integer(&state->index, it_index);
            } else {
                handlebars_value_integer(&state->index, state->iteration);
            }
            handlebars_value_boolean(
                &state->first,
                state->iteration == 0
            );
            handlebars_value_boolean(
                &state->last,
                state->iteration == len
            );

            handlebars_value_array_set(&state->block_params, 0, it_child);
            handlebars_value_array_set(&state->block_params, 1, &state->key);

            state->data_map = handlebars_map_str_update(
                state->data_map,
                HBS_STRL("index"),
                &state->index
            );
            state->data_map = handlebars_map_str_update(
                state->data_map,
                HBS_STRL("key"),
                &state->key
            );
            state->data_map = handlebars_map_str_update(
                state->data_map,
                HBS_STRL("first"),
                &state->first
            );
            state->data_map = handlebars_map_str_update(
                state->data_map,
                HBS_STRL("last"),
                &state->last
            );
            handlebars_value_map(&state->data, state->data_map);
        }

        state->nested_result = handlebars_vm_execute_program_ex(
            vm,
            state->options->program,
            it_child,
            &state->data,
            &state->block_params
        );
        state->result = handlebars_string_append(
            HBSCTX(vm),
            state->result,
            HBS_STR_STRL(state->nested_result)
        );
        handlebars_string_delref(state->nested_result);
        state->nested_result = NULL;

        handlebars_value_null(&state->data);

        state->iteration++;
    } HANDLEBARS_VALUE_FOREACH_END();

whoopsie:
    if( state->iteration == 0 ) {
        state->nested_result = handlebars_vm_execute_program(
            vm,
            state->options->inverse,
            state->options->scope
        );
        assert(state->nested_result != NULL);
        state->result = handlebars_string_append(
            HBSCTX(vm),
            state->result,
            HBS_STR_STRL(state->nested_result)
        );
        handlebars_string_delref(state->nested_result);
        state->nested_result = NULL;
    }

    handlebars_value_str(state->rv, state->result);
    state->result = NULL;

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_each_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

struct handlebars_value * handlebars_builtin_each(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_each_call_state state = {0};

    state.argc = argc;
    state.options = options;
    state.rv = rv;
    state.context = argc > 0 ? argv : NULL;
    handlebars_builtin_each_guarded(vm, &state);

    return rv;
}

struct handlebars_value * handlebars_builtin_block_helper_missing(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * context;
    struct handlebars_string * result_str = NULL;
    bool is_zero;

    if( argc < 1 ) {
        goto inverse;
    }

    context = &argv[0];
    is_zero = handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_INTEGER && handlebars_value_get_intval(context) == 0;

    if( handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_TRUE ) {
        result_str = handlebars_vm_execute_program(vm, options->program, options->scope);
    } else if( handlebars_value_is_empty(context) && !is_zero ) {
inverse:
        result_str = handlebars_vm_execute_program(vm, options->inverse, options->scope);
    } else if( handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        rv = handlebars_vm_call_helper_str(HBS_STRL("each"), HANDLEBARS_HELPER_ARGS_PASSTHRU);
        goto done;
    } else {
        // For object, etc
        result_str = handlebars_vm_execute_program(vm, options->program, context);
    }

    handlebars_value_str(rv, result_str);

done:
    return rv;
}

struct handlebars_value * handlebars_builtin_helper_missing(HANDLEBARS_HELPER_ARGS)
{
    if( argc != 0 ) {
        handlebars_throw(
            CONTEXT,
            HANDLEBARS_ERROR,
            "Missing helper: \"%.*s\"",
            (int) hbs_str_len(options->name),
            hbs_str_val(options->name)
        );
    }

    return rv;
}

struct handlebars_value * handlebars_builtin_log(HANDLEBARS_HELPER_ARGS)
{
    handlebars_func log_func = handlebars_vm_get_log_func(vm);

    if (log_func) {
        rv = log_func(HANDLEBARS_FUNCTION_ARGS_PASSTHRU);
    } else {
        int i;
        fprintf(stderr, "[INFO] ");
        for (i = 0; i < argc; i++) {
            char *tmp = handlebars_value_dump(&argv[i], HBSCTX(vm), 0);
            fprintf(stderr, "%s ", tmp);
            handlebars_talloc_free(tmp);
        }
        fprintf(stderr, "\n");
        //fflush(stderr);
    }

    return rv;
}

struct handlebars_value * handlebars_builtin_lookup(HANDLEBARS_HELPER_ARGS)
{
    if( argc < 2 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "lookup requires two parameters");
    }

    struct handlebars_value * context = &argv[0];
    struct handlebars_value * field = &argv[1];
    struct handlebars_string * key = handlebars_value_to_string(field, CONTEXT);
    struct handlebars_value * result;

    result = handlebars_vm_lookup_property(vm, context, key, rv);
    handlebars_string_delref(key);

    return result != NULL ? result : rv;
}

struct handlebars_if_call_state {
    int argc;
    struct handlebars_options * options;
    struct handlebars_value * rv;
    struct handlebars_value * conditional;
    struct handlebars_value rv2;
    struct handlebars_value lambda_argv[1];
    struct handlebars_vm_call_checkpoint checkpoint;
};

static void handlebars_if_call_state_deinit(
    struct handlebars_if_call_state * state
)
{
    handlebars_value_dtor(&state->lambda_argv[0]);
    handlebars_value_dtor(&state->rv2);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void handlebars_builtin_if_guarded(
    struct handlebars_vm * vm,
    struct handlebars_if_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    struct handlebars_value * conditional;
    long program;
    struct handlebars_string * result_str = NULL;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    if (state->argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#if requires exactly one argument");
    }

    conditional = state->conditional;
    if( handlebars_value_is_callable(conditional) ) {
        const int argc2 = 1;
        handlebars_value_value(
            &state->lambda_argv[0],
            state->options->scope
        );
        conditional = handlebars_value_call(
            conditional,
            argc2,
            state->lambda_argv,
            state->options,
            vm,
            &state->rv2
        );
    }

    if( !handlebars_value_is_empty(conditional) ) {
        program = state->options->program;
    } else if( handlebars_value_get_type(conditional) == HANDLEBARS_VALUE_TYPE_INTEGER &&
            handlebars_value_get_intval(conditional) == 0 &&
            NULL != handlebars_value_map_str_find(
                state->options->hash,
                HBS_STRL("includeZero"),
                &state->rv2
            ) ) {
        program = state->options->program;
    } else {
        program = state->options->inverse;
    }

    result_str = handlebars_vm_execute_program(
        vm,
        program,
        state->options->scope
    );
    handlebars_value_str(state->rv, result_str);

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_if_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

struct handlebars_value * handlebars_builtin_if(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_if_call_state state = {0};

    state.argc = argc;
    state.options = options;
    state.rv = rv;
    state.conditional = argc > 0 ? argv : NULL;
    handlebars_builtin_if_guarded(vm, &state);

    return rv;
}

struct handlebars_value * handlebars_builtin_unless(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * conditional;

    if (argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#unless requires exactly one argument");
    }

    conditional = &argv[0];
    assert(conditional != NULL);

    handlebars_value_boolean(conditional, handlebars_value_is_empty(conditional));

    return handlebars_vm_call_helper_str(HBS_STRL("if"), HANDLEBARS_HELPER_ARGS_PASSTHRU);
}

struct handlebars_with_call_state {
    int argc;
    struct handlebars_options * options;
    struct handlebars_value * rv;
    struct handlebars_value * context;
    struct handlebars_value block_params;
    struct handlebars_value rv2;
    struct handlebars_value lambda_argv[1];
    struct handlebars_string * result;
    struct handlebars_vm_call_checkpoint checkpoint;
};

static void handlebars_with_call_state_deinit(
    struct handlebars_with_call_state * state
)
{
    if( state->result != NULL ) {
        handlebars_string_delref(state->result);
    }
    handlebars_value_dtor(&state->lambda_argv[0]);
    handlebars_value_dtor(&state->rv2);
    handlebars_value_dtor(&state->block_params);
}

HBS_ATTR_NOINLINE HBS_ATTR_NONNULL_ALL
static void handlebars_builtin_with_guarded(
    struct handlebars_vm * vm,
    struct handlebars_with_call_state * state
)
{
    struct handlebars_error * error = HBSCTX(vm)->e;
    jmp_buf * volatile prev_jmp = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(vm, &buf) ) {
        caught = error->num;
        goto done;
    }

    handlebars_vm_call_checkpoint_begin(vm, &state->checkpoint);

    if (state->argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#with requires exactly one argument");
    }

    if( handlebars_value_is_callable(state->context) ) {
        const int argc2 = 1;
        handlebars_value_value(
            &state->lambda_argv[0],
            state->options->scope
        );
        state->context = handlebars_value_call(
            state->context,
            argc2,
            state->lambda_argv,
            state->options,
            vm,
            &state->rv2
        );
    }

    assert(state->context != NULL);

    if( handlebars_value_get_type(state->context) == HANDLEBARS_VALUE_TYPE_NULL ) {
        state->result = handlebars_vm_execute_program(
            vm,
            state->options->inverse,
            state->context
        );
    } else {
        handlebars_value_array(
            &state->block_params,
            handlebars_stack_ctor(CONTEXT, 2)
        );
        handlebars_value_array_set(
            &state->block_params,
            0,
            state->context
        );

        state->result = handlebars_vm_execute_program_ex(
            vm,
            state->options->program,
            state->context,
            state->options->data,
            &state->block_params
        );
    }

    handlebars_value_str(state->rv, state->result);
    state->result = NULL;

done:
    error->jmp = prev_jmp;
    handlebars_vm_call_checkpoint_finish(vm, &state->checkpoint, caught);
    handlebars_with_call_state_deinit(state);

    if( caught != HANDLEBARS_SUCCESS ) {
        handlebars_vm_rethrow_caught(vm, prev_jmp, caught);
    }
}

struct handlebars_value * handlebars_builtin_with(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_with_call_state state = {0};

    state.argc = argc;
    state.options = options;
    state.rv = rv;
    state.context = argc > 0 ? argv : NULL;
    handlebars_builtin_with_guarded(vm, &state);

    return rv;
}

struct handlebars_value * handlebars_builtin_hbsc_set_delimiters(HANDLEBARS_HELPER_ARGS)
{
    if (argc == 2 && argv[0].type == HANDLEBARS_VALUE_TYPE_STRING && argv[1].type == HANDLEBARS_VALUE_TYPE_STRING) {
        struct handlebars_string * delim_open = handlebars_value_get_string(&argv[0]);
        struct handlebars_string * delim_close = handlebars_value_get_string(&argv[1]);

        handlebars_string_addref(delim_open);
        handlebars_string_addref(delim_close);
        if( vm->delim_open != NULL ) {
            handlebars_string_delref(vm->delim_open);
        }
        if( vm->delim_close != NULL ) {
            handlebars_string_delref(vm->delim_close);
        }
        vm->delim_open = delim_open;
        vm->delim_close = delim_close;
    }
    return rv;
}





static const char * names[] = {
    "blockHelperMissing",
    "helperMissing",
    "each",
    "if",
    "unless",
    "with",
    "log",
    "lookup",
    "hbsc_set_delimiters",
    NULL
};

static handlebars_helper_func builtins[] = {
    &handlebars_builtin_block_helper_missing,
    &handlebars_builtin_helper_missing,
    &handlebars_builtin_each,
    &handlebars_builtin_if,
    &handlebars_builtin_unless,
    &handlebars_builtin_with,
    &handlebars_builtin_log,
    &handlebars_builtin_lookup,
    &handlebars_builtin_hbsc_set_delimiters,
    NULL
};

const char ** handlebars_builtins_names(void)
{
    return names;
}

handlebars_helper_func * handlebars_builtins(void)
{
    return builtins;
}

handlebars_helper_func handlebars_builtins_find(const char * str, unsigned int len)
{
    const struct handlebars_builtin_pair * pair = hbs_builtin_lut_lookup(str, len);
    handlebars_helper_func fn = NULL;
    if( pair ) {
        fn = builtins[pair->pos];
    }
    return fn;
}

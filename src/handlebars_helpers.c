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

#include <assert.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_helpers_ht.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"



#define SAFE_RETURN(val) return val ? val : handlebars_value_ctor(CONTEXT)

#undef CONTEXT
#define CONTEXT HBSCTX(vm)

static inline struct handlebars_value * call_helper_str(const char * name, unsigned int len, int argc, struct handlebars_value * argv[], struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * result;
    handlebars_helper_func fn;
    if( NULL != (helper = handlebars_value_map_str_find(options->vm->helpers, name, len)) ) {
        result = handlebars_value_call(helper, argc, argv, options);
        handlebars_value_delref(helper);
        return result;
    } else if( NULL != (fn = handlebars_builtins_find(name, len)) ) {
        return fn(argc, argv, options);
    }
    return NULL;
}

#undef CONTEXT
#define CONTEXT HBSCTX(options->vm)

void handlebars_options_deinit(struct handlebars_options * options)
{
    //handlebars_value_try_delref(options->scope);
    handlebars_value_try_delref(options->data);
    handlebars_value_try_delref(options->hash);
    //handlebars_stack_dtor(options->params);
    //handlebars_talloc_free(options->name);
}

struct handlebars_value * handlebars_builtin_each(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * context;
    struct handlebars_value_iterator it;
    struct handlebars_value * result = NULL;
    short use_data;
    struct handlebars_value * data = NULL;
    struct handlebars_value * block_params = NULL;
    struct handlebars_string * tmp;
    size_t i = 0;
    long len;
    struct handlebars_value * ret = NULL;
    struct handlebars_value * key = NULL;
    struct handlebars_value * index = NULL;
    struct handlebars_value * first = NULL;
    struct handlebars_value * last = NULL;

    use_data = (options->data != NULL);

    if( argc < 1 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Must pass iterator to #each");
    }

    context = argv[0];
    result = handlebars_value_ctor(CONTEXT);

    if( handlebars_value_is_callable(context) ) {
        struct handlebars_options options2 = {0};
        int argc2 = 1;
        //struct handlebars_value * argv2[argc2];
        struct handlebars_value ** argv2 = alloca(sizeof(struct handlebars_value *) * argc2);
        options2.vm = options->vm;
        options2.scope = options->scope;
        argv2[0] = options->scope;
        ret = handlebars_value_call(context, argc2, argv2, &options2);
        if( !ret ) {
            handlebars_options_deinit(&options2);
            goto whoopsie;
        }
        //handlebars_value_delref(context); // @todo double-check
        context = ret;
        handlebars_options_deinit(&options2);
    }

    handlebars_value_string(result, "");

    if( handlebars_value_get_type(context) != HANDLEBARS_VALUE_TYPE_MAP && handlebars_value_get_type(context) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
        goto whoopsie;
    }

    key = handlebars_value_ctor(CONTEXT);
    block_params = handlebars_value_ctor(CONTEXT);
    handlebars_value_array_init(block_params);

    if( use_data ) {
        data = handlebars_value_ctor(CONTEXT);
        handlebars_value_map_init(data);
        if( handlebars_value_get_type(options->data) == HANDLEBARS_VALUE_TYPE_MAP ) {
            handlebars_value_iterator_init(&it, options->data);
            for (; it.current != NULL; it.next(&it)) {
                handlebars_map_update(data->v.map, it.key, it.current);
            }
        }
        index = handlebars_value_ctor(CONTEXT);
        first = handlebars_value_ctor(CONTEXT);
        last = handlebars_value_ctor(CONTEXT);
        handlebars_map_str_update(data->v.map, HBS_STRL("index"), index);
        handlebars_map_str_update(data->v.map, HBS_STRL("key"), key);
        handlebars_map_str_update(data->v.map, HBS_STRL("first"), first);
        handlebars_map_str_update(data->v.map, HBS_STRL("last"), last);
    }

    handlebars_value_iterator_init(&it, context);
    len = handlebars_value_count(context) - 1;

    for( ; it.current != NULL; it.next(&it) ) {
        if( it.current->type == HANDLEBARS_VALUE_TYPE_NULL ) {
            i++;
            continue;
        }

        if( it.key /*it->value->type == HANDLEBARS_VALUE_TYPE_MAP*/ ) {
            handlebars_value_str(key, it.key);
        } else {
            handlebars_value_integer(key, it.index);
        }

        if( use_data ) {
            if( it.index ) { // @todo zero?
                handlebars_value_integer(index, it.index);
            } else {
                handlebars_value_integer(index, i);
            }
            handlebars_value_boolean(first, i == 0);
            handlebars_value_boolean(last, i == len);
        }

        handlebars_stack_set(block_params->v.stack, 0, it.current);
        handlebars_stack_set(block_params->v.stack, 1, key);

        tmp = handlebars_vm_execute_program_ex(options->vm, options->program, it.current, data, block_params);
        result->v.string = handlebars_string_append(HBSCTX(options->vm), result->v.string, HBS_STR_STRL(tmp));

        i++;
    }

whoopsie:
    if( i == 0 ) {
        tmp = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
        result->v.string = handlebars_string_append(HBSCTX(options->vm), result->v.string, HBS_STR_STRL(tmp));
    }

    //handlebars_value_try_delref(context);  // @todo double-check
    handlebars_value_try_delref(data);
    if( data ) {
        handlebars_value_try_delref(key);
        handlebars_value_try_delref(index);
        handlebars_value_try_delref(first);
        handlebars_value_try_delref(last);
        handlebars_value_try_delref(block_params);
    }

    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_block_helper_missing(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * context;
    struct handlebars_string * result = NULL;
    bool is_zero;
    struct handlebars_value * ret = NULL;

    if( argc < 1 ) {
        goto inverse;
    }

    context = argv[0];
    is_zero = handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_INTEGER && handlebars_value_get_intval(context) == 0;

    if( context->type == HANDLEBARS_VALUE_TYPE_TRUE ) {
        result = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    } else if( handlebars_value_is_empty(context) && !is_zero ) {
inverse:
        result = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
    } else if( handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        ret = call_helper_str(HBS_STRL("each"), argc, argv, options);
        goto done;
    } else {
        // For object, etc
        result = handlebars_vm_execute_program(options->vm, options->program, context);
    }

    ret = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(ret, result);

    //handlebars_value_try_delref(context); // @todo double-check

done:
    SAFE_RETURN(ret);
}

struct handlebars_value * handlebars_builtin_helper_missing(HANDLEBARS_HELPER_ARGS)
{
    if( argc != 0 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Missing helper: \"%s\"", options->name->val);
    }
    SAFE_RETURN(NULL);
}

struct handlebars_value * handlebars_builtin_log(HANDLEBARS_HELPER_ARGS)
{
    if( options->vm->log_func ) {
        options->vm->log_func(argc, argv, options);
    } else {
        int i;
        fprintf(stderr, "[INFO] ");
        for (i = 0; i < argc; i++) {
            char *tmp = handlebars_value_dump(argv[i], 0);
            fprintf(stderr, "%s ", tmp);
            handlebars_talloc_free(tmp);
        }
        fprintf(stderr, "\n");
        //fflush(stderr);
    }
    SAFE_RETURN(NULL);
}

struct handlebars_value * handlebars_builtin_lookup(HANDLEBARS_HELPER_ARGS)
{
    if( argc < 2 ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "lookup requires two parameters");
    }

    struct handlebars_value * context = argv[0];
    struct handlebars_value * field = argv[1];
    struct handlebars_value * result = NULL;
    enum handlebars_value_type type = handlebars_value_get_type(context);

    if( type == HANDLEBARS_VALUE_TYPE_MAP ) {
        struct handlebars_string * key = handlebars_value_to_string(field);
        result = handlebars_value_map_find(context, key);
        handlebars_talloc_free(key);
    } else if( type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        // @todo sscanf?
        if( field->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
            result = handlebars_value_array_find(context, handlebars_value_get_intval(field));
        }
    }

    //handlebars_value_try_delref(context); // @todo double-check
    //handlebars_value_try_delref(field); // @todo double-check

    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_if(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * conditional = argv[0];
    long program;
    struct handlebars_value * tmp = NULL;
    struct handlebars_value * ret = NULL;
    struct handlebars_string * result;

    if (argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#if requires exactly one argument");
    }

    if( handlebars_value_is_callable(conditional) ) {
        struct handlebars_options options2 = {0};
        int argc2 = 1;
        //struct handlebars_value * argv2[argc2];
        struct handlebars_value ** argv2 = alloca(sizeof(struct handlebars_value *) * argc2);
        options2.vm = options->vm;
        options2.scope = options->scope;
        argv2[0] = options->scope;
        ret = handlebars_value_call(conditional, argc2, argv2, &options2);
        //handlebars_value_delref(conditional); // @todo double-check
        conditional = ret;
        if( !conditional ) {
            conditional = handlebars_value_ctor(CONTEXT);
        }
        ret = NULL;
        handlebars_options_deinit(&options2);
    }

    if( !handlebars_value_is_empty(conditional) ) {
        program = options->program;
    } else if( conditional->type == HANDLEBARS_VALUE_TYPE_INTEGER &&
            handlebars_value_get_intval(conditional) == 0 &&
            NULL != (tmp = handlebars_value_map_str_find(options->hash, HBS_STRL("includeZero"))) ) {
        program = options->program;
        handlebars_value_delref(tmp);
    } else {
        program = options->inverse;
    }

    result = handlebars_vm_execute_program(options->vm, program, options->scope);
    ret = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(ret, result);

    SAFE_RETURN(ret);
}

struct handlebars_value * handlebars_builtin_unless(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * conditional;
    struct handlebars_value * result;

    if (argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#unless requires exactly one argument");
    }

    conditional = argv[0];

    handlebars_value_boolean(conditional, conditional ? handlebars_value_is_empty(conditional) : true);

    result = call_helper_str(HBS_STRL("if"), argc, argv, options);
    //handlebars_value_delref(conditional); // @todo double-check
    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_with(HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_string * result = NULL;
    struct handlebars_value * context = argv[0];
    struct handlebars_value * block_params = NULL;
    struct handlebars_value * ret = NULL;

    if (argc != 1) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "#with requires exactly one argument");
    }

    if( handlebars_value_is_callable(context) ) {
        struct handlebars_options options2 = {0};
        int argc2 = 1;
        //struct handlebars_value * argv2[argc2];
        struct handlebars_value ** argv2 = alloca(sizeof(struct handlebars_value *) * argc2);
        argv2[0] = options->scope;
        ret = handlebars_value_call(context, argc2, argv2, options);
        if( !ret ) {
            ret = handlebars_value_ctor(CONTEXT);
        }
        //handlebars_value_delref(context); // @todo double-check
        context = ret;
        ret = NULL;
        handlebars_options_deinit(&options2);
    }

    if( context == NULL ) {
        context = handlebars_value_ctor(CONTEXT);
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
    } else if( context->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
    } else {
        block_params = handlebars_value_ctor(CONTEXT);
        handlebars_value_array_init(block_params);
        handlebars_stack_set(block_params->v.stack, 0, context);

        result = handlebars_vm_execute_program_ex(options->vm, options->program, context, options->data, block_params);
    }

    ret = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(ret, result);

    //handlebars_value_delref(context); // @todo double-check
    handlebars_value_try_delref(block_params);

    SAFE_RETURN(ret);
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
    const struct handlebars_builtin_pair * pair = in_word_set(str, len);
    handlebars_helper_func fn = NULL;
    if( pair ) {
        fn = builtins[pair->pos];
    }
    return fn;
}

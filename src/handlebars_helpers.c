
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "handlebars_context.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(cond) \
    do { \
        if( unlikely(!cond) ) { \
            handlebars_context_throw(options->vm->ctx, HANDLEBARS_NOMEM, "Out of memory  [" __S2(__FILE__) ":" __S2(__LINE__) "]"); \
        } \
    } while(0)



static const char * names[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};

const char ** handlebars_builtins_names(void)
{
    return names;
}

struct handlebars_value * handlebars_builtin_block_helper_missing(struct handlebars_options * options)
{
    struct handlebars_value * context;
    char * result = NULL;
    bool is_zero;
    struct handlebars_value * ret = NULL;

    assert(handlebars_stack_length(options->params) >= 1);

    context = handlebars_stack_get(options->params, 0);
    is_zero = handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_INTEGER && handlebars_value_get_intval(context) == 0;

    if( handlebars_value_get_boolval(context) ) {
        result = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    } else if( handlebars_value_is_empty(context) && !is_zero ) {
        result = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
    } else if( handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        // @todo use hash lookup
        return handlebars_builtin_each(options);
    } else {
        // For object, etc
        result = handlebars_vm_execute_program(options->vm, options->program, context);
    }

    if( result ) {
        ret = handlebars_value_ctor(options->vm);
        ret->type = HANDLEBARS_VALUE_TYPE_STRING;
        ret->v.strval = talloc_steal(ret, result);
    }

    return ret;
}

struct handlebars_value * handlebars_builtin_each(struct handlebars_options * options)
{
    struct handlebars_value * context;
    struct handlebars_value_iterator * it;
    struct handlebars_value * result;
    short use_data;
    use_data = (options->data != NULL);
    struct handlebars_value * data = NULL;
    struct handlebars_value * block_params = NULL;
    char * tmp;
    size_t i = 0;
    size_t len;
    struct handlebars_options * options2;
    struct handlebars_value * ret;

    if( handlebars_stack_length(options->params) < 1 ) {
        handlebars_vm_throw(options->vm, HANDLEBARS_ERROR, "Must pass iterator to #each");
    }

    context = handlebars_stack_get(options->params, 0);
    result = handlebars_value_ctor(options->vm);

    if( context->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        options2 = handlebars_talloc_zero(options->vm, struct handlebars_options);
        __MEMCHECK(options2);
        options2->params = handlebars_stack_ctor(options);
        handlebars_stack_push(options2->params, options->scope);
        ret = context->v.helper(options);
        if( !ret ) {
            goto whoopsie;
        }
        handlebars_value_delref(context);
        context = ret;
    }

    handlebars_value_string(result, "");

    if( use_data ) {
        data = handlebars_value_ctor(options->vm);
        data->type = HANDLEBARS_VALUE_TYPE_MAP;
        data->v.map = handlebars_map_ctor(data);
        if( handlebars_value_get_type(options->data) == HANDLEBARS_VALUE_TYPE_MAP ) {
            struct handlebars_value_iterator *it2 = handlebars_value_iterator_ctor(options->data);
            for (; it2->current != NULL; handlebars_value_iterator_next(it2)) {
                handlebars_map_add(data->v.map, it2->key, it2->current);
            }
            handlebars_talloc_free(it2);
        }
    }

    if( context->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        len = context->v.map->i - 1;
    } else if( context->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        len = handlebars_stack_length(context->v.stack) - 1;
    } else {
        goto whoopsie;
    }

    for( it = handlebars_value_iterator_ctor(context) ; it->current != NULL; handlebars_value_iterator_next(it) ) {
        if( context->type == HANDLEBARS_VALUE_TYPE_NULL ) {
            i++;
            continue;
        }

        struct handlebars_value * key = handlebars_value_ctor(options->vm);
        if( it->value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
            handlebars_value_string(key, it->key);
        } else {
            handlebars_value_integer(key, it->index);
        }

        if( data ) {
            struct handlebars_value * index = handlebars_value_ctor(options->vm);
            struct handlebars_value * first = handlebars_value_ctor(options->vm);
            struct handlebars_value * last = handlebars_value_ctor(options->vm);
            if( it->value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
                handlebars_value_integer(index, i);
            } else {
                handlebars_value_integer(index, it->index);
            }
            handlebars_value_boolean(first, i == 0);
            handlebars_value_boolean(last, i == len);

            handlebars_map_update(data->v.map, "index", index);
            handlebars_map_update(data->v.map, "key", key);
            handlebars_map_update(data->v.map, "first", first);
            handlebars_map_update(data->v.map, "last", last);
            handlebars_value_delref(index);
            handlebars_value_delref(first);
            handlebars_value_delref(last);
        }

        block_params = handlebars_value_ctor(options->vm);
        block_params->type = HANDLEBARS_VALUE_TYPE_ARRAY;
        block_params->v.stack = handlebars_stack_ctor(block_params);
        handlebars_stack_set(block_params->v.stack, 0, it->current);
        handlebars_stack_set(block_params->v.stack, 1, key);

        tmp = handlebars_vm_execute_program_ex(options->vm, options->program, it->current, data, block_params);
        if( tmp ) {
            result->v.strval = handlebars_talloc_strdup_append(result->v.strval, tmp);
        }

        handlebars_value_delref(key);
        handlebars_value_delref(block_params);

        i++;
    }

whoopsie:
    if( i == 0 ) {
        tmp = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
        if( tmp ) {
            result->v.strval = handlebars_talloc_strdup_append(result->v.strval, tmp);
        }
    }

    return result;
}

struct handlebars_value * handlebars_builtin_helper_missing(struct handlebars_options * options)
{
    if( handlebars_stack_length(options->params) != 0 ) {
        char * msg = handlebars_talloc_asprintf(options->vm, "Missing helper: \"%s\"", options->name);
        handlebars_vm_throw(options->vm, HANDLEBARS_ERROR, msg);
    }
    return NULL;
}

struct handlebars_value * handlebars_builtin_lookup(struct handlebars_options * options)
{
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);
    struct handlebars_value * field = handlebars_stack_get(options->params, 1);

    if( context->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        const char * tmp = handlebars_value_get_strval(field);
        return handlebars_value_map_find(context, tmp);
    } else if( context->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        // @todo sscanf?
        if( field->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
            return handlebars_value_array_find(context, handlebars_value_get_intval(field));
        }
    }

    return NULL;
}

struct handlebars_value * handlebars_builtin_if(struct handlebars_options * options)
{
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);
    int program;
    struct handlebars_value * tmp = NULL;

    if( conditional->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        struct handlebars_options * options2 = handlebars_talloc_zero(options->vm, struct handlebars_options);
        options2->params = handlebars_stack_ctor(options);
        handlebars_stack_push(options2->params, options->scope);
        struct handlebars_value * ret = conditional->v.helper(options);
        handlebars_value_delref(conditional);
        conditional = ret;
        if( !conditional ) {
            conditional = handlebars_value_ctor(options->vm);
        }
    }

    if( !handlebars_value_is_empty(conditional) ) {
        program = options->program;
    } else if( conditional->type == HANDLEBARS_VALUE_TYPE_INTEGER &&
            handlebars_value_get_intval(conditional) == 0 &&
            NULL != (tmp = handlebars_value_map_find(options->hash, "includeZero")) ) {
        program = options->program;
        handlebars_value_delref(tmp);
    } else {
        program = options->inverse;
    }

    char * result = handlebars_vm_execute_program(options->vm, program, options->scope);

    if( result ) {
        struct handlebars_value * ret = handlebars_value_ctor(options->vm);
        ret->type = HANDLEBARS_VALUE_TYPE_STRING;
        ret->v.strval = talloc_steal(ret, result);
        return ret;
    }

    return NULL;
}

struct handlebars_value * handlebars_builtin_unless(struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);

    handlebars_value_boolean(conditional, handlebars_value_is_empty(conditional));

    helper = handlebars_value_map_find(options->vm->helpers, "if");
    assert(helper != NULL);
    return helper->v.helper(options);
}

struct handlebars_value * handlebars_builtin_with(struct handlebars_options * options)
{
    char * result = NULL;
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);
    struct handlebars_value * block_params;
    struct handlebars_value * ret;

    if( context->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        struct handlebars_options * options2 = handlebars_talloc_zero(options->vm, struct handlebars_options);
        options2->params = handlebars_stack_ctor(options);
        handlebars_stack_push(options2->params, options->scope);
        ret = context->v.helper(options);
        if( !ret ) {
            ret = handlebars_value_ctor(options->vm);
        }
        handlebars_value_delref(context);
        context = ret;
    }

    if( context == NULL ) {
        context = handlebars_value_ctor(options->vm);
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
    } else if( context->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
    } else {
        block_params = handlebars_value_ctor(options->vm);
        block_params->type = HANDLEBARS_VALUE_TYPE_ARRAY;
        block_params->v.stack = handlebars_stack_ctor(block_params);
        handlebars_stack_set(block_params->v.stack, 0, context);

        result = handlebars_vm_execute_program_ex(options->vm, options->program, context, options->data, block_params);
    }

    handlebars_value_delref(context);

    if( result ) {
        ret = handlebars_value_ctor(options->vm);
        ret->type = HANDLEBARS_VALUE_TYPE_STRING;
        ret->v.strval = talloc_steal(ret, result);
        return ret;
    }

    return NULL;
}

struct handlebars_value * handlebars_builtins(void * ctx)
{
#define ADDHELPER(name, func) do { \
        tmp = handlebars_value_ctor(ctx); \
        if( unlikely(tmp == NULL) ) { \
            map = NULL; \
            goto error; \
        } \
        tmp->type = HANDLEBARS_VALUE_TYPE_HELPER; \
        tmp->v.helper = func; \
        if( unlikely(!handlebars_map_add(map, #name, tmp)) ) { \
            map = NULL; \
            goto error; \
        } \
    } while(0)

    struct handlebars_value * value;
    struct handlebars_map * map;
    struct handlebars_value * tmp;

    value = handlebars_value_ctor(ctx);
    if( unlikely(value == NULL) ) {
        return NULL;
    }

    map = handlebars_map_ctor(value);
    if( unlikely(map == NULL) ) {
        goto error;
    }

    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = map;

    ADDHELPER(blockHelperMissing, handlebars_builtin_block_helper_missing);
    ADDHELPER(each, handlebars_builtin_each);
    ADDHELPER(helperMissing, handlebars_builtin_helper_missing);
    ADDHELPER(if, handlebars_builtin_if);
    ADDHELPER(lookup, handlebars_builtin_lookup);
    ADDHELPER(unless, handlebars_builtin_unless);
    ADDHELPER(with, handlebars_builtin_with);

    return value;
error:
    handlebars_value_delref(value);
    return NULL;
#undef ADDHELPER
}
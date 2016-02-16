
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



#define CONTEXT options->vm->ctx
#define SAFE_RETURN(val) return val ? val : handlebars_value_ctor(CONTEXT)

static const char * names[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};

const char ** handlebars_builtins_names(void)
{
    return names;
}

static inline struct handlebars_value * get_helper(struct handlebars_vm * vm, const char * name)
{
    struct handlebars_value * helper;
    helper = handlebars_value_map_find(vm->helpers, name);
    if( !helper ) {
        if( !vm->builtins ) {
            vm->builtins = handlebars_builtins(vm->ctx);
        }
        helper = handlebars_value_map_find(vm->builtins, name);
    }
    return helper;
}



void handlebars_options_dtor(struct handlebars_options * options)
{
    //handlebars_value_try_delref(options->scope);
    handlebars_value_try_delref(options->data);
    handlebars_value_try_delref(options->hash);
    handlebars_stack_dtor(options->params);
    handlebars_talloc_free(options);
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
        struct handlebars_value * each = get_helper(options->vm, "each");
        ret = handlebars_value_call(each, options);
    } else {
        // For object, etc
        result = handlebars_vm_execute_program(options->vm, options->program, context);
    }

    if( result ) {
        ret = handlebars_value_ctor(CONTEXT);
        handlebars_value_string_steal(ret, result);
    }

    handlebars_value_try_delref(context);

    SAFE_RETURN(ret);
}

struct handlebars_value * handlebars_builtin_each(struct handlebars_options * options)
{
    struct handlebars_value * context;
    struct handlebars_value_iterator * it;
    struct handlebars_value * result = NULL;
    short use_data;
    use_data = (options->data != NULL);
    struct handlebars_value * data = NULL;
    struct handlebars_value * block_params = NULL;
    char * tmp;
    size_t i = 0;
    size_t len;
    struct handlebars_options * options2;
    struct handlebars_value * ret = NULL;

    if( handlebars_stack_length(options->params) < 1 ) {
        handlebars_vm_throw(options->vm, HANDLEBARS_ERROR, "Must pass iterator to #each");
    }

    context = handlebars_stack_get(options->params, 0);
    result = handlebars_value_ctor(CONTEXT);

    if( handlebars_value_is_callable(context) ) {
        options2 = MC(handlebars_talloc_zero(options->vm, struct handlebars_options));
        options2->params = talloc_steal(options2, handlebars_stack_ctor(CONTEXT));
        options2->vm = options->vm;
        options2->scope = options->scope;
        handlebars_stack_push(options2->params, options->scope);
        ret = handlebars_value_call(context, options2);
        if( !ret ) {
            handlebars_options_dtor(options2);
            goto whoopsie;
        }
        handlebars_value_delref(context);
        context = ret;
        handlebars_options_dtor(options2);
    }

    handlebars_value_string(result, "");

    if( use_data ) {
        data = handlebars_value_ctor(CONTEXT);
        handlebars_value_map_init(data);
        if( handlebars_value_get_type(options->data) == HANDLEBARS_VALUE_TYPE_MAP ) {
            struct handlebars_value_iterator *it2 = handlebars_value_iterator_ctor(options->data);
            for (; it2->current != NULL; handlebars_value_iterator_next(it2)) {
                handlebars_map_add(data->v.map, it2->key, it2->current);
            }
            handlebars_talloc_free(it2);
        }
    }

    if( handlebars_value_get_type(context) != HANDLEBARS_VALUE_TYPE_MAP && handlebars_value_get_type(context) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
        goto whoopsie;
    }

    it = handlebars_value_iterator_ctor(context);
    len = handlebars_value_count(context) - 1;

    for( ; it->current != NULL; handlebars_value_iterator_next(it) ) {
        struct handlebars_value * key;

        if( it->current->type == HANDLEBARS_VALUE_TYPE_NULL ) {
            i++;
            continue;
        }

        key = handlebars_value_ctor(CONTEXT);
        if( it->key /*it->value->type == HANDLEBARS_VALUE_TYPE_MAP*/ ) {
            handlebars_value_string(key, it->key);
        } else {
            handlebars_value_integer(key, it->index);
        }

        if( data ) {
            struct handlebars_value * index = handlebars_value_ctor(CONTEXT);
            struct handlebars_value * first = handlebars_value_ctor(CONTEXT);
            struct handlebars_value * last = handlebars_value_ctor(CONTEXT);
            if( it->index ) { // @todo zero?
                handlebars_value_integer(index, it->index);
            } else {
                handlebars_value_integer(index, i);
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

        block_params = handlebars_value_ctor(CONTEXT);
        handlebars_value_array_init(block_params);
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
    handlebars_talloc_free(it);

whoopsie:
    if( i == 0 ) {
        tmp = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
        if( tmp ) {
            result->v.strval = handlebars_talloc_strdup_append(result->v.strval, tmp);
        }
    }

    handlebars_value_try_delref(context);
    handlebars_value_try_delref(data);

    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_helper_missing(struct handlebars_options * options)
{
    if( handlebars_stack_length(options->params) != 0 ) {
        char * msg = handlebars_talloc_asprintf(options->vm, "Missing helper: \"%s\"", options->name);
        handlebars_vm_throw(options->vm, HANDLEBARS_ERROR, msg);
    }
    SAFE_RETURN(NULL);
}

struct handlebars_value * handlebars_builtin_lookup(struct handlebars_options * options)
{
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);
    struct handlebars_value * field = handlebars_stack_get(options->params, 1);
    struct handlebars_value * result = NULL;
    enum handlebars_value_type type = handlebars_value_get_type(context);

    if( type == HANDLEBARS_VALUE_TYPE_MAP ) {
        const char * tmp = handlebars_value_get_strval(field);
        result = handlebars_value_map_find(context, tmp);
    } else if( type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        // @todo sscanf?
        if( field->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
            result = handlebars_value_array_find(context, handlebars_value_get_intval(field));
        }
    }

    handlebars_value_try_delref(context);
    handlebars_value_try_delref(field);

    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_if(struct handlebars_options * options)
{
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);
    int program;
    struct handlebars_value * tmp = NULL;
    struct handlebars_value * ret = NULL;
    char * result;

    if( handlebars_value_is_callable(conditional) ) {
        struct handlebars_options * options2 = handlebars_talloc_zero(options->vm, struct handlebars_options);
        options2->params = talloc_steal(options2, handlebars_stack_ctor(CONTEXT));
        options2->vm = options->vm;
        options2->scope = options->scope;
        handlebars_stack_push(options2->params, options->scope);
        ret = handlebars_value_call(conditional, options2);
        handlebars_value_delref(conditional);
        conditional = ret;
        if( !conditional ) {
            conditional = handlebars_value_ctor(CONTEXT);
        }
        ret = NULL;
        handlebars_options_dtor(options2);
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

    result = handlebars_vm_execute_program(options->vm, program, options->scope);

    if( result ) {
        ret = handlebars_value_ctor(CONTEXT);
        handlebars_value_string_steal(ret, result);
    }

    SAFE_RETURN(ret);
}

struct handlebars_value * handlebars_builtin_unless(struct handlebars_options * options)
{
    struct handlebars_value * helper;
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);
    struct handlebars_value * result = NULL;

    if( !conditional ) {
        conditional = handlebars_value_ctor(CONTEXT);
    }
    handlebars_value_boolean(conditional, handlebars_value_is_empty(conditional));

    helper = get_helper(options->vm, "if");
    assert(helper != NULL);
    result = handlebars_value_call(helper, options);

    handlebars_value_delref(helper);
    handlebars_value_delref(conditional);

    SAFE_RETURN(result);
}

struct handlebars_value * handlebars_builtin_with(struct handlebars_options * options)
{
    char * result = NULL;
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);
    struct handlebars_value * block_params = NULL;
    struct handlebars_value * ret = NULL;

    if( handlebars_value_is_callable(context) ) {
        struct handlebars_options * options2 = handlebars_talloc_zero(options->vm, struct handlebars_options);
        options2->params = talloc_steal(options2, handlebars_stack_ctor(CONTEXT));
        handlebars_stack_push(options2->params, options->scope);
        ret = handlebars_value_call(context, options);
        if( !ret ) {
            ret = handlebars_value_ctor(CONTEXT);
        }
        handlebars_value_delref(context);
        context = ret;
        ret = NULL;
        handlebars_options_dtor(options2);
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

    if( result ) {
        ret = handlebars_value_ctor(CONTEXT);
        handlebars_value_string_steal(ret, result);
    }

    handlebars_value_delref(context);
    handlebars_value_try_delref(block_params);

    SAFE_RETURN(ret);
}

struct handlebars_value * handlebars_builtins(struct handlebars_context * ctx)
{
#define ADDHELPER(name, func) do { \
        tmp = handlebars_value_ctor(ctx); \
        tmp->type = HANDLEBARS_VALUE_TYPE_HELPER; \
        tmp->v.helper = func; \
        handlebars_map_add(value->v.map, #name, tmp); \
        handlebars_value_delref(tmp); \
    } while(0)

    struct handlebars_value * value;
    struct handlebars_value * tmp;

    assert(ctx != NULL);

    value = handlebars_value_ctor(ctx);
    handlebars_value_map_init(value);

    ADDHELPER(blockHelperMissing, handlebars_builtin_block_helper_missing);
    ADDHELPER(each, handlebars_builtin_each);
    ADDHELPER(helperMissing, handlebars_builtin_helper_missing);
    ADDHELPER(if, handlebars_builtin_if);
    ADDHELPER(lookup, handlebars_builtin_lookup);
    ADDHELPER(unless, handlebars_builtin_unless);
    ADDHELPER(with, handlebars_builtin_with);

    return value;
#undef ADDHELPER
}
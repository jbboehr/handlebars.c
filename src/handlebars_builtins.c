
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "handlebars_builtins.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

static const char * names[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};

const char ** handlebars_builtins_names()
{
    return names;
}

struct handlebars_value * handlebars_builtin_block_helper_missing(struct handlebars_options * options)
{
    char * result = NULL;

    assert(handlebars_stack_length(options->params) >= 1);

    struct handlebars_value * context = handlebars_stack_get(options->params, 0);

    if( handlebars_value_get_boolval(context) == 1 ) {
        result = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    } else if( handlebars_value_is_empty(context) ) { // @todo supposed to be !== 0 ?
        result = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
    } else if( handlebars_value_get_type(context) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        // @todo
    } else {
        // @todo
    }

    if( result ) {
        struct handlebars_value * ret = handlebars_value_ctor(options->vm);
        ret->type = HANDLEBARS_VALUE_TYPE_STRING;
        ret->v.strval = talloc_steal(ret, result);
        return ret;
    }

    return NULL;
}

struct handlebars_value * handlebars_builtin_each(struct handlebars_options * options)
{
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);

    // @todo

    return NULL;
}

struct handlebars_value * handlebars_builtin_if(struct handlebars_options * options)
{
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);

    // @todo callable conditional

    int program = handlebars_value_is_empty(conditional) ? options->inverse : options->program;

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
    struct handlebars_value * conditional = handlebars_stack_get(options->params, 0);

    handlebars_value_boolean(conditional, handlebars_value_is_empty(conditional));

    struct handlebars_value * helper = handlebars_map_find(options->vm->helpers, "if");
    assert(helper != NULL);
    return helper->v.helper(options);
}

struct handlebars_value * handlebars_builtin_with(struct handlebars_options * options)
{
    // @todo callable
    char * result = NULL;
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);

    if( context == NULL ) {
        context = handlebars_value_ctor(options->vm);
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
        handlebars_value_delref(context);
    } else if( context->type == HANDLEBARS_VALUE_TYPE_NULL ) {
        result = handlebars_vm_execute_program(options->vm, options->inverse, context);
    } else {
        result = handlebars_vm_execute_program(options->vm, options->program, context);
    }

    if( result ) {
        struct handlebars_value * ret = handlebars_value_ctor(options->vm);
        ret->type = HANDLEBARS_VALUE_TYPE_STRING;
        ret->v.strval = talloc_steal(ret, result);
        return ret;
    }

    return NULL;
}

struct handlebars_map * handlebars_builtins(void * ctx)
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

    TALLOC_CTX * tmpctx;
    struct handlebars_map * map;
    struct handlebars_value * tmp;

    tmpctx = talloc_init(ctx);
    if( unlikely(tmpctx) == NULL ) {
        return NULL;
    }

    map = handlebars_map_ctor(tmpctx);
    if( unlikely(map == NULL) ) {
        return NULL;
    }

    ADDHELPER(blockHelperMissing, handlebars_builtin_block_helper_missing);
    ADDHELPER(each, handlebars_builtin_each);
    ADDHELPER(if, handlebars_builtin_if);
    ADDHELPER(unless, handlebars_builtin_unless);
    ADDHELPER(with, handlebars_builtin_with);

    talloc_steal(ctx, map);

error:
    handlebars_talloc_free(tmpctx);
    return map;
#undef ADDHELPER
}
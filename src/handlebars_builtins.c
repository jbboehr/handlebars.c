
#include "handlebars_builtins.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"

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
    return NULL;
}

struct handlebars_value * handlebars_builtin_each(struct handlebars_options * options)
{
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

    talloc_steal(ctx, map);

error:
    handlebars_talloc_free(tmpctx);
    return map;
#undef ADDHELPER
}
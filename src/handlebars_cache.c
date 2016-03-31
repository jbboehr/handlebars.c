
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <lmdb.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"


void handlebars_cache_dtor(struct handlebars_cache * cache)
{
    handlebars_talloc_free(cache);
}
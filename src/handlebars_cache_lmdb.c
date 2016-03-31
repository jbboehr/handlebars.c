
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

#define HANDLE_RC(err) if( err != 0 && err != MDB_NOTFOUND ) handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "%s", mdb_strerror(err));



#undef CONTEXT
#define CONTEXT HBSCTX(cache)

static void cache_dtor(struct handlebars_cache * cache)
{
    mdb_env_close(cache->u.env);
}

static int cache_gc(struct handlebars_cache * cache)
{
    return 0;
}

static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    char tmp[256];
    struct handlebars_module * module;

    err = mdb_txn_begin(cache->u.env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_open(txn, NULL, 0, &dbi);
    if( err != 0 ) {
        goto error;
    }

    // Make key
    if( tmpl->len > mdb_env_get_maxkeysize(cache->u.env) ) {
        unsigned long hash = HBS_STR_HASH(tmpl);
        snprintf(tmp, 256, "hash:%lu", hash);
        key.mv_size = strlen(tmp);
        key.mv_data = tmp;
    } else {
        key.mv_size = tmpl->len + 1;
        key.mv_data = tmpl->val;
    }

    // Fetch data
    err = mdb_get(txn, dbi, &key, &data);
    if( err != 0 ) {
        goto error;
    }

    // Duplicate data
    size_t size = ((struct handlebars_module *) data.mv_data)->size;
    module = handlebars_talloc_size(cache, size);
    memcpy(module, data.mv_data, size);

    // Convert pointers
    // @todo

    return module;

error:
    mdb_txn_abort(txn);
    HANDLE_RC(err);
    return NULL;
}

static void cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
) {
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    char tmp[256];

    err = mdb_txn_begin(cache->u.env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_open(txn, NULL, 0, &dbi);
    if( err != 0 ) {
        goto error;
    }

    // Make key
    if( tmpl->len > mdb_env_get_maxkeysize(cache->u.env) ) {
        unsigned long hash = HBS_STR_HASH(tmpl);
        snprintf(tmp, 256, "hash:%lu", hash);
        key.mv_size = strlen(tmp);
        key.mv_data = tmp;
    } else {
        key.mv_size = tmpl->len + 1;
        key.mv_data = tmpl->val;
    }

    // Make data
    data.mv_size = module->size;
    data.mv_data = module;

    // Store
    err = mdb_put(txn, dbi, &key, &data, 0);
    if( err != 0 ) {
        goto error;
    }

    // Commit
    err = mdb_txn_commit(txn);
    if( err != 0 ) {
        goto error;
    }

    return;

error:
    mdb_txn_abort(txn);
    HANDLE_RC(err);
}

#undef CONTEXT
#define CONTEXT context

struct handlebars_cache * handlebars_cache_lmdb_ctor(
    struct handlebars_context * context,
    const char * path
) {
    struct handlebars_cache * cache = MC(handlebars_talloc_zero(context, struct handlebars_cache));
    cache->u.map = talloc_steal(cache, handlebars_map_ctor(context));
    cache->u.map->ctx = HBSCTX(cache);
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;

    mdb_env_create(&cache->u.env);
    talloc_set_destructor(cache, cache_dtor);

    int err = mdb_env_open(cache->u.env, ".", MDB_WRITEMAP | MDB_MAPASYNC, 0644);
    HANDLE_RC(err);

    return cache;
}

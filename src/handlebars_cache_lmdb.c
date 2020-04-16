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

#if HAVE_LIBLMDB
#include <lmdb.h>
#endif

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_cache.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_opcode_serializer.h"



#if HAVE_LIBLMDB

#define HANDLE_RC(err) if( err != 0 && err != MDB_NOTFOUND ) handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "%s", mdb_strerror(err));

struct handlebars_cache_lmdb {
    MDB_env * env;
    struct handlebars_cache_stat stat;
};


#undef CONTEXT
#define CONTEXT HBSCTX(cache)

static int cache_dtor(struct handlebars_cache * cache)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    mdb_env_close(intern->env);
    return 0;
}

static int cache_gc(struct handlebars_cache * cache)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    MDB_cursor *cursor;
    MDB_stat stat;
    time_t now;

    time(&now);

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    err = mdb_stat(txn, dbi, &stat);
    if( err != 0 ) goto error;

    err = mdb_cursor_open(txn, dbi, &cursor);
    if( err != 0 ) goto error;

    while( (err = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0 ) {
        /* fprintf(stderr, "key: %p %.*s, data: %p %.*s\n",
                key.mv_data,  (int) key.mv_size,  (char *) key.mv_data,
                data.mv_data, (int) data.mv_size, (char *) data.mv_data); */

        struct handlebars_module * module = (struct handlebars_module *) data.mv_data;
        if( cache->max_age >= 0 && difftime(now, module->ts) > cache->max_age ) {
            mdb_del(txn, dbi, &key, NULL);
        }
    }

    mdb_cursor_close(cursor);
error:
    mdb_txn_abort(txn);
    return 0;
}

static struct handlebars_module * cache_find(struct handlebars_cache * cache, struct handlebars_string * tmpl)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    char tmp[256];
    struct handlebars_module * module;
    time_t now;

    time(&now);

    err = mdb_txn_begin(intern->env, NULL, MDB_RDONLY, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    if( tmpl->len > mdb_env_get_maxkeysize(intern->env) ) {
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
    if( err == MDB_NOTFOUND ) {
        intern->stat.misses++;
        mdb_txn_abort(txn);
        return NULL;
    }
    if( err != 0 ) goto error;

    module = ((struct handlebars_module *) data.mv_data);

    // Check if it's too old or wrong version
    if( module->version != handlebars_version() || (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) ) {
        intern->stat.misses++;
        goto error;
    }

    intern->stat.hits++;

    // Duplicate data
    size_t size = module->size;
    module = handlebars_talloc_size(cache, size);
    memcpy(module, data.mv_data, size);

    // Close
    mdb_txn_abort(txn);

    // Convert pointers
    handlebars_module_patch_pointers(module);

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
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    char tmp[256];

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    if( tmpl->len > mdb_env_get_maxkeysize(intern->env) ) {
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
    if( err != 0 ) goto error;

    // Commit
    err = mdb_txn_commit(txn);
    if( err != 0 ) goto error;

    return;

error:
    mdb_txn_abort(txn);
    HANDLE_RC(err);
}

static void cache_release(struct handlebars_cache * cache, struct handlebars_string * tmpl, struct handlebars_module * module)
{
    handlebars_talloc_free(module);
}

static struct handlebars_cache_stat cache_stat(struct handlebars_cache * cache)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_stat stat;

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    err = mdb_stat(txn, dbi, &stat);
    if( err != 0 ) goto error;

    intern->stat.name = "mmap";
    intern->stat.current_entries = stat.ms_entries;

error:
    mdb_txn_abort(txn);
    HANDLE_RC(err);
    return intern->stat;
}

static void cache_reset(struct handlebars_cache * cache)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    err = mdb_drop(txn, dbi, 0);
    if( err != 0 ) goto error;

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
    handlebars_context_bind(context, HBSCTX(cache));

    cache->max_age = -1;
    cache->add = &cache_add;
    cache->find = &cache_find;
    cache->gc = &cache_gc;
    cache->release = &cache_release;
    cache->stat = &cache_stat;
    cache->reset = &cache_reset;

    struct handlebars_cache_lmdb * intern = MC(handlebars_talloc_zero(context, struct handlebars_cache_lmdb));
    cache->internal = intern;

    mdb_env_create(&intern->env);
    talloc_set_destructor(cache, cache_dtor);

    int err = mdb_env_open(intern->env, path, MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOSUBDIR, 0644);
    HANDLE_RC(err);

    return cache;
}

#else

struct handlebars_cache * handlebars_cache_lmdb_ctor(
    struct handlebars_context * context,
    const char * path
) {
    handlebars_throw(context, HANDLEBARS_ERROR, "lmdb not available");
}

#endif

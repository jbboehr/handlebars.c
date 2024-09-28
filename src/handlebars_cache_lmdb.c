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
#include <time.h>

#ifdef HANDLEBARS_HAVE_LMDB
#include <lmdb.h>
#endif

#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_cache_private.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_value.h"



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
    if (intern->env) {
        mdb_env_close(intern->env);
        intern->env = NULL;
    }
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
    HANDLE_RC(err);
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
    size_t size;

    time(&now);

    err = mdb_txn_begin(intern->env, NULL, MDB_RDONLY, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    if( hbs_str_len(tmpl) > (size_t) mdb_env_get_maxkeysize(intern->env) ) {
        snprintf(tmp, 256, "hash:%lu", (unsigned long) hbs_str_hash(tmpl));
        key.mv_size = strlen(tmp);
        key.mv_data = tmp;
    } else {
        key.mv_size = hbs_str_len(tmpl) + 1;
        key.mv_data = hbs_str_val(tmpl);
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

#if defined(HANDLEBARS_ENABLE_DEBUG)
    // In debug mode, throw
    handlebars_module_verify(module, CONTEXT);
#else
    // In release mode, consider a failed hash/version match a miss
    if (!handlebars_module_verify(module, NULL)) {
        intern->stat.misses++;
        goto error;
    }
#endif

    // Check if it's too old
    if (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) {
        intern->stat.misses++;
        goto error;
    }

    intern->stat.hits++;

    // Duplicate data
    size = module->size;
    module = handlebars_talloc_size(cache, size);
    talloc_set_type(module, struct handlebars_module);
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
    struct handlebars_module * module_copy;

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    if( hbs_str_len(tmpl) > (size_t) mdb_env_get_maxkeysize(intern->env) ) {
        snprintf(tmp, 256, "hash:%lu", (unsigned long) hbs_str_hash(tmpl));
        key.mv_size = strlen(tmp);
        key.mv_data = tmp;
    } else {
        key.mv_size = hbs_str_len(tmpl) + 1;
        key.mv_data = hbs_str_val(tmpl);
    }

    // Normalize pointers
    module_copy = handlebars_talloc_size(CONTEXT, module->size);
    talloc_set_type(module_copy, struct handlebars_module);
    memcpy(module_copy, module, module->size);
    handlebars_module_patch_pointers(module_copy);
    handlebars_module_normalize_pointers(module_copy, (void *) 0);
    handlebars_module_generate_hash(module_copy);

    // Make data
    data.mv_size = module_copy->size;
    data.mv_data = module_copy;

    // Store
    err = mdb_put(txn, dbi, &key, &data, 0);
    handlebars_talloc_free(module_copy);
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

static const struct handlebars_cache_handlers hbs_cache_handlers_lmdb = {
    &cache_add,
    &cache_find,
    &cache_gc,
    &cache_release,
    &cache_stat,
    &cache_reset
};

struct handlebars_cache * handlebars_cache_lmdb_ctor(
    struct handlebars_context * context,
    const char * path
) {
    struct handlebars_cache * cache = handlebars_talloc_zero_size(context, sizeof(struct handlebars_cache) + sizeof(struct handlebars_cache_lmdb));
    HANDLEBARS_MEMCHECK(cache, context);
    talloc_set_type(cache, struct handlebars_cache);
    handlebars_context_bind(context, HBSCTX(cache));

    cache->max_age = -1;
    cache->hnd = &hbs_cache_handlers_lmdb;

    struct handlebars_cache_lmdb * intern = (void *) ((char *) cache + sizeof(struct handlebars_cache));
    cache->internal = intern;

    mdb_env_create(&intern->env);
    talloc_set_destructor(cache, cache_dtor);

    int err = mdb_env_open(intern->env, path, MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOSUBDIR, 0644);
    HANDLE_RC(err);

    return cache;
}

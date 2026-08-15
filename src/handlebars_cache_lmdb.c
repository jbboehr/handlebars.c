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

enum cache_copy_module_result {
    cache_copy_module_valid,
    cache_copy_module_invalid,
    cache_copy_module_nomem
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

static enum cache_copy_module_result cache_copy_module(
    struct handlebars_cache * cache,
    const MDB_val * data,
    struct handlebars_module ** result
) {
    struct handlebars_module * module;
    size_t module_size;

    *result = NULL;

    if( data->mv_size < sizeof(struct handlebars_module) ) {
        return cache_copy_module_invalid;
    }

    /* LMDB only promises byte-addressable value storage. Read the claimed
     * size without first casting the database memory to an aligned module. */
    memcpy(
        &module_size,
        (const unsigned char *) data->mv_data + offsetof(struct handlebars_module, size),
        sizeof(module_size)
    );
    if( module_size != data->mv_size ) {
        return cache_copy_module_invalid;
    }

    module = handlebars_talloc_size(cache, module_size);
    if( module == NULL ) {
        return cache_copy_module_nomem;
    }
    talloc_set_type(module, struct handlebars_module);
    memcpy(module, data->mv_data, module_size);

    if( !handlebars_module_verify_ex(module, module_size, NULL) ) {
        handlebars_talloc_free(module);
        return cache_copy_module_invalid;
    }

    *result = module;
    return cache_copy_module_valid;
}

static int cache_gc(struct handlebars_cache * cache)
{
    struct handlebars_cache_lmdb * intern = (struct handlebars_cache_lmdb *) cache->internal;
    int err;
    MDB_txn *txn;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val data;
    MDB_cursor *cursor = NULL;
    MDB_stat stat;
    int removed = 0;
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

        struct handlebars_module * module;
        enum cache_copy_module_result copy_result = cache_copy_module(cache, &data, &module);
        if( copy_result == cache_copy_module_nomem ) {
            goto nomem;
        }
        if( copy_result == cache_copy_module_invalid
                || (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) ) {
            err = mdb_cursor_del(cursor, 0);
            if( err != 0 ) {
                handlebars_talloc_free(module);
                break;
            }
            removed++;
        }
        handlebars_talloc_free(module);
    }

    if( err == MDB_NOTFOUND ) {
        err = 0;
    }
    if( err != 0 ) {
        goto error;
    }

    mdb_cursor_close(cursor);
    cursor = NULL;
    err = mdb_txn_commit(txn);
    HANDLE_RC(err);
    return removed;

error:
    if( cursor != NULL ) {
        mdb_cursor_close(cursor);
    }
    mdb_txn_abort(txn);
    HANDLE_RC(err);
    return 0;

nomem:
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);
    handlebars_throw(CONTEXT, HANDLEBARS_NOMEM, HANDLEBARS_MEMCHECK_MSG);
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
    struct handlebars_module * module = NULL;
    enum cache_copy_module_result copy_result;
    time_t now;

    time(&now);

    if( hbs_str_len(tmpl) >= (size_t) mdb_env_get_maxkeysize(intern->env) ) {
        intern->stat.misses++;
        return NULL;
    }

    err = mdb_txn_begin(intern->env, NULL, MDB_RDONLY, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    key.mv_size = hbs_str_len(tmpl) + 1;
    key.mv_data = hbs_str_val(tmpl);

    // Fetch data
    err = mdb_get(txn, dbi, &key, &data);
    if( err == MDB_NOTFOUND ) {
        intern->stat.misses++;
        mdb_txn_abort(txn);
        return NULL;
    }
    if( err != 0 ) goto error;

    copy_result = cache_copy_module(cache, &data, &module);
    if( copy_result == cache_copy_module_nomem ) {
        goto nomem;
    }
    if( copy_result == cache_copy_module_invalid ) {
        intern->stat.misses++;
        goto error;
    }

    // Check if it's too old
    if (cache->max_age >= 0 && difftime(now, module->ts) >= cache->max_age) {
        intern->stat.misses++;
        goto error;
    }

    intern->stat.hits++;

    // Close
    mdb_txn_abort(txn);

    // Convert pointers
    handlebars_module_patch_pointers(module);

    return module;

error:
    mdb_txn_abort(txn);
    handlebars_talloc_free(module);
    HANDLE_RC(err);
    return NULL;

nomem:
    mdb_txn_abort(txn);
    handlebars_throw(CONTEXT, HANDLEBARS_NOMEM, HANDLEBARS_MEMCHECK_MSG);
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
    struct handlebars_module * module_copy;

    // LMDB cannot store keys beyond this limit. Do not reduce long templates
    // to a non-unique hash key, since that can return another template's module.
    if( hbs_str_len(tmpl) >= (size_t) mdb_env_get_maxkeysize(intern->env) ) {
        return;
    }

    err = mdb_txn_begin(intern->env, NULL, 0, &txn);
    HANDLE_RC(err);

    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) goto error;

    // Make key
    key.mv_size = hbs_str_len(tmpl) + 1;
    key.mv_data = hbs_str_val(tmpl);

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

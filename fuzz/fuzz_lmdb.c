/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lmdb.h>

#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_cache_private.h"
#include "handlebars_compiler.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"

#define HANDLEBARS_FUZZ_MAX_INPUT_SIZE (64 * 1024)
#define HANDLEBARS_FUZZ_MAX_OPERATIONS 256
#define HANDLEBARS_FUZZ_LMDB_PATH "/tmp/handlebars-fuzz-lmdb-XXXXXX"

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

static void handlebars_fuzz_lmdb_cleanup(
    struct handlebars_context * context,
    const char * path
) {
    char lock_path[sizeof(HANDLEBARS_FUZZ_LMDB_PATH) + sizeof("-lock")];

    if( context != NULL ) {
        handlebars_context_dtor(context);
    }
    unlink(path);
    snprintf(lock_path, sizeof(lock_path), "%s-lock", path);
    unlink(lock_path);
}

static bool handlebars_fuzz_lmdb_put_raw(
    const char * path,
    const char * key_string,
    const uint8_t * bytes,
    size_t size
) {
    MDB_env * env = NULL;
    MDB_txn * txn = NULL;
    MDB_dbi dbi;
    MDB_val key;
    MDB_val value;
    int err;

    err = mdb_env_create(&env);
    if( err != 0 ) {
        goto done;
    }
    err = mdb_env_open(env, path, MDB_WRITEMAP | MDB_MAPASYNC | MDB_NOSUBDIR, 0644);
    if( err != 0 ) {
        goto done;
    }
    err = mdb_txn_begin(env, NULL, 0, &txn);
    if( err != 0 ) {
        goto done;
    }
    err = mdb_dbi_open(txn, NULL, MDB_CREATE, &dbi);
    if( err != 0 ) {
        goto done;
    }

    key.mv_size = strlen(key_string) + 1;
    key.mv_data = (void *) key_string;
    value.mv_size = size;
    value.mv_data = size > 0 ? (void *) bytes : NULL;
    err = mdb_put(txn, dbi, &key, &value, 0);
    if( err != 0 ) {
        goto done;
    }

    {
        MDB_txn * committing = txn;
        txn = NULL;
        err = mdb_txn_commit(committing);
    }

done:
    if( txn != NULL ) {
        mdb_txn_abort(txn);
    }
    if( env != NULL ) {
        mdb_env_close(env);
    }
    return err == 0;
}

static struct handlebars_module * handlebars_fuzz_lmdb_make_module(
    struct handlebars_context * context
) {
    struct handlebars_parser * parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * compiler = handlebars_compiler_ctor(context);
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{foo}}"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    struct handlebars_module * module = handlebars_program_serialize(context, program);

    handlebars_module_generate_hash(module);
    return module;
}

static void handlebars_fuzz_lmdb_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
) {
    struct handlebars_module * module = handlebars_cache_find(cache, key);

    if( module != NULL ) {
        handlebars_cache_release(cache, key, module);
    }
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    static const char * valid_key_names[] = {
        "valid-0",
        "valid-1",
        "valid-2"
    };
    const char raw_key_name[] = "raw";
    char path[] = HANDLEBARS_FUZZ_LMDB_PATH;
    struct handlebars_context * context;
    struct handlebars_cache * cache;
    struct handlebars_module * module;
    struct handlebars_string * raw_key;
    struct handlebars_string * valid_keys[3];
    size_t operation_count;
    int fd;
    jmp_buf buf;

    if( size > HANDLEBARS_FUZZ_MAX_INPUT_SIZE ) {
        return 0;
    }

    fd = mkstemp(path);
    if( fd < 0 ) {
        return 0;
    }
    close(fd);
    if( unlink(path) != 0 ) {
        return 0;
    }

    context = handlebars_context_ctor();
    if( context == NULL ) {
        handlebars_fuzz_lmdb_cleanup(NULL, path);
        return 0;
    }

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_fuzz_lmdb_cleanup(context, path);
        return 0;
    }

    if( !handlebars_fuzz_lmdb_put_raw(path, raw_key_name, data, size) ) {
        handlebars_fuzz_lmdb_cleanup(context, path);
        return 0;
    }

    module = handlebars_fuzz_lmdb_make_module(context);
    raw_key = handlebars_string_ctor(context, raw_key_name, strlen(raw_key_name));
    for( size_t i = 0; i < 3; i++ ) {
        valid_keys[i] = handlebars_string_ctor(
            context,
            valid_key_names[i],
            strlen(valid_key_names[i])
        );
    }
    cache = handlebars_cache_lmdb_ctor(context, path);
    operation_count = size < HANDLEBARS_FUZZ_MAX_OPERATIONS
        ? size
        : HANDLEBARS_FUZZ_MAX_OPERATIONS;

    for( size_t i = 0; i < operation_count; i++ ) {
        struct handlebars_string * valid_key = valid_keys[(data[i] >> 3) % 3];

        switch( data[i] & 7 ) {
            case 0:
                handlebars_fuzz_lmdb_find(cache, raw_key);
                break;
            case 1:
                handlebars_cache_add(cache, valid_key, module);
                break;
            case 2:
                handlebars_fuzz_lmdb_find(cache, valid_key);
                break;
            case 3:
                cache->max_age = data[i] & 0x10 ? 0 : -1;
                (void) handlebars_cache_gc(cache);
                break;
            case 4:
                handlebars_cache_reset(cache);
                break;
            case 5: {
                struct handlebars_cache_stat stat = handlebars_cache_stat(cache);
                (void) stat.current_entries;
                break;
            }
            case 6:
                handlebars_cache_dtor(cache);
                cache = handlebars_cache_lmdb_ctor(context, path);
                break;
            case 7:
                handlebars_cache_dtor(cache);
                if( !handlebars_fuzz_lmdb_put_raw(path, raw_key_name, data, size) ) {
                    handlebars_fuzz_lmdb_cleanup(context, path);
                    return 0;
                }
                cache = handlebars_cache_lmdb_ctor(context, path);
                break;
            default:
                break;
        }
    }

    handlebars_fuzz_lmdb_cleanup(context, path);
    return 0;
}

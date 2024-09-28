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

#ifndef HANDLEBARS_CACHE_PRIVATE_H
#define HANDLEBARS_CACHE_PRIVATE_H

#include "handlebars.h"

HBS_EXTERN_C_START

typedef void (*handlebars_cache_add_func)(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
);

typedef struct handlebars_module * (*handlebars_cache_find_func)(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl
);

typedef int (*handlebars_cache_gc_func)(
    struct handlebars_cache * cache
);

typedef void (*handlebars_cache_release_func)(
        struct handlebars_cache * cache,
        struct handlebars_string * tmpl,
        struct handlebars_module * module
);

typedef struct handlebars_cache_stat (*handlebars_cache_stat_func)(
    struct handlebars_cache * cache
);

typedef void (*handlebars_cache_reset_func)(
    struct handlebars_cache * cache
);

struct handlebars_cache_handlers {
    handlebars_cache_add_func add;
    handlebars_cache_find_func find;
    handlebars_cache_gc_func gc;
    handlebars_cache_release_func release;
    handlebars_cache_stat_func stat;
    handlebars_cache_reset_func reset;
};

/**
 * @brief In-memory opcode cache.
 */
struct handlebars_cache {
    //! Common header
    struct handlebars_context ctx;

    //! Opaque pointer for implementation use
    void * internal;

    // @TODO should we just go back to embedding? saves one dereference
    const struct handlebars_cache_handlers * hnd;

    //! The max amount of time to keep an entry, in seconds, or zero to disable
    double max_age;

    //! The max number of entries to keep, or zero to disable
    size_t max_entries;

    //! The max size of all entries, or zero to disable
    size_t max_size;
};

#endif /* HANDLEBARS_CACHE_PRIVATE_H */

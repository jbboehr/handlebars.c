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

#define HANDLEBARS_CACHE_IMPL
#define HANDLEBARS_CACHE_PRIVATE

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"



void handlebars_cache_dtor(struct handlebars_cache * cache)
{
    handlebars_talloc_free(cache);
}

extern inline struct handlebars_module * handlebars_cache_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
);

extern inline void handlebars_cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
);

extern inline int handlebars_cache_gc(struct handlebars_cache * cache);

extern inline void handlebars_cache_reset(struct handlebars_cache * cache);

extern inline struct handlebars_cache_stat handlebars_cache_stat(struct handlebars_cache * cache);

extern inline void handlebars_cache_dtor(struct handlebars_cache * cache);

extern inline void handlebars_cache_release(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
);

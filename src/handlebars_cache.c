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

#include "handlebars.h"
#include "handlebars_cache.h"
#include "handlebars_cache_private.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"



const size_t HANDLEBARS_CACHE_SIZE = sizeof(struct handlebars_cache);

void handlebars_cache_dtor(struct handlebars_cache * cache)
{
    handlebars_talloc_free(cache);
}

struct handlebars_cache_stat handlebars_cache_stat(struct handlebars_cache * cache)
{
    return cache->hnd->stat(cache);
}

struct handlebars_module * handlebars_cache_find(
    struct handlebars_cache * cache,
    struct handlebars_string * key
) {
    return cache->hnd->find(cache, key);
}

void handlebars_cache_add(
    struct handlebars_cache * cache,
    struct handlebars_string * tmpl,
    struct handlebars_module * module
) {
    cache->hnd->add(cache, tmpl, module);
}

int handlebars_cache_gc(struct handlebars_cache * cache)
{
    return cache->hnd->gc(cache);
}

void handlebars_cache_reset(struct handlebars_cache * cache)
{
    cache->hnd->reset(cache);
}

void handlebars_cache_release(
    struct handlebars_cache * cache,
    struct handlebars_string * key,
    struct handlebars_module * module
) {
    cache->hnd->release(cache, key, module);
}

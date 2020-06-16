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
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_map.h"
#include "handlebars_partial_loader.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif

#define GET_INTERN_V(value) GET_INTERN(handlebars_value_get_user(value))
#define GET_INTERN(user) ((struct handlebars_partial_loader *) talloc_get_type_abort(user, struct handlebars_partial_loader))



struct handlebars_partial_loader {
    struct handlebars_user user;
    struct handlebars_string *base_path;
    struct handlebars_string *extension;
    struct handlebars_map * map;
};

static int partial_loader_dtor(struct handlebars_partial_loader * intern)
{
    // When this gets run, the map has been already freed by talloc it appears
    // if (intern->map) {
    //     handlebars_map_delref(intern->map);
    //     intern->map = NULL;
    // }

    return 0;
}

static struct handlebars_value * hbs_partial_loader_copy(struct handlebars_value * value)
{
    return NULL;
}

static void hbs_partial_loader_dtor(struct handlebars_user * user)
{
    struct handlebars_partial_loader * intern = GET_INTERN(user);
    if (intern->map) {
        handlebars_map_delref(intern->map);
        intern->map = NULL;
    }
}

static enum handlebars_value_type hbs_partial_loader_type(struct handlebars_value * value)
{
    return HANDLEBARS_VALUE_TYPE_MAP;
}

static struct handlebars_value * hbs_partial_loader_map_find(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * rv)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_value *retval = handlebars_map_find(intern->map, key);

    if (retval) {
        handlebars_value_value(rv, retval);
        return rv;
    }

    struct handlebars_string *filename = handlebars_string_copy_ctor(intern->user.ctx, intern->base_path);
    filename = handlebars_string_append(intern->user.ctx, filename, HBS_STRL("/"));
    filename = handlebars_string_append_str(intern->user.ctx, filename, key);
    if (intern->extension) {
        filename = handlebars_string_append_str(intern->user.ctx, filename, intern->extension);
    }

    FILE * f;
    long size;

    f = fopen(hbs_str_val(filename), "rb");
    if( !f ) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "File to open partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char * buf = handlebars_talloc_array(intern->user.ctx, char, size + 1);
    size_t read = fread(buf, size, 1, f);
    fclose(f);

    if (!read) {
        handlebars_throw(intern->user.ctx, HANDLEBARS_ERROR, "Failed to read partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    buf[size] = 0;

    // Need to duplicate the key because it may be owned by a child VM
    key = handlebars_string_copy_ctor(intern->user.ctx, key);

    handlebars_value_str(rv, handlebars_string_ctor(intern->user.ctx, buf, size));
    handlebars_talloc_free(buf);

    intern->map = handlebars_map_add(intern->map, key, rv);

    handlebars_talloc_free(filename);
    return rv;
}

static bool hbs_partial_loader_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool hbs_partial_loader_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(it->value);
    struct handlebars_map * map = intern->map;
    struct handlebars_value * tmp;

    assert(it->value != NULL);
    assert(handlebars_value_get_type(it->value) == HANDLEBARS_VALUE_TYPE_MAP);

    if( it->index >= handlebars_map_count(map) - 1 ) {
        handlebars_value_dtor(it->cur);
        handlebars_map_set_is_in_iteration(map, false);
        return false;
    }

    it->index++;
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &tmp);
    handlebars_value_value(it->cur, tmp);
    return true;
}

static bool hbs_partial_loader_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_map * map = intern->map;
    struct handlebars_value * tmp;

    if (handlebars_map_count(map) <= 0) {
        it->next = &hbs_partial_loader_iterator_next_void;
        return false;
    }

    handlebars_map_sparse_array_compact(map); // meh
    it->value = value;
    it->index = 0;
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &tmp);
    handlebars_value_value(it->cur, tmp);
    it->next = &hbs_partial_loader_iterator_next_map;
    if (handlebars_map_set_is_in_iteration(map, true)) {
        fprintf(stderr, "Nested map iteration is not currently supported [%s:%d]", __FILE__, __LINE__);
        abort();
    }

    return true;
}

static long hbs_partial_loader_count(struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    return handlebars_map_count(intern->map);
}

static const struct handlebars_value_handlers handlebars_value_hbs_partial_loader_handlers = {
    "json",
    &hbs_partial_loader_copy,
    &hbs_partial_loader_dtor,
    NULL, // convert
    &hbs_partial_loader_type,
    &hbs_partial_loader_map_find,
    NULL, // array_find
    &hbs_partial_loader_iterator_init,
    NULL, // call
    &hbs_partial_loader_count
};

struct handlebars_value * handlebars_value_partial_loader_init(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * rv
) {
    struct handlebars_partial_loader *obj = MC(handlebars_talloc(context, struct handlebars_partial_loader));
    handlebars_user_init((struct handlebars_user *) obj, context, &handlebars_value_hbs_partial_loader_handlers);
    obj->base_path = talloc_steal(obj, handlebars_string_copy_ctor(context, base_path));
    obj->extension = talloc_steal(obj, handlebars_string_copy_ctor(context, extension));
    obj->map = talloc_steal(obj, handlebars_map_ctor(context, 32));
    talloc_set_destructor(obj, partial_loader_dtor);
    handlebars_value_user(rv, (struct handlebars_user *) obj);
    return rv;
}

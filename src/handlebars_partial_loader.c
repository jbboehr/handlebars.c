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
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



struct handlebars_partial_loader {
    struct handlebars_user usr;
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


#define GET_INTERN_V(value) GET_INTERN(handlebars_value_get_user(value))
#define GET_INTERN(user) ((struct handlebars_partial_loader *) talloc_get_type_abort(user, struct handlebars_partial_loader))

#undef CONTEXT
#define CONTEXT HBSCTX(handlebars_value_get_ctx(value))

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

static void hbs_partial_loader_convert(struct handlebars_value * value, bool recurse)
{
    ;
}

static enum handlebars_value_type hbs_partial_loader_type(struct handlebars_value * value)
{
    return HANDLEBARS_VALUE_TYPE_MAP;
}

static struct handlebars_value * hbs_partial_loader_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_value *retval = handlebars_map_find(intern->map, key);

    if (retval) {
        return retval;
    }

    struct handlebars_string *filename = handlebars_string_copy_ctor(CONTEXT, intern->base_path);
    filename = handlebars_string_append(CONTEXT, filename, HBS_STRL("/"));
    filename = handlebars_string_append_str(CONTEXT, filename, key);
    if (intern->extension) {
        filename = handlebars_string_append_str(CONTEXT, filename, intern->extension);
    }

    FILE * f;
    long size;

    f = fopen(hbs_str_val(filename), "rb");
    if( !f ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "File to open partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char * buf = handlebars_talloc_array(CONTEXT, char, size);
    size_t read = fread(buf, size, 1, f);
    fclose(f);

    if (!read) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to read partial: %.*s", (int) hbs_str_len(filename), hbs_str_val(filename));
    }

    buf[size - 1] = 0;

    retval = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(retval, handlebars_string_ctor(CONTEXT, buf, size - 1));
    handlebars_talloc_free(buf);

    handlebars_map_add(intern->map, key, retval);

    handlebars_talloc_free(filename);
    return retval;
}

static struct handlebars_value * hbs_partial_loader_array_find(struct handlebars_value * value, size_t index)
{
    return NULL;
}

static bool hbs_partial_loader_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool hbs_partial_loader_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(it->value);
    struct handlebars_map * map = intern->map;

    assert(it->value != NULL);
    assert(handlebars_value_get_type(it->value) == HANDLEBARS_VALUE_TYPE_MAP);
    assert(it->current != NULL);

    it->current = NULL;

    if( it->index >= handlebars_map_count(map) - 1 ) {
        handlebars_map_set_is_in_iteration(map, false); // @todo we should restore the previous flag?
        return false;
    }

    it->index++;
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &it->current);
    return true;
}

static bool hbs_partial_loader_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN_V(value);
    struct handlebars_map * map = intern->map;

    if (handlebars_map_count(map) <= 0) {
        it->next = &hbs_partial_loader_iterator_next_void;
        return false;
    }

    handlebars_map_sparse_array_compact(map); // meh
    it->value = value;
    it->index = 0;
    it->length = handlebars_map_count(map);
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &it->current);
    it->next = &hbs_partial_loader_iterator_next_map;
    handlebars_map_set_is_in_iteration(map, true); // @todo we should store the result

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
    &hbs_partial_loader_convert,
    &hbs_partial_loader_type,
    &hbs_partial_loader_map_find,
    &hbs_partial_loader_array_find,
    &hbs_partial_loader_iterator_init,
    NULL, // call
    &hbs_partial_loader_count
};

struct handlebars_value * handlebars_value_partial_loader_ctor(
    struct handlebars_context * context,
    struct handlebars_string *base_path,
    struct handlebars_string *extension
) {
    struct handlebars_value *value = handlebars_value_ctor(context);

    struct handlebars_partial_loader *obj = MC(handlebars_talloc(context, struct handlebars_partial_loader));
    handlebars_user_init((struct handlebars_user *) obj, &handlebars_value_hbs_partial_loader_handlers);
    obj->base_path = talloc_steal(obj, handlebars_string_copy_ctor(context, base_path));
    obj->extension = talloc_steal(obj, handlebars_string_copy_ctor(context, extension));
    obj->map = talloc_steal(obj, handlebars_map_ctor(context, 32));
    talloc_set_destructor(obj, partial_loader_dtor);
    handlebars_value_user(value, (struct handlebars_user *) obj);

    return value;
}

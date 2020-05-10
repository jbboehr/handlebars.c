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

#define HANDLEBARS_STRING_PRIVATE
#define HANDLEBARS_VALUE_HANDLERS_PRIVATE
#define HANDLEBARS_VALUE_PRIVATE

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



struct handlebars_partial_loader {
    struct handlebars_user usr;
    struct handlebars_string *base_path;
    struct handlebars_string *extension;
    struct handlebars_map * map;
};


static int partial_loader_dtor(struct handlebars_partial_loader * obj)
{
    // @TODO FIXME
    // I believe this may not be working due to the map not being allocated
    // directly under the partial loader

    // if( obj && obj->map ) {
    //     handlebars_map_dtor(obj->map);
    //     obj->map = NULL;
    // }

    return 0;
}


#define GET_INTERN(value) ((struct handlebars_partial_loader *) talloc_get_type(value->v.usr, struct handlebars_partial_loader))

#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)

static struct handlebars_value * std_partial_loader_copy(struct handlebars_value * value)
{
    return NULL;
}

static void std_partial_loader_dtor(struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN(value);
    handlebars_talloc_free(intern);
}

static void std_partial_loader_convert(struct handlebars_value * value, bool recurse)
{
    ;
}

static enum handlebars_value_type std_partial_loader_type(struct handlebars_value * value)
{
    return HANDLEBARS_VALUE_TYPE_MAP;
}

static struct handlebars_value * std_partial_loader_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    struct handlebars_partial_loader * intern = GET_INTERN(value);
    struct handlebars_value *retval = handlebars_map_find(intern->map, key);

    if (retval) {
        return retval;
    }

    struct handlebars_string *filename = handlebars_string_copy_ctor(value->ctx, intern->base_path);
    filename = handlebars_string_append(CONTEXT, filename, HBS_STRL("/"));
    filename = handlebars_string_append_str(CONTEXT, filename, key);
    if (intern->extension) {
        filename = handlebars_string_append_str(CONTEXT, filename, intern->extension);
    }

    FILE * f;
    long size;

    f = fopen(filename->val, "rb");
    if( !f ) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "File to open partial: %s", filename->val);
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    struct handlebars_string *buf = handlebars_string_init(CONTEXT, size);
    size_t read = fread(buf->val, size, 1, f);
    fclose(f);

    if (!read) {
        handlebars_throw(CONTEXT, HANDLEBARS_ERROR, "Failed to read partial: %s", filename->val);
    }

    buf->val[size] = 0;
    buf->len = size - 1;

    retval = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(retval, buf);

    handlebars_map_add(intern->map, key, retval);

    handlebars_talloc_free(filename);
    return retval;
}

static struct handlebars_value * std_partial_loader_array_find(struct handlebars_value * value, size_t index)
{
    return NULL;
}

static bool std_partial_loader_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool std_partial_loader_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_partial_loader * intern = GET_INTERN(it->value);
    struct handlebars_map * map = intern->map;

    assert(it->value != NULL);
    assert(it->value->type == HANDLEBARS_VALUE_TYPE_MAP);
    assert(it->current != NULL);

    handlebars_value_delref(it->current);
    it->current = NULL;

    if( it->index >= handlebars_map_count(map) - 1 ) {
        handlebars_map_set_is_in_iteration(map, false); // @todo we should restore the previous flag?
        return false;
    }

    it->index++;
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &it->current);
    return true;
}

bool std_partial_loader_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN(value);
    struct handlebars_map * map = intern->map;

    if (handlebars_map_count(map) <= 0) {
        it->next = &std_partial_loader_iterator_next_void;
        return false;
    }

    handlebars_map_sparse_array_compact(map); // meh
    it->value = value;
    it->index = 0;
    it->length = handlebars_map_count(map);
    handlebars_map_get_kv_at_index(map, it->index, &it->key, &it->current);
    it->next = &std_partial_loader_iterator_next_map;
    handlebars_value_addref(it->current);
    handlebars_map_set_is_in_iteration(map, true); // @todo we should store the result

    return true;
}

long std_partial_loader_count(struct handlebars_value * value)
{
    struct handlebars_partial_loader * intern = GET_INTERN(value);
    return handlebars_map_count(intern->map);
}

static struct handlebars_value_handlers handlebars_value_std_partial_loader_handlers = {
    "json",
    &std_partial_loader_copy,
    &std_partial_loader_dtor,
    &std_partial_loader_convert,
    &std_partial_loader_type,
    &std_partial_loader_map_find,
    &std_partial_loader_array_find,
    &std_partial_loader_iterator_init,
    NULL, // call
    &std_partial_loader_count
};

struct handlebars_value_handlers * handlebars_value_get_std_partial_loader_handlers()
{
    return &handlebars_value_std_partial_loader_handlers;
}

struct handlebars_value * handlebars_value_partial_loader_ctor(
    struct handlebars_context * context,
    struct handlebars_string *base_path,
    struct handlebars_string *extension
) {
    struct handlebars_value *value = handlebars_value_ctor(context);

    struct handlebars_partial_loader *obj = MC(handlebars_talloc(context, struct handlebars_partial_loader));
    obj->usr.handlers = handlebars_value_get_std_partial_loader_handlers();
    obj->base_path = talloc_steal(obj, handlebars_string_copy_ctor(context, base_path));
    obj->extension = talloc_steal(obj, handlebars_string_copy_ctor(context, extension));
    obj->map = talloc_steal(obj, handlebars_map_ctor(context, 32));
    talloc_set_destructor(obj, partial_loader_dtor);

    value->type = HANDLEBARS_VALUE_TYPE_USER;
    value->v.usr = (struct handlebars_user *) obj;

    return value;
}

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

/* Exercise the installed-header path for compilers without VLA support. */
#ifndef __STDC_NO_VLA__
#define __STDC_NO_VLA__ 1
#endif

/* HAVE_ALLOCA_H is intentionally absent from installed handlebars_config.h. */
#ifdef HAVE_ALLOCA_H
#undef HAVE_ALLOCA_H
#endif

#include <stdio.h>

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_ptr.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

static size_t count_map_entries(struct handlebars_map * _hbs_map_iterator)
{
    size_t count = 0;

    handlebars_map_foreach(_hbs_map_iterator, entry_index, key, value) {
        (void) entry_index;
        (void) key;
        (void) value;
        count++;
    } handlebars_map_foreach_end(_hbs_map_iterator);

    return count;
}

int main(void)
{
    struct handlebars_context * context = handlebars_context_ctor();
    const struct handlebars_string * string = handlebars_string_ctor(context, HBS_STRL("api"));
    const char * bytes = hbs_str_val(string);
    struct handlebars_ptr * ptr;
    struct handlebars_map * map;
    struct handlebars_string * map_key = NULL;
    struct handlebars_value * map_value = NULL;
    void * pointer_result = NULL;
    int payload = 7;
    int result = 0;
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_ITERATOR_DECL(iterator);
    HANDLEBARS_MAP_ITERATOR_DECL(map_iterator);

    if( hbs_str_len(string) != 3 || bytes[0] != 'a' ) {
        result = 3;
    }

    ptr = handlebars_ptr_ctor(context, int, &payload, true);
    if( !handlebars_ptr_try_get(ptr, "int", &pointer_result) ||
        pointer_result != &payload ) {
        result = 5;
    }
    handlebars_value_ptr(value, ptr);
    if( !handlebars_value_ptr_try_get(value, "int", &pointer_result) ||
        pointer_result != &payload ) {
        result = 6;
    }

    handlebars_value_array(value, handlebars_stack_ctor(context, 1));
    handlebars_value_integer(child, 42);
    if( handlebars_value_array_push_try(value, child) != HANDLEBARS_SUCCESS ) {
        result = 4;
    }

    if( !HANDLEBARS_VALUE_ITERATOR_INIT(iterator, value) ) {
        result = 1;
    } else if( handlebars_value_get_intval(iterator->cur) != 42 ) {
        result = 2;
    }

    handlebars_value_iterator_close(iterator);

    map = handlebars_map_ctor(context, 1);
    map = handlebars_map_str_add(map, HBS_STRL("answer"), child);
    if( !handlebars_map_iterator_init(map_iterator, map) ||
        !handlebars_map_iterator_next(map_iterator, &map_key, &map_value) ||
        handlebars_value_get_intval(map_value) != 42 ) {
        result = 7;
    }
    handlebars_map_iterator_close(map_iterator);
    if( count_map_entries(map) != 1 ) {
        result = 8;
    }
    handlebars_map_delref(map);

    HANDLEBARS_VALUE_UNDECL(child);
    HANDLEBARS_VALUE_UNDECL(value);
    handlebars_context_dtor(context);
    printf(
        "%s 1 - public value, string, and iterator declarations\n1..1\n",
        result == 0 ? "ok" : "not ok"
    );
    return result;
}

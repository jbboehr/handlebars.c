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

#include "handlebars_map.h"
#include "handlebars_ptr.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

void handlebars_test_value_api_cpp(
    struct handlebars_value * source,
    struct handlebars_ptr * pointer,
    struct handlebars_map * map
)
{
    struct handlebars_string * key = NULL;
    struct handlebars_value * map_value = NULL;
    void * pointer_result = NULL;
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ARRAY_DECL(values, 2);
    HANDLEBARS_VALUE_ITERATOR_DECL(iterator);
    HANDLEBARS_MAP_ITERATOR_DECL(map_iterator);

    if( handlebars_ptr_try_get(pointer, "int", &pointer_result) ) {
        pointer_result = NULL;
    }
    if( handlebars_value_ptr_try_get(source, "int", &pointer_result) ) {
        pointer_result = NULL;
    }

    if( HANDLEBARS_VALUE_ITERATOR_INIT(iterator, source) ) {
        do {
            (void) iterator->cur;
        } while( handlebars_value_iterator_next(iterator) );
    }
    handlebars_value_iterator_close(iterator);

    if( handlebars_map_iterator_init(map_iterator, map) ) {
        while( handlebars_map_iterator_next(map_iterator, &key, &map_value) ) {
            (void) key;
            (void) map_value;
        }
    }
    handlebars_map_iterator_close(map_iterator);

    handlebars_map_foreach(map, entry_index, foreach_key, foreach_value) {
        (void) entry_index;
        (void) foreach_key;
        (void) foreach_value;
        break;
    } handlebars_map_foreach_end(map);

    HANDLEBARS_VALUE_FOREACH(source, child) {
        (void) child;
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_ARRAY_UNDECL(values, 2);
    HANDLEBARS_VALUE_UNDECL(value);
}

const char * handlebars_test_string_api_cpp(const struct handlebars_string * string)
{
    (void) hbs_str_len(string);
    return hbs_str_val(string);
}

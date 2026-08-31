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

#include "handlebars_ptr.h"
#include "handlebars_string.h"
#include "handlebars_value.h"

void handlebars_test_value_api_cpp(
    struct handlebars_value * source,
    struct handlebars_ptr * pointer
)
{
    void * pointer_result = NULL;
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_ARRAY_DECL(values, 2);
    HANDLEBARS_VALUE_ITERATOR_DECL(iterator);

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

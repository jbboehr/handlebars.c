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

#ifndef HANDLEBARS_VALUE_HANDLERS_H
#define HANDLEBARS_VALUE_HANDLERS_H

#include "handlebars.h"
#include "handlebars_types.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_options;
struct handlebars_string;
struct handlebars_user;
struct handlebars_value_iterator;
struct handlebars_value_handlers;

typedef struct handlebars_value * (*handlebars_copy_func)(struct handlebars_value * value);
typedef void (*handlebars_value_dtor_func)(struct handlebars_user * user);
typedef void (*handlebars_value_convert_func)(struct handlebars_value * value, bool recurse);
typedef enum handlebars_value_type (*handlebars_value_type_func)(struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, struct handlebars_string * key, struct handlebars_value * rv);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index, struct handlebars_value * rv);
typedef bool (*handlebars_iterator_init_func)(struct handlebars_value_iterator * it, struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_call_func)(struct handlebars_value * value, HANDLEBARS_FUNCTION_ARGS);
typedef long (*handlebars_count_func)(struct handlebars_value * value);

void handlebars_user_init(struct handlebars_user * user, struct handlebars_context * ctx, const struct handlebars_value_handlers * handlers)
    HBS_ATTR_NONNULL_ALL;

// {{{ Reference Counting
void handlebars_user_addref(struct handlebars_user * user)
    HBS_ATTR_NONNULL_ALL;
void handlebars_user_delref(struct handlebars_user * user)
    HBS_ATTR_NONNULL_ALL;
void handlebars_user_addref_ex(struct handlebars_user * user, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;
void handlebars_user_delref_ex(struct handlebars_user * user, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_user_addref(user) handlebars_user_addref_ex(user, #user, HBS_LOC)
#define handlebars_user_delref(user) handlebars_user_delref_ex(user, #user, HBS_LOC)
#endif
// }}} Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif

//! Common header for user-defined types
struct handlebars_user
{
    struct handlebars_context * ctx;
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif
    const struct handlebars_value_handlers * handlers;
};

struct handlebars_value_handlers {
    const char * name;
    handlebars_copy_func copy;
    handlebars_value_dtor_func dtor;
    handlebars_value_convert_func convert;
    handlebars_value_type_func type;
    handlebars_map_find_func map_find;
    handlebars_array_find_func array_find;
    handlebars_iterator_init_func iterator;
    handlebars_call_func call;
    handlebars_count_func count;
};

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_HANDLERS_H */

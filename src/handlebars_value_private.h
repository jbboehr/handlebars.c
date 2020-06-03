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

#ifndef HANDLEBARS_VALUE_PRIVATE_H
#define HANDLEBARS_VALUE_PRIVATE_H

#ifdef HANDLEBARS_VALUE_H
#warning "You should include handlebars_value_private.h before handlebars_value.h"
#endif

#include "handlebars_types.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif

HBS_EXTERN_C_START

struct handlebars_map;
struct handlebars_options;
struct handlebars_ptr;
struct handlebars_string;
struct handlebars_user;

//! Internal value union
union handlebars_value_internals
{
    long lval;
    double dval;
    struct handlebars_string * string;
    struct handlebars_map * map;
    struct handlebars_stack * stack;
    struct handlebars_user * user;
    struct handlebars_ptr * ptr;
    handlebars_helper_func helper;
    struct handlebars_options * options;
};

//! Main value struct
struct handlebars_value
{
    //! The type of value from enum #handlebars_value_type
	enum handlebars_value_type type;

    //! Bitwise value flags from enum #handlebars_value_flags
    unsigned char flags;

    //! Internal value union
    union handlebars_value_internals v;
};

/**
 * @brief Value iterator context. Should be stack allocated. Must be initialized with #handlebars_value_iterator_init
 */
struct handlebars_value_iterator
{
    //! The number of child elements
    size_t length;

    //! The current array index. Unused for map
    size_t index;

    //! The current map index. Unused for array
    struct handlebars_string * key;

    //! The element being iterated over
    struct handlebars_value * value;

    //! The current child element
    struct handlebars_value current;

    //! Opaque pointer for user-defined types
    void * usr;

    //! A function pointer to move to the next child element
    bool (*next)(struct handlebars_value_iterator * it);
};

#define HANDLEBARS_VALUE_SIZE sizeof(struct handlebars_value)
#define HANDLEBARS_VALUE_INTERNALS_SIZE sizeof(union handlebars_value_internals)
#define HANDLEBARS_VALUE_ITERATOR_SIZE sizeof(struct handlebars_value_iterator)

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_PRIVATE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */

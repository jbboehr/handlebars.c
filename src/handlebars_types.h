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

#ifndef HANDLEBARS_TYPES_H
#define HANDLEBARS_TYPES_H

HBS_EXTERN_C_START

struct handlebars_options;
struct handlebars_vm;

// {{{ value

/**
 * @brief Enumeration of value types
 */
enum handlebars_value_type
{
    HANDLEBARS_VALUE_TYPE_NULL = 0,
    HANDLEBARS_VALUE_TYPE_TRUE = 1,
    HANDLEBARS_VALUE_TYPE_FALSE = 2,
    HANDLEBARS_VALUE_TYPE_INTEGER = 3,
    HANDLEBARS_VALUE_TYPE_FLOAT = 4,
    HANDLEBARS_VALUE_TYPE_STRING = 5,
    HANDLEBARS_VALUE_TYPE_ARRAY = 6,
    HANDLEBARS_VALUE_TYPE_MAP = 7,
    //! A user-defined value type, must implement #handlebars_value_handlers
    HANDLEBARS_VALUE_TYPE_USER = 8,
    //! An opaque pointer type
    HANDLEBARS_VALUE_TYPE_PTR = 9,
    HANDLEBARS_VALUE_TYPE_HELPER = 10,
    HANDLEBARS_VALUE_TYPE_CLOSURE = 11
};

enum handlebars_value_flags
{
    HANDLEBARS_VALUE_FLAG_NONE = 0,
    //! Indicates that the string value should not be escaped when appending to the output buffer
    HANDLEBARS_VALUE_FLAG_SAFE_STRING = 1
};

// }}} value
// {{{ function

#define HANDLEBARS_FUNCTION_ARGS \
    int argc, \
    struct handlebars_value * argv, \
    struct handlebars_options * options, \
    struct handlebars_vm * vm, \
    struct handlebars_value * rv
#define HANDLEBARS_FUNCTION_ATTRS HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT
#define HANDLEBARS_FUNCTION_ARGS_PASSTHRU argc, argv, options, vm, rv
#define HANDLEBARS_FUNCTION(name) HANDLEBARS_FUNCTION_ATTRS struct handlebars_value * name(HANDLEBARS_FUNCTION_ARGS)

#if defined(__GNUC__) && !defined(__clang__)
// clang throws -Wignored-attributes
typedef HANDLEBARS_FUNCTION_ATTRS struct handlebars_value * (*handlebars_func)(HANDLEBARS_FUNCTION_ARGS);
#else
typedef struct handlebars_value * (*handlebars_func)(HANDLEBARS_FUNCTION_ARGS);
#endif

/* b/c */
#define handlebars_helper_func handlebars_func
#define HANDLEBARS_HELPER_ARGS HANDLEBARS_FUNCTION_ARGS
#define HANDLEBARS_HELPER_ATTRS HANDLEBARS_FUNCTION_ATTRS
#define HANDLEBARS_HELPER_ARGS_PASSTHRU HANDLEBARS_FUNCTION_ARGS_PASSTHRU

// }}} function
// {{{ closure

#define HANDLEBARS_CLOSURE_ARGS \
    int localc, \
    struct handlebars_value * localv, \
    HANDLEBARS_FUNCTION_ARGS
#define HANDLEBARS_CLOSURE_ATTRS HANDLEBARS_FUNCTION_ATTRS
#define HANDLEBARS_CLOSURE(name) HANDLEBARS_CLOSURE_ATTRS struct handlebars_value * name(HANDLEBARS_CLOSURE_ARGS)

#if defined(__GNUC__) && !defined(__clang__)
// clang throws -Wignored-attributes
typedef HANDLEBARS_CLOSURE_ATTRS struct handlebars_value * (*handlebars_closure_func)(HANDLEBARS_CLOSURE_ARGS);
#else
typedef struct handlebars_value * (*handlebars_closure_func)(HANDLEBARS_CLOSURE_ARGS);
#endif

// }}} closure
// {{{ options

struct handlebars_options {
    long inverse;
    long program;
    struct handlebars_string * name;
    struct handlebars_value * scope;
    struct handlebars_value * data;
    struct handlebars_value * hash;
};

// }}} options

HBS_EXTERN_C_END

#endif /* HANDLEBARS_TYPES_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */

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

#ifndef HANDLEBARS_VALUE_H
#define HANDLEBARS_VALUE_H

#include <assert.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value_handlers.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_map;
struct handlebars_options;
struct handlebars_stack;
struct handlebars_value;
struct handlebars_value_handlers;
struct json_object;
struct yaml_document_s;
struct yaml_node_s;

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
    HANDLEBARS_VALUE_TYPE_HELPER = 10
};

enum handlebars_value_flags
{
    HANDLEBARS_VALUE_FLAG_NONE = 0,
    //! Indicates that the string value should not be escaped when appending to the output buffer
    HANDLEBARS_VALUE_FLAG_SAFE_STRING = 1,
    //! Indicates that the value was not stack allocated, but allocated using talloc
    HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED = 2
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
    struct handlebars_value * current;

    //! Opaque pointer for user-defined types
    void * usr;

    //! A function pointer to move to the next child element
    bool (*next)(struct handlebars_value_iterator * it);
};

//! Common header for user-defined types
struct handlebars_user
{
    //! User-defined type handlers
    struct handlebars_value_handlers * handlers;
};

extern size_t HANDLEBARS_VALUE_SIZE;

/**
 * @brief Get the string value, or an empty string for non-string types
 * @param[in] value
 * @return The string value
 */
const char * handlebars_value_get_strval(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the string length, or zero for invalid types
 * @param[in] value
 * @return The string length
 */
size_t handlebars_value_get_strlen(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the boolean value. Follows javascript boolean conversion rules.
 * @param[in] value
 * @return The boolean value
 */
bool handlebars_value_get_boolval(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the integer value
 * @param[in] value
 * @return The integer value, or zero if not a float type
 */
long handlebars_value_get_intval(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the float value
 * @param[in] value
 * @return The float value, or zero if not a float type
 */
double handlebars_value_get_floatval(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the value as a string (primitive types only)
 * @param[in] value The handlebars value
 * @return The value as a string
 */
struct handlebars_string * handlebars_value_to_string(struct handlebars_value * value) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Lookup an index in an array
 * @param[in] value
 * @param[in] index
 * @return The found element, or NULL
 */
struct handlebars_value * handlebars_value_array_find(
    struct handlebars_value * value,
    size_t index
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Lookup a key in a map
 * @param[in] value
 * @param[in] key
 * @return The found element, or NULL
 */
struct handlebars_value * handlebars_value_map_find(
    struct handlebars_value * value,
    struct handlebars_string * key
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Lookup a key in a map
 * @param[in] value
 * @param[in] key
 * @param[in] len
 * @return The found element, or NULL
 */
struct handlebars_value * handlebars_value_map_str_find(
    struct handlebars_value * value,
    const char * key, size_t len
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Convert a value to string, following handlebars (javascript) string conversion rules
 * @param[in] value The value to convert
 * @param[in] escape Whether or not to escape the value. Overridden by #HANDLEBARS_VALUE_FLAG_SAFE_STRING
 * @return The value converted to a string
 */
struct handlebars_string * handlebars_value_expression(
    struct handlebars_value * value,
    bool escape
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Convert a value to string and append to the given buffer, following handlebars (javascript)
 *        string conversion rules.
 * @param[in] string The buffer to which the result will be appended
 * @param[in] value The value to convert
 * @param[in] escape Whether or not to escape the value. Overridden by #HANDLEBARS_VALUE_FLAG_SAFE_STRING
 * @return The original buffer with the expression appended. The pointer may change via realloc.
 */
struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_string * string,
    struct handlebars_value * value,
    bool escape
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

char * handlebars_value_dump(struct handlebars_value * value, size_t depth) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a new value
 * @param[in] Handlebars context, used for error handling and memory allocation
 * @return The newly constructed value
 */
struct handlebars_value * handlebars_value_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Create a copy of a value
 * @param[in] value
 * @return The copy of the value
 */
struct handlebars_value * handlebars_value_copy(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a value. Does not free the value object itself. Frees any child resources and sets the value to null.
 * @param[in] value
 * @return void
 */
void handlebars_value_dtor(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Convert a value from a user-defined type to an internal type
 * @param[in] value
 * @param[in] recurse
 * @return void
 */
void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse) HBS_ATTR_NONNULL_ALL;
#define handlebars_value_convert(value) handlebars_value_convert_ex(value, 1);

/**
 * @brief Initialize an iterator
 * @param[in] it The iterator to initialize
 * @param[in] value The value for iteration
 * @return true, or false if the value is empty or of an invalid type
 */
bool handlebars_value_iterator_init(
    struct handlebars_value_iterator * it,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Call a value, if the value is a callable type such as #HANDLEBARS_VALUE_TYPE_HELPER or
 *        #HANDLEBARS_VALUE_TYPE_USER. If the value is not callable, this function will return NULL.
 * @param[in] value
 * @param[in] argc
 * @param[in] argv
 * @param[in] options
 */
struct handlebars_value * handlebars_value_call(
    struct handlebars_value * value,
    int argc,
    struct handlebars_value * argv[],
    struct handlebars_options * options
) HBS_ATTR_NONNULL_ALL;

#define handlebars_value_addref(value) ((void) value)
#define handlebars_value_addref2(value) (value)
#define handlebars_value_delref(value) ((void) value)
#define handlebars_value_try_delref(value)
#define handlebars_value_try_addref(value)
#define handlebars_value_refcount(v) 999

#ifndef HANDLEBARS_VALUE_PRIVATE

/**
 * @brief Check if the value is callable
 * @param[in] value
 * @return Whether or not the value is callable
 */
bool handlebars_value_is_callable(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Check if the value is empty. Follows javascript boolean conversion rules.
 * @param[in] value
 * @return Whether or not the value is empty
 */
bool handlebars_value_is_empty(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Check if the value is a scalar type
 * @param[in] value
 * @return Whether or not the value is a scalar type
 */
bool handlebars_value_is_scalar(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the handlers for a user-defined value type
 * @param[in] value
 * @return The value handlers
 */
struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

struct handlebars_map * handlebars_value_get_map(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

void * handlebars_value_get_ptr(struct handlebars_value * value);

struct handlebars_stack * handlebars_value_get_stack(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_value_get_string(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

void * handlebars_value_get_usr(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the type of the specified value
 * @param[in] value The handlebars value
 * @return The value type
 */
enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the number of child elements for array and map
 * @param[in] value
 * @return The number of child elements, or -1 for invalid types
 */
long handlebars_value_count(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the value to null
 * @param[in] value
 * @return void
 */
void handlebars_value_null(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the boolean value
 * @param[in] value
 * @param[in] bval
 * @return void
 */
void handlebars_value_boolean(struct handlebars_value * value, bool bval) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the integer value
 * @param[in] value
 * @param[in] lval
 * @return void
 */
void handlebars_value_integer(struct handlebars_value * value, long lval) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the float value
 * @param[in] value
 * @param[in] dval
 * @return void
 */
void handlebars_value_float(struct handlebars_value * value, double dval) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the string value (#handlebars_string variant)
 * @param[in] value
 * @param[in] string
 * @return void
 */
void handlebars_value_str(struct handlebars_value * value, struct handlebars_string * string) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the string value (#handlebars_string variant) and `talloc_steal` the given string.
 * @param[in] value
 * @param[in] string
 * @return void
 */
void handlebars_value_str_steal(struct handlebars_value * value, struct handlebars_string * string) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the string value (const char[] variant)
 * @param[in] value
 * @param[in] strval
 * @return void
 */

void handlebars_value_string(struct handlebars_value * value, const char * strval) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the string value (char[] variant) and `talloc_free` the given string
 * @param[in] value
 * @param[in] strval
 * @return void
 */
void handlebars_value_string_steal(struct handlebars_value * value, char * strval) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the string value (char[] with length variant)
 * @param[in] value
 * @param[in] strval
 * @param[in] strlen
 * @return void
 */
void handlebars_value_stringl(struct handlebars_value * value, const char * strval, size_t strlen) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the value to an empty map
 * @param[in] value
 * @return void
 */
void handlebars_value_map_init(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Set the value to an empty array
 * @param[in] value
 * @return void
 */
void handlebars_value_array_init(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

void handlebars_value_ptr(struct handlebars_value * value, void * ptr) HBS_ATTR_NONNULL_ALL;

#else

//! Internal value union
union handlebars_value_internals
{
    long lval;
    double dval;
    struct handlebars_string * string;
    struct handlebars_map * map;
    struct handlebars_stack * stack;
    struct handlebars_user * usr;
    void * ptr;
    handlebars_helper_func helper;
    struct handlebars_options * options;
};

//! Main value struct
struct handlebars_value
{
    //! The type of value from enum #handlebars_value_type
	enum handlebars_value_type type;

    //! Bitwise value flags from enum #handlebars_value_flags
    unsigned long flags;

    //! Internal value union
    union handlebars_value_internals v;

    //! Handlebars context, used for error handling and memory allocation
	struct handlebars_context * ctx;
};

inline enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value);

HBS_ATTR_NONNULL_ALL
inline bool handlebars_value_is_callable(struct handlebars_value * value) {
    return handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_HELPER;
}

HBS_ATTR_NONNULL_ALL
inline bool handlebars_value_is_empty(struct handlebars_value * value) {
    return !handlebars_value_get_boolval(value);
}

HBS_ATTR_NONNULL_ALL
inline bool handlebars_value_is_scalar(struct handlebars_value * value) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_TRUE:
        case HANDLEBARS_VALUE_TYPE_FALSE:
        case HANDLEBARS_VALUE_TYPE_FLOAT:
        case HANDLEBARS_VALUE_TYPE_INTEGER:
        case HANDLEBARS_VALUE_TYPE_STRING:
            return true;
        default:
            return false;
    }
}

HBS_ATTR_NONNULL_ALL
inline struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value) {
    return value->v.usr->handlers;
}

HBS_ATTR_NONNULL_ALL
inline struct handlebars_map * handlebars_value_get_map(struct handlebars_value * value) {
    if (value->type == HANDLEBARS_VALUE_TYPE_MAP) {
        return value->v.map;
    } else {
        return NULL;
    }
}

HBS_ATTR_NONNULL_ALL
inline void * handlebars_value_get_ptr(struct handlebars_value * value) {
    if (value->type == HANDLEBARS_VALUE_TYPE_PTR) {
        return value->v.ptr;
    } else {
        return NULL;
    }
}

HBS_ATTR_NONNULL_ALL
inline struct handlebars_stack * handlebars_value_get_stack(struct handlebars_value * value) {
    if (value->type == HANDLEBARS_VALUE_TYPE_ARRAY) {
        return value->v.stack;
    } else {
        return NULL;
    }
}

HBS_ATTR_NONNULL_ALL
inline struct handlebars_string * handlebars_value_get_string(struct handlebars_value * value) {
    if (value->type == HANDLEBARS_VALUE_TYPE_STRING) {
        return value->v.string;
    } else {
        return NULL;
    }
}

HBS_ATTR_NONNULL_ALL
inline void * handlebars_value_get_usr(struct handlebars_value * value) {
    if (value->type == HANDLEBARS_VALUE_TYPE_USER) {
        return value->v.usr;
    } else {
        return NULL;
    }
}

HBS_ATTR_NONNULL_ALL
inline enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value) {
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		return handlebars_value_get_handlers(value)->type(value);
	} else {
		return value->type;
	}
}

HBS_ATTR_NONNULL_ALL
inline long handlebars_value_count(struct handlebars_value * value) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_length(value->v.stack);
        case HANDLEBARS_VALUE_TYPE_MAP:
            return handlebars_map_count(value->v.map);
        case HANDLEBARS_VALUE_TYPE_USER:
            return (handlebars_value_handlers_get_count_fn(handlebars_value_get_handlers(value)))(value);
        default:
            return -1;
    }
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_null(struct handlebars_value * value) {
    if( value->type != HANDLEBARS_VALUE_TYPE_NULL ) {
        handlebars_value_dtor(value);
    }
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_boolean(struct handlebars_value * value, bool bval) {
    handlebars_value_null(value);
    value->type = bval ? HANDLEBARS_VALUE_TYPE_TRUE : HANDLEBARS_VALUE_TYPE_FALSE;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_integer(struct handlebars_value * value, long lval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_INTEGER;
    value->v.lval = lval;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_float(struct handlebars_value * value, double dval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_FLOAT;
    value->v.dval = dval;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_str(struct handlebars_value * value, struct handlebars_string * string) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = handlebars_string_copy_ctor(value->ctx, string);
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_str_steal(struct handlebars_value * value, struct handlebars_string * string) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = talloc_steal(value->ctx, string);
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_string(struct handlebars_value * value, const char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen(strval))/*)*/;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_string_steal(struct handlebars_value * value, char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen(strval))/*)*/;
    talloc_free(strval); // meh
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_stringl(struct handlebars_value * value, const char * strval, size_t strlen) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen)/*)*/;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_map_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = /*talloc_steal(value,*/ handlebars_map_ctor(value->ctx)/*)*/;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_array_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    value->v.stack = /*talloc_steal(value,*/ handlebars_stack_ctor(value->ctx)/*)*/;
}

HBS_ATTR_NONNULL_ALL
inline void handlebars_value_ptr(struct handlebars_value * value, void * ptr) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_PTR;
    value->v.ptr = ptr;
}

#endif

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_H */

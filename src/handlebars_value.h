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

#ifndef HANDLEBARS_VALUE_H
#define HANDLEBARS_VALUE_H

#include <assert.h>
#include <string.h>

#if defined(__cplusplus) || defined(__STDC_NO_VLA__) || defined(_MSC_VER)
#if defined(__clang__) || defined(__GNUC__)
#define HBS_VALUE_STACK_ALLOC(size) __builtin_alloca(size)
#elif defined(_MSC_VER)
#include <malloc.h>
#define HBS_VALUE_STACK_ALLOC(size) _alloca(size)
#elif defined(HAVE_ALLOCA_H)
#include <alloca.h>
#define HBS_VALUE_STACK_ALLOC(size) alloca(size)
#else
#error "handlebars_value.h requires compiler-supported stack allocation when VLAs are unavailable"
#endif
#endif

#include "handlebars.h"
#include "handlebars_types.h"

HBS_EXTERN_C_START

// {{{ Prototypes & Variables

struct handlebars_closure;
struct handlebars_context;
struct handlebars_map;
struct handlebars_options;
struct handlebars_ptr;
struct handlebars_stack;
struct handlebars_user;
struct handlebars_value;
struct handlebars_value_handlers;
struct handlebars_value_iterator;
struct json_object;
struct yaml_document_s;
struct yaml_node_s;

/** Maximum number of active containers in a recursive value traversal. */
#define HANDLEBARS_VALUE_MAX_DEPTH 256

/**
 * @brief Value iterator context. Declare with
 *        #HANDLEBARS_VALUE_ITERATOR_DECL and initialize with
 *        #HANDLEBARS_VALUE_ITERATOR_INIT.
 */
struct handlebars_value_iterator
{
    //! The current array index. Unused for map
    size_t index;

    //! The current map index. Unused for array
    struct handlebars_string * key;

    //! The element being iterated over
    struct handlebars_value * value;

    //! The current child element
    struct handlebars_value * cur;

    //! Opaque backing-state pointer used by the iterator implementation
    void * usr;

    //! Retained user-defined value owner, when applicable
    struct handlebars_user * user;

    //! A function pointer to move to the next child element
    bool (*next)(struct handlebars_value_iterator * it);

    //! A function pointer to release iterator-owned state
    void (*close)(struct handlebars_value_iterator * it);

    //! Opaque storage position used by native iterators
    size_t position;

    //! Internal error-unwind linkage
    struct handlebars_value_iterator * unwind_next;
    struct handlebars_value_iterator ** unwind_previous;
    jmp_buf * unwind_target;
};

#ifndef HANDLEBARS_VALUE_SIZE
extern const size_t HANDLEBARS_VALUE_SIZE;
#endif
#ifndef HANDLEBARS_VALUE_INTERNALS_SIZE
extern const size_t HANDLEBARS_VALUE_INTERNALS_SIZE;
#endif
#define HANDLEBARS_VALUE_ITERATOR_SIZE sizeof(struct handlebars_value_iterator)

// }}} Prototypes & Variables

// {{{ Constructors and Destructors

#if defined(HBS_HAVE_ATTR_CLEANUP)
#define HANDLEBARS_VALUE_DECL_CLEANUP HBS_ATTR_CLEANUP(handlebars_value_cleanup)
#else
#define HANDLEBARS_VALUE_DECL_CLEANUP
#endif

#if defined(HANDLEBARS_VALUE_SIZE)
// We know the size at compile-time
#define HANDLEBARS_VALUE_DECL_PRED(name) \
    struct handlebars_value mem_ ## name = {0}; \
    struct handlebars_value * const name HANDLEBARS_VALUE_DECL_CLEANUP = &mem_ ## name
#elif !defined(__STDC_NO_VLA__) && !defined(__cplusplus) && !defined(_MSC_VER)
// Use a char vla
#define HANDLEBARS_VALUE_DECL_PRED(name) \
    char mem_ ## name[HANDLEBARS_VALUE_SIZE]; \
    struct handlebars_value * const name HANDLEBARS_VALUE_DECL_CLEANUP = (struct handlebars_value *) mem_ ## name; \
    handlebars_value_init(name);
#else
// Use compiler-supported stack allocation; storage lasts until function return.
#define HANDLEBARS_VALUE_DECL_PRED(name) \
    struct handlebars_value * const name HANDLEBARS_VALUE_DECL_CLEANUP = (struct handlebars_value *) HBS_VALUE_STACK_ALLOC(HANDLEBARS_VALUE_SIZE); \
    handlebars_value_init(name);
#endif

/**
 * @def HANDLEBARS_VALUE_DECL
 * @brief Declare automatic storage for one value.
 *
 * On C++, MSVC, and C implementations without VLAs, the storage is
 * `alloca`-backed and is reclaimed when the containing function returns.
 * Hoist declarations outside long-running loops in those builds.
 */
#if defined(HANDLEBARS_ENABLE_DEBUG)
// The cleanup variable helps makes sure there is a matching pair of DECL/UNDECL
#define HANDLEBARS_VALUE_DECL(name) \
    void * cleanup_ ## name = NULL; \
    HANDLEBARS_VALUE_DECL_PRED(name)

#define HANDLEBARS_VALUE_UNDECL(name) \
    handlebars_value_dtor(name); \
    (void) cleanup_ ## name
#else
#define HANDLEBARS_VALUE_DECL(name) HANDLEBARS_VALUE_DECL_PRED(name)
#define HANDLEBARS_VALUE_UNDECL(name) handlebars_value_dtor(name)
#endif

// C does not support zero-length arrays, so reserve one slot for an empty logical array
#define HANDLEBARS_VALUE_ARRAY_CAPACITY(num) ((num) > 0 ? (num) : 1)

#if (defined(HANDLEBARS_VALUE_SIZE) && !defined(__STDC_NO_VLA__) && !defined(__cplusplus) && !defined(_MSC_VER))
// We know the size of value at compile-time, but we don't know if num is a constant, so we have be able to support VLA
#define HANDLEBARS_VALUE_ARRAY_DECL_PRED(name, num) \
    struct handlebars_value name[HANDLEBARS_VALUE_ARRAY_CAPACITY(num)]; \
    memset(&name, 0, sizeof(name));
#define HANDLEBARS_VALUE_ARRAY_AT(name, pos) (&name[pos])
#elif !defined(__STDC_NO_VLA__) && !defined(__cplusplus) && !defined(_MSC_VER)
// Use a char vla
#define HANDLEBARS_VALUE_ARRAY_DECL_PRED(name, num) \
    char mem_ ## name[HANDLEBARS_VALUE_SIZE * HANDLEBARS_VALUE_ARRAY_CAPACITY(num)]; \
    struct handlebars_value * name = (struct handlebars_value *) &mem_ ## name; \
    memset(name, 0, HANDLEBARS_VALUE_SIZE * HANDLEBARS_VALUE_ARRAY_CAPACITY(num));
#else
// Use compiler-supported stack allocation; storage lasts until function return.
#define HANDLEBARS_VALUE_ARRAY_DECL_PRED(name, num) \
    struct handlebars_value * name = (struct handlebars_value *) HBS_VALUE_STACK_ALLOC(HANDLEBARS_VALUE_SIZE * HANDLEBARS_VALUE_ARRAY_CAPACITY(num)); \
    memset(name, 0, HANDLEBARS_VALUE_SIZE * HANDLEBARS_VALUE_ARRAY_CAPACITY(num));
#endif

#ifndef HANDLEBARS_VALUE_ARRAY_AT
#define HANDLEBARS_VALUE_ARRAY_AT(name, pos) ((struct handlebars_value *) ((char *) name + (HANDLEBARS_VALUE_SIZE * (pos))))
#endif

#define HANDLEBARS_VALUE_ARRAY_UNDECL_PRED(name, num) \
    do { \
        for (int i_ ## name = 0; i_ ## name < num; (i_ ## name)++) { \
            handlebars_value_dtor(HANDLEBARS_VALUE_ARRAY_AT(name, i_ ## name)); \
        } \
    } while (0)

/**
 * @def HANDLEBARS_VALUE_ARRAY_DECL
 * @brief Declare automatic storage for an array of values.
 *
 * On C++, MSVC, and C implementations without VLAs, the storage is
 * `alloca`-backed and is reclaimed when the containing function returns.
 * Hoist declarations outside long-running loops in those builds.
 */
#if defined(HANDLEBARS_ENABLE_DEBUG)
// The cleanup variable helps makes sure there is a matching pair of DECL/UNDECL
#define HANDLEBARS_VALUE_ARRAY_DECL(name, num) \
    void * cleanup_ ## name = NULL; \
    HANDLEBARS_VALUE_ARRAY_DECL_PRED(name, num)

#define HANDLEBARS_VALUE_ARRAY_UNDECL(name, num) \
    HANDLEBARS_VALUE_ARRAY_UNDECL_PRED(name, num); \
    (void) cleanup_ ## name
#else
#define HANDLEBARS_VALUE_ARRAY_DECL(name, num) HANDLEBARS_VALUE_ARRAY_DECL_PRED(name, num)
#define HANDLEBARS_VALUE_ARRAY_UNDECL(name, num) HANDLEBARS_VALUE_ARRAY_UNDECL_PRED(name, num)
#endif

#if defined(HANDLEBARS_ENABLE_DEBUG) && defined(HANDLEBARS_HAVE_STATEMENT_EXPRESSIONS)
#define HANDLEBARS_ARG_AT_EX(argc, argv, pos) ({ \
        assert(pos < argc); \
        HANDLEBARS_VALUE_ARRAY_AT(argv, pos); \
    })
#else
#define HANDLEBARS_ARG_AT_EX(argc, argv, pos) HANDLEBARS_VALUE_ARRAY_AT(argv, pos)
#endif

#define HANDLEBARS_ARG_AT(pos) HANDLEBARS_ARG_AT_EX(argc, argv, pos)
#define HANDLEBARS_LOCAL_AT(pos) HANDLEBARS_ARG_AT_EX(localc, localv, pos)

/**
 * @brief Construct a new value
 * @param[in] Handlebars context, used for error handling and memory allocation
 * @return The newly constructed value
 */
struct handlebars_value * handlebars_value_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT HBS_ATTR_DEPRECATED;

/**
 * @brief Destruct a value. Does not free the value object itself. Frees any child resources and sets the value to null.
 * @param[in] value
 * @return void
 */
void handlebars_value_dtor(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

struct handlebars_value * handlebars_value_init(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

void handlebars_value_cleanup(struct handlebars_value * const * value);

// }}} Constructors and Destructors

// {{{ Getters

/**
 * @brief Get the type of the specified value
 * @param[in] value The handlebars value
 * @return The value type
 */
enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

enum handlebars_value_type handlebars_value_get_real_type(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

unsigned char handlebars_value_get_flags(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the handlers for a user-defined value type
 * @param[in] value
 * @return The value handlers
 */
const struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

struct handlebars_map * handlebars_value_get_map(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

/**
 * @brief Retrieve a borrowed pointer from a native pointer value.
 *
 * USER values whose effective type is PTR are not native pointer values. On a
 * native-type or registered-type mismatch, #result is set to NULL. Retrieval
 * does not transfer ownership; the pointee's lifetime follows the ownership
 * policy of the contained pointer wrapper, including externally managed
 * storage constructed with nofree=true.
 *
 * @param[in] value The value to inspect
 * @param[in] typ The registered C type name
 * @param[out] result The borrowed pointer, or NULL on failure
 * @return true for a matching native pointer value, otherwise false
 */
bool handlebars_value_ptr_try_get(
    struct handlebars_value * value,
    const char * typ,
    void ** result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

void * handlebars_value_get_ptr_ex(struct handlebars_value * value, const char * typ)
    HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;
#define handlebars_value_get_ptr(value, typ) ((typ *) handlebars_value_get_ptr_ex(value, HBS_S1(typ)))

struct handlebars_stack * handlebars_value_get_stack(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_value_get_string(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

struct handlebars_user * handlebars_value_get_user(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

struct handlebars_closure * handlebars_value_get_closure(struct handlebars_value * value)
    HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the string value, or an empty string for non-string types
 * @param[in] value
 * @return The string value
 */
const char * handlebars_value_get_strval(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the string length, or zero for invalid types
 * @param[in] value
 * @return The string length
 */
size_t handlebars_value_get_strlen(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the boolean value. Follows javascript boolean conversion rules.
 * @param[in] value
 * @return The boolean value
 */
bool handlebars_value_get_boolval(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the integer value
 * @param[in] value
 * @return The integer value, or zero if not a float type
 */
long handlebars_value_get_intval(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the float value
 * @param[in] value
 * @return The float value, or zero if not a float type
 */
double handlebars_value_get_floatval(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

// }}} Getters

// {{{ Conversion

/**
 * @brief Get the value as a string (primitive types only)
 * @param[in] value The handlebars value
 * @param[in] context The handlebars memory context used for converted values
 * @return For string values, the value's retained string, which the caller must
 *         release with handlebars_string_delref(). For other primitive types,
 *         a fresh string owned by @p context, which the caller may free directly.
 */
struct handlebars_string * handlebars_value_to_string(
    struct handlebars_value * value,
    struct handlebars_context * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Convert a value to string, following handlebars (javascript) string conversion rules.
 *        Cyclic graphs and container nesting beyond #HANDLEBARS_VALUE_MAX_DEPTH
 *        raise an error.
 * @param[in] context The handlebars memory context
 * @param[in] value The value to convert
 * @param[in] escape Whether or not to escape the value. Overridden by #HANDLEBARS_VALUE_FLAG_SAFE_STRING
 * @return The value converted to a string
 */
struct handlebars_string * handlebars_value_expression(
    struct handlebars_context * context,
    struct handlebars_value * value,
    bool escape
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Convert a value to string and append to the given buffer, following handlebars (javascript)
 *        string conversion rules. Cyclic graphs and container nesting beyond
 *        #HANDLEBARS_VALUE_MAX_DEPTH raise an error.
 * @param[in] context The handlebars memory context
 * @param[in] value The value to convert
 * @param[in] string The buffer to which the result will be appended
 * @param[in] escape Whether or not to escape the value. Overridden by #HANDLEBARS_VALUE_FLAG_SAFE_STRING
 * @return The original buffer with the expression appended. The pointer may change via realloc.
 */
struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_string * string,
    bool escape
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Convert a value from a user-defined type to an internal type. Native
 *        cyclic graphs and container nesting beyond
 *        #HANDLEBARS_VALUE_MAX_DEPTH raise an error. User-defined conversion
 *        handlers remain responsible for bounding their own internal traversal.
 * @param[in] value
 * @param[in] recurse
 * @return void
 */
void handlebars_value_convert_ex(
    struct handlebars_value * value,
    bool recurse
) HBS_ATTR_NONNULL_ALL;

#define handlebars_value_convert(value) handlebars_value_convert_ex(value, 1);

bool handlebars_value_eq(
    struct handlebars_value * value1,
    struct handlebars_value * value2
) HBS_ATTR_NONNULL_ALL;

// }}} Conversion

// {{{ Mutators

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

void handlebars_value_ptr(struct handlebars_value * value, struct handlebars_ptr * ptr) HBS_ATTR_NONNULL_ALL;

void handlebars_value_user(struct handlebars_value * value, struct handlebars_user * user) HBS_ATTR_NONNULL_ALL;

void handlebars_value_map(struct handlebars_value * value, struct handlebars_map * map) HBS_ATTR_NONNULL_ALL;

void handlebars_value_array(struct handlebars_value * value, struct handlebars_stack * stack) HBS_ATTR_NONNULL_ALL;

void handlebars_value_helper(struct handlebars_value * value, handlebars_helper_func helper) HBS_ATTR_NONNULL_ALL;

void handlebars_value_closure(struct handlebars_value * value, struct handlebars_closure * closure) HBS_ATTR_NONNULL_ALL;

void handlebars_value_value(struct handlebars_value * dest, struct handlebars_value * src) HBS_ATTR_NONNULL_ALL;

void handlebars_value_set_flag(struct handlebars_value * value, enum handlebars_value_flags flag)
    HBS_ATTR_NONNULL_ALL;

// }}} Mutators

// {{{ Misc

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
 * @brief Get the number of child elements for array and map
 * @param[in] value
 * @return The number of child elements, or -1 for invalid types
 */
long handlebars_value_count(struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

// }}} Misc

// {{{ Array

/**
 * @brief Replace an element in a native stack-backed array.
 *
 * This legacy entry point aborts when #value is not a native array and may
 * propagate allocation or bounds failures through `longjmp`. USER values whose
 * effective type is ARRAY are read-only through this interface.
 *
 * @param[in,out] value The native array to update
 * @param[in] index The element index, or the current count to append
 * @param[in] child The replacement value
 */
void handlebars_value_array_set(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Replace an element without allowing an internal `longjmp` to escape.
 *
 * Returns #HANDLEBARS_TYPE_ERROR without changing #value when it is not a
 * native stack-backed array. USER values whose effective type is ARRAY are not
 * mutable through this interface. A type error does not change any context's
 * error state; captured native-container errors remain available from the
 * container's allocation context.
 *
 * @param[in,out] value The native array to update
 * @param[in] index The element index, or the current count to append
 * @param[in] child The replacement value
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_array_set_try(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Append an element to a native stack-backed array.
 *
 * This legacy entry point aborts when #value is not a native array and may
 * propagate allocation failures through `longjmp`. USER values whose effective
 * type is ARRAY are read-only through this interface.
 *
 * @param[in,out] value The native array to update
 * @param[in] child The value to append
 */
void handlebars_value_array_push(
    struct handlebars_value * value,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Append an element without allowing an internal `longjmp` to escape.
 *
 * Returns #HANDLEBARS_TYPE_ERROR without changing #value when it is not a
 * native stack-backed array. USER values whose effective type is ARRAY are not
 * mutable through this interface. A type error does not change any context's
 * error state; captured native-container errors remain available from the
 * container's allocation context.
 *
 * @param[in,out] value The native array to update
 * @param[in] child The value to append
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_array_push_try(
    struct handlebars_value * value,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Lookup an index in an array
 * @param[in] value
 * @param[in] index
 * @return The found element, or NULL
 */
struct handlebars_value * handlebars_value_array_find(
    struct handlebars_value * value,
    size_t index,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL;

// }}} Array

// {{{ Map

struct handlebars_value * handlebars_value_map_find(
    struct handlebars_value * value,
    struct handlebars_string * key,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL;

struct handlebars_value * handlebars_value_map_str_find(
    struct handlebars_value * value,
    const char * key,
    size_t len,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Update a native map.
 *
 * This legacy entry point aborts when #value is not a native map and may
 * propagate allocation failures through `longjmp`. USER values whose effective
 * type is MAP are read-only through this interface.
 *
 * @param[in,out] value The native map to update
 * @param[in] key The key to update or add
 * @param[in] child The value to store
 */
void handlebars_value_map_update(
    struct handlebars_value * value,
    struct handlebars_string * key,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Update a native map without allowing an internal `longjmp` to escape.
 *
 * Returns #HANDLEBARS_TYPE_ERROR without changing #value when it is not a
 * native map. USER values whose effective type is MAP are read-only through
 * this interface. A type error does not change any context's error state;
 * captured native-container errors remain available from the container's
 * allocation context.
 *
 * @param[in,out] value The native map to update
 * @param[in] key The key to update or add
 * @param[in] child The value to store
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_map_update_try(
    struct handlebars_value * value,
    struct handlebars_string * key,
    struct handlebars_value * child
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

// }}} Map

// {{{ Misc

/**
 * @brief Return a diagnostic representation of a value. Cyclic graphs and
 *        container nesting beyond #HANDLEBARS_VALUE_MAX_DEPTH raise an error.
 */
char * handlebars_value_dump(
    struct handlebars_value * value,
    struct handlebars_context * context,
    size_t depth
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Call a value, if the value is a callable type such as #HANDLEBARS_VALUE_TYPE_HELPER or
 *        #HANDLEBARS_VALUE_TYPE_USER. If the value is not callable, this function will return NULL.
 * @param[in] value
 * @param[in] argc
 * @param[in] argv
 * @param[in] options
 * @param[in] vm
 * @param[out] rv
 */
struct handlebars_value * handlebars_value_call(
    struct handlebars_value * value,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * vm,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

const char * handlebars_value_type_readable(enum handlebars_value_type type)
    HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

// }}} Misc

// {{{ Iteration

#if defined(HBS_HAVE_ATTR_CLEANUP)
#define HANDLEBARS_VALUE_ITERATOR_DECL_CLEANUP HBS_ATTR_CLEANUP(handlebars_value_iterator_cleanup)
#else
#define HANDLEBARS_VALUE_ITERATOR_DECL_CLEANUP
#endif

/**
 * @brief Declare a value iterator and its private current-value storage.
 *
 * On C++, MSVC, and C implementations without VLAs, this uses `alloca`-style
 * storage that is reclaimed when the containing function returns. Repeated
 * declarations in a long-running loop therefore accumulate stack storage;
 * hoist the declaration outside such loops and close the iterator before
 * reinitializing it.
 *
 * @param name A bare C identifier used by #HANDLEBARS_VALUE_ITERATOR_INIT.
 */
#if defined(HANDLEBARS_VALUE_ITERATOR_SIZE) && defined(HANDLEBARS_VALUE_SIZE)
// We know the size at compile-time
#define HANDLEBARS_VALUE_ITERATOR_DECL(name) \
    struct { struct handlebars_value_iterator it; struct handlebars_value value; } mem_ ## name = {0}; \
    struct handlebars_value_iterator * const name HANDLEBARS_VALUE_ITERATOR_DECL_CLEANUP = &mem_ ## name.it; \
    struct handlebars_value * const hbs_value_iterator_current_ ## name = &mem_ ## name.value
#elif !defined(__STDC_NO_VLA__) && !defined(__cplusplus) && !defined(_MSC_VER)
// Use a char vla
#define HANDLEBARS_VALUE_ITERATOR_DECL(name) \
    char mem_ ## name[HANDLEBARS_VALUE_ITERATOR_SIZE + HANDLEBARS_VALUE_SIZE]; \
    struct handlebars_value_iterator * const name HANDLEBARS_VALUE_ITERATOR_DECL_CLEANUP = (struct handlebars_value_iterator *) mem_ ## name; \
    struct handlebars_value * const hbs_value_iterator_current_ ## name = (struct handlebars_value *) (mem_ ## name + HANDLEBARS_VALUE_ITERATOR_SIZE); \
    memset(mem_ ## name, 0, sizeof(mem_ ## name))
#else
// Use compiler-supported stack allocation; storage lasts until function return.
#define HANDLEBARS_VALUE_ITERATOR_DECL(name) \
    struct handlebars_value_iterator * const name HANDLEBARS_VALUE_ITERATOR_DECL_CLEANUP = (struct handlebars_value_iterator *) HBS_VALUE_STACK_ALLOC(HANDLEBARS_VALUE_ITERATOR_SIZE + HANDLEBARS_VALUE_SIZE); \
    struct handlebars_value * const hbs_value_iterator_current_ ## name = (struct handlebars_value *) (((char *) name) + HANDLEBARS_VALUE_ITERATOR_SIZE); \
    memset(name, 0, HANDLEBARS_VALUE_ITERATOR_SIZE + HANDLEBARS_VALUE_SIZE)
#endif

#define HBS_VALUE_ITERATOR_CURRENT(name) hbs_value_iterator_current_ ## name

/**
 * @brief Initialize an iterator declared with
 *        #HANDLEBARS_VALUE_ITERATOR_DECL. An initialized iterator retains its
 *        backing storage and may outlive the source value. It must be advanced
 *        to completion or closed with #handlebars_value_iterator_close.
 * @param name The bare identifier passed to
 *             #HANDLEBARS_VALUE_ITERATOR_DECL.
 * @param value The value for iteration.
 * @return true, or false if the value is empty or of an invalid type.
 */
#define HANDLEBARS_VALUE_ITERATOR_INIT(name, value) \
    handlebars_value_iterator_init_internal( \
        name, \
        HBS_VALUE_ITERATOR_CURRENT(name), \
        value \
    )

/**
 * @name Value iteration
 *
 * On the `alloca` fallback described by #HANDLEBARS_VALUE_ITERATOR_DECL, each
 * foreach invocation inside an enclosing loop retains its declaration storage
 * until the containing function returns. Use an explicitly hoisted iterator
 * for long-running loops in those builds.
 * @{
 */

#define HANDLEBARS_VALUE_FOREACH(value, v) \
    do { \
        HANDLEBARS_VALUE_ITERATOR_DECL(iter); \
        if (HANDLEBARS_VALUE_ITERATOR_INIT(iter, value)) { \
            do { \
                struct handlebars_value * v = iter->cur; \

#define HANDLEBARS_VALUE_FOREACH_IDX(value, idx, v) \
    do { \
        HANDLEBARS_VALUE_ITERATOR_DECL(iter); \
        if (HANDLEBARS_VALUE_ITERATOR_INIT(iter, value)) { \
            do { \
                size_t idx = iter->index; \
                struct handlebars_value * v = iter->cur;

#define HANDLEBARS_VALUE_FOREACH_KV(value, k, v) \
    do { \
        HANDLEBARS_VALUE_ITERATOR_DECL(iter); \
        if (HANDLEBARS_VALUE_ITERATOR_INIT(iter, value)) { \
            do { \
                struct handlebars_string * k = iter->key; \
                struct handlebars_value * v = iter->cur;

#define HANDLEBARS_VALUE_FOREACH_IDX_KV(value, idx, k, v) \
    do { \
        HANDLEBARS_VALUE_ITERATOR_DECL(iter); \
        if (HANDLEBARS_VALUE_ITERATOR_INIT(iter, value)) { \
            do { \
                size_t idx = iter->index; \
                struct handlebars_string * k = iter->key; \
                struct handlebars_value * v = iter->cur;

#define HANDLEBARS_VALUE_FOREACH_END() \
            } while (handlebars_value_iterator_next(iter)); \
        } \
        handlebars_value_iterator_close(iter); \
    } while(0)

/** @} */

/**
 * @internal Backend for #HANDLEBARS_VALUE_ITERATOR_INIT. This symbol is
 *           exported only so the public macro can call it from downstream
 *           translation units. Callers must use the declaration and
 *           initialization macros instead.
 * @param[in,out] it Exact-size iterator storage
 * @param[in,out] current Uninitialized storage of at least
 *                        `HANDLEBARS_VALUE_SIZE` bytes that overlaps neither
 *                        @p it nor @p value; initialized by this function and
 *                        managed by @p it until it is closed
 * @param[in] value The value to iterate
 * @return true, or false if @p value is empty or not iterable
 */
bool handlebars_value_iterator_init_internal(
    struct handlebars_value_iterator * it,
    struct handlebars_value * current,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;

bool handlebars_value_iterator_next(
    struct handlebars_value_iterator * it
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Release iterator-owned state. Safe to call more than once after the
 *        iterator has been zero initialized or passed to
 *        #HANDLEBARS_VALUE_ITERATOR_INIT. Manually initialized iterators that
 *        are not advanced to completion must be closed explicitly. The foreach
 *        macros close their iterator, and supported compilers also arrange
 *        scope cleanup for iterators declared with
 *        #HANDLEBARS_VALUE_ITERATOR_DECL.
 * @param[in] it The initialized or zero-initialized iterator to close
 */
void handlebars_value_iterator_close(
    struct handlebars_value_iterator * it
) HBS_ATTR_NONNULL_ALL;

void handlebars_value_iterator_cleanup(
    struct handlebars_value_iterator * const * it
);

// }}} Iteration

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */

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

#ifndef HANDLEBARS_STRING_H
#define HANDLEBARS_STRING_H

#include <stdarg.h>
#include <stdint.h>

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_string;

#define HBS_STR_STRL(string) hbs_str_val(string), hbs_str_len(string)
#define HBS_STR_SIZE(length) ((HANDLEBARS_STRING_SIZE) + (length) + (1))

extern const size_t HANDLEBARS_STRING_SIZE;

// {{{ Accessors
char * hbs_str_val(struct handlebars_string * str)
    HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

size_t hbs_str_len(struct handlebars_string * str)
    HBS_ATTR_NONNULL_ALL;

uint32_t hbs_str_hash(struct handlebars_string * str)
    HBS_ATTR_NONNULL_ALL;
// }}} Accessors

// {{{ Hash functions
extern const char * HANDLEBARS_XXHASH_VERSION;
extern const unsigned HANDLEBARS_XXHASH_VERSION_ID;

uint32_t handlebars_hash_djbx33a(const char * str, size_t len)
    HBS_ATTR_NONNULL_ALL;

uint64_t handlebars_hash_xxh3(const char * str, size_t len)
    HBS_ATTR_NONNULL_ALL;

uint32_t handlebars_hash_xxh3low(const char * str, size_t len)
    HBS_ATTR_NONNULL_ALL;

uint32_t handlebars_string_hash(const char * str, size_t len)
    HBS_ATTR_NONNULL_ALL;
// }}} Hash functions

// {{{ Reference Counting
void handlebars_string_addref(struct handlebars_string * string)
    HBS_ATTR_NONNULL_ALL;
void handlebars_string_delref(struct handlebars_string * string)
    HBS_ATTR_NONNULL_ALL;
void handlebars_string_addref_ex(struct handlebars_string * string, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;
void handlebars_string_delref_ex(struct handlebars_string * string, const char * expr, const char * loc)
    HBS_ATTR_NONNULL_ALL;
void handlebars_string_immortalize(struct handlebars_string * string)
    HBS_ATTR_NONNULL_ALL;

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_string_addref(string) handlebars_string_addref_ex(string, #string, HBS_LOC)
#define handlebars_string_delref(string) handlebars_string_delref_ex(string, #string, HBS_LOC)
#endif
// }}} Reference Counting

/**
 * @brief Implements `strnstr`
 * @param[in] haystack
 * @param[in] haystack_len
 * @param[in] needle
 * @param[in] needle_len
 * @return Pointer to the found position, or NULL
 */
const char * handlebars_strnstr(
    const char * haystack,
    size_t haystack_len,
    const char * needle,
    size_t needle_len
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Performs a string replace in-place. `replacement` must not be longer than `search`.
 * @param[in] string The input string
 * @param[in] search The search string
 * @param[in] search_len The search string length
 * @param[in] replacement The replacement string
 * @param[in] replacement_len The replacement string length
 * @return The original pointer, transformed
 */
struct handlebars_string * handlebars_str_reduce(
    struct handlebars_string * string,
    const char * search, size_t search_len,
    const char * replacement, size_t replacement_len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Performs a string replace.
 * @param[in] context The handlebars context
 * @param[in] string The input string
 * @param[in] search The search string
 * @param[in] search_len The search string length
 * @param[in] replacement The replacement string
 * @param[in] replacement_len The replacement string length
 * @return A newly allocated string, transformed
 */
struct handlebars_string * handlebars_str_replace(
    struct handlebars_context * context,
    const struct handlebars_string * string,
    const char * search, size_t search_len,
    const char * replacement, size_t replacement_len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Adds slashes to as string for a list of specified characters. Returns a
 *        newly allocated string.
 * @param[in] context The handlebars context
 * @param[in] string The string to which to add slashes
 * @param[in] what A list of characters to escape
 * @param[in] what_length The length of the character list
 * @return The string with escaped characters
 */
struct handlebars_string * handlebars_string_addcslashes(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * what, size_t what_length
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Strip slashes in place
 * @param[in] string
 * @return The original string
 */
struct handlebars_string * handlebars_string_stripcslashes(
    struct handlebars_string * string
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Implements `asprintf` for #handlebars_string
 * @see asprintf(3)
 * @param[in] context
 * @param[in] fmt
 * @return A newly constructed string
 */
struct handlebars_string * handlebars_string_asprintf(
    struct handlebars_context * context,
    const char * fmt,
    ...
) HBS_ATTR_PRINTF(2, 3) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Implements `asprintf` by appending for #handlebars_string
 * @see asprintf(3)
 * @param[in] context
 * @param[in] fmt
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_asprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    ...
) HBS_ATTR_PRINTF(3, 4) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Implements `vasprintf` for #handlebars_string
 * @see vasprintf(3)
 * @param[in] context
 * @param[in] fmt
 * @param[in] ap
 * @return A newly constructed string
 */
struct handlebars_string * handlebars_string_vasprintf(
    struct handlebars_context * context,
    const char * fmt,
    va_list ap
) HBS_ATTR_PRINTF(2, 0) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Implements `vasprintf` by appending for #handlebars_string
 * @see vasprintf(3)
 * @param[in] context
 * @param[in] fmt
 * @param[in] ap
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_vasprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    va_list ap
) HBS_ATTR_PRINTF(3, 0) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Escapes HTML control characters using the handlebars (not PHP) escape sequences
 * @param[in] context
 * @param[in] str
 * @param[in] len
 * @return A newly constructed string
 */
struct handlebars_string * handlebars_string_htmlspecialchars(
    struct handlebars_context * context,
    const char * str, size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Escapes HTML control characters using the handlebars (not PHP) escape sequences appended to the
 *        specified string
 * @param[in] context
 * @param[in] string
 * @param[in] str
 * @param[in] len
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_htmlspecialchars_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Concat an array of strings with the specified separator
 * @param[in] context
 * @param[in] sep
 * @param[in] sep_len
 * @param[in] parts NULL-terminated array of strings
 * @return A newly constructed string
 */
struct handlebars_string * handlebars_string_implode(
    struct handlebars_context * context,
    const char * sep,
    size_t sep_len,
    /*const*/ struct handlebars_string ** parts
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Indent all text by the specified indent
 * @param[in] context
 * @param[in] str
 * @param[in] str_len
 * @param[in] indent
 * @param[in] indent_len
 * @return A newly constructed string
 */
struct handlebars_string * handlebars_string_indent(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const struct handlebars_string * indent_str
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_string * handlebars_string_indent_append(
    struct handlebars_context * context,
    struct handlebars_string * append_to_string,
    struct handlebars_string * input_string,
    const struct handlebars_string * indent_str
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Trims a set of characters off the left end of string. Trims in
 *        place by setting a null terminator and moving the contents
 * @param[in] string the string to trim
 * @param[in] what the set of characters to trim
 * @param[in] what_length The length of the character list
 * @return the original pointer, with trimmed input characters
 */
struct handlebars_string * handlebars_string_ltrim(
    struct handlebars_string * string,
    const char * what, size_t what_length
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Trims a set of characters off the right end of string. Trims in
 *        place by setting a null terminator.
 * @param[in] string the string to trim
 * @param[in] what the set of characters to trim
 * @param[in] what_length The length of the character list
 * @return the original pointer, with null moved to trim input characters
 */
struct handlebars_string * handlebars_string_rtrim(
    struct handlebars_string * string,
    const char * what, size_t what_length
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Allocate a new, empty string of the specified length
 * @param[in] context
 * @param[in] size
 * @return The newly allocated string
 */
struct handlebars_string * handlebars_string_init(
    struct handlebars_context * context,
    size_t length
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a string from the specified parameters, including hash
 * @param[in] context
 * @param[in] str
 * @param[in] len
 * @param[in] hash
 * @return The newly constructed string
 */
struct handlebars_string * handlebars_string_ctor_ex(
    struct handlebars_context * context,
    const char * str,
    size_t len,
    uint32_t hash
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a string from the specified parameters
 * @param[in] context
 * @param[in] str
 * @param[in] len
 * @return The newly constructed string
 */
struct handlebars_string * handlebars_string_ctor(
    struct handlebars_context * context,
    const char * str, size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a copy of a string
 * @param[in] context
 * @param[in] string
 * @return The newly constructed string
 */
struct handlebars_string * handlebars_string_copy_ctor(
    struct handlebars_context * context,
    const struct handlebars_string * string
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Reserve the specified size with an existing string. Will not decrease the size of the buffer.
 * @param[in] context
 * @param[in] string
 * @param[in] len The desired total length of the string, no less than the current length
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_extend(
    struct handlebars_context * context,
    struct handlebars_string * string,
    size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Append to a string without checking length or reallocating
 * @param[in] string
 * @param[in] str
 * @param[in] len
 * @return The original string
 */
struct handlebars_string * handlebars_string_append_unsafe(
    struct handlebars_string * string,
    const char * str, size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Append to a string
 * @param[in] context
 * @param[in] string
 * @param[in] str
 * @param[in] len
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Append to a string
 * @param[in] context
 * @param[in] string
 * @param[in] str
 * @return The original string, unless moved by reallocation
 */
struct handlebars_string * handlebars_string_append_str(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const struct handlebars_string * string2
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Resize a string buffer to match the size of it's contents
 * @param[in] string
 * @return The original string
 */
struct handlebars_string * handlebars_string_compact(
    struct handlebars_string * string
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Compare two strings (#handlebars_string variant)
 * @param[in] string1
 * @param[in] string2
 * @return Whether or not the strings are equal
 */
bool handlebars_string_eq(
    /*const*/ struct handlebars_string * string1,
    /*const*/ struct handlebars_string * string2
) HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_string_truncate(
    struct handlebars_string * string,
    size_t start,
    size_t end
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

bool hbs_str_eq_strl(
    struct handlebars_string * string1,
    const char * str2,
    size_t len2
) HBS_ATTR_NONNULL_ALL;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_STRING_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */

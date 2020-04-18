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

#ifndef HANDLEBARS_STRING_H
#define HANDLEBARS_STRING_H

#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <memory.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define HBS_STRVAL(str) str->val
#define HBS_STRLEN(str) str->len

struct handlebars_string {
    size_t len;
    unsigned long hash;
    char val[];
};

#define HBS_STR_STRL(string) string->val, string->len
#define HBS_STR_HASH_EX(str, len, hash) (hash ? hash : handlebars_string_hash(str, len))
#define HBS_STR_HASH(string) (string->hash ? string->hash : (string->hash = handlebars_string_hash(string->val, string->len)))
#define HBS_STR_SIZE(length) (offsetof(struct handlebars_string, val) + (length) + 1)

HBS_ATTR_NONNULL(1)
static inline unsigned long handlebars_string_hash_cont(const char * str, size_t len, unsigned long hash)
{
    unsigned long c;
    const unsigned char * ptr = (const unsigned char *) str;
    const unsigned char * end = ptr + len;
    for( c = *ptr++; ptr <= end && c; c = *ptr++ ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

HBS_ATTR_NONNULL(1)
static inline unsigned long handlebars_string_hash(const char * str, size_t len)
{
    unsigned long hash = 5381;
    return handlebars_string_hash_cont(str, len, hash);
}

/**
 * @brief Allocate a new, empty string of the specified length
 * @param[in] context
 * @param[in] size
 * @return The newly allocated string
 */
HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_init(struct handlebars_context * context, size_t length)
{
    struct handlebars_string * st = handlebars_talloc_size(context, HBS_STR_SIZE(length));
    HANDLEBARS_MEMCHECK(st, context);
    st->len = 0;
    st->val[0] = 0;
    return st;
}

/**
 * @brief Construct a string from the specified parameters, including hash
 * @param[in] context
 * @param[in] str
 * @param[in] len
 * @param[in] hash
 * @return The newly constructed string
 */
HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_ctor_ex(
    struct handlebars_context * context,
    const char * str, size_t len, unsigned long hash
) {
    struct handlebars_string * st = handlebars_string_init(context, len + 1);
    HANDLEBARS_MEMCHECK(st, context);
    st->len = len;
    memcpy(st->val, str, len);
    st->val[st->len] = 0;
    st->hash = hash;
    return st;
}

/**
 * @brief Construct a string from the specified parameters
 * @param[in] context
 * @param[in] str
 * @param[in] len
 * @return The newly constructed string
 */
HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_ctor(
    struct handlebars_context * context,
    const char * str, size_t len
) {
    return handlebars_string_ctor_ex(context, str, len, 0);
}

/**
 * @brief Construct a copy of a string
 * @param[in] context
 * @param[in] string
 * @return The newly constructed string
 */
HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_copy_ctor(
    struct handlebars_context * context,
    const struct handlebars_string * string
) {
    size_t size = HBS_STR_SIZE(string->len);
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    memcpy(st, string, size);
    return st;
}

/**
 * @brief Reserve the specified size with an existing string. Will not decrease the size of the buffer.
 * @param[in] context
 * @param[in] string
 * @param[in] len The desired total length of the string, no less than the current length
 * @return The original string, unless moved by reallocation
 */
HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_extend(
    struct handlebars_context * context,
    struct handlebars_string * string,
    size_t len
) {
    size_t size;
    if( string == NULL ) {
        return handlebars_string_init(context, len);
    }
    size = HBS_STR_SIZE(len);
    if( size > talloc_get_size(string) ) {
        string = (struct handlebars_string *) handlebars_talloc_realloc_size(context, string, size);
        HANDLEBARS_MEMCHECK(string, context);
    }
    return string;
}

/**
 * @brief Append to a string without checking length or reallocating
 * @param[in] string
 * @param[in] str
 * @param[in] len
 * @return The original string
 */
HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_append_unsafe(
    struct handlebars_string * string,
    const char * str, size_t len
) {
    memcpy(string->val + string->len, str, len);
    string->len += len;
    string->val[string->len] = 0;
    string->hash = 0;
    return string;
}

/**
 * @brief Append to a string
 * @param[in] context
 * @param[in] string
 * @param[in] str
 * @param[in] len
 * @return The original string, unless moved by reallocation
 */
HBS_ATTR_NONNULL(1, 2, 3) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) {
    string = handlebars_string_extend(context, string, string->len + len);
    string = handlebars_string_append_unsafe(string, str, len);
    return string;
}

/**
 * @brief Append to a string
 * @param[in] context
 * @param[in] string
 * @param[in] str
 * @return The original string, unless moved by reallocation
 */
HBS_ATTR_NONNULL(1, 2, 3) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_append_str(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const struct handlebars_string * string2
) {
    return handlebars_string_append(context, string, string2->val, string2->len);
}

/**
 * @brief Resize a string buffer to match the size of it's contents
 * @param[in] string
 * @return The original string
 */
HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_compact(struct handlebars_string * string) {
    size_t size = HBS_STR_SIZE(string->len);
    if( talloc_get_size(string) > size ) {
        return (struct handlebars_string *) handlebars_talloc_realloc_size(NULL, string, size);
    } else {
        return string;
    }
}

/**
 * @brief Compare two strings (const char[] with length and hash variant)
 * @param[in] string1
 * @param[in] length1
 * @param[in] hash1
 * @param[in] string2
 * @param[in] length2
 * @param[in] hash2
 * @return Whether or not the strings are equal
 */
HBS_ATTR_NONNULL(1, 4)
static inline bool handlebars_string_eq_ex(
    const char * string1, size_t length1, unsigned long hash1,
    const char * string2, size_t length2, unsigned long hash2
) {
    if( length1 != length2 ) {
        return false;
    } else {
        return HBS_STR_HASH_EX(string1, length1, hash1) == HBS_STR_HASH_EX(string2, length2, hash2);
    }
}

/**
 * @brief Compare two strings (#handlebars_string variant)
 * @param[in] string1
 * @param[in] string2
 * @return Whether or not the strings are equal
 */
HBS_ATTR_NONNULL(1, 2)
static inline bool handlebars_string_eq(
    /*const*/ struct handlebars_string * string1,
    /*const*/ struct handlebars_string * string2
) {
    if( string1->len != string2->len ) {
        return false;
    } else {
        return HBS_STR_HASH(string1) == HBS_STR_HASH(string2);
    }
}

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
) HBS_ATTR_NONNULL(1, 3);

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
) HBS_ATTR_NONNULL(1, 2, 4);

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
) HBS_ATTR_NONNULL(1, 2, 3, 5);

/**
 * @brief Adds slashes to as string for a list of specified characters. Returns a
 *        newly allocated string, or NULL on failure.
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
) HBS_ATTR_NONNULL(1, 2, 3) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Strip slashes in place
 * @param[in] string
 * @return The original string
 */
struct handlebars_string * handlebars_string_stripcslashes(
    struct handlebars_string * string
) HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_PRINTF(2, 3) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_PRINTF(3, 4) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_PRINTF(2, 0) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_PRINTF(3, 0) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

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
    /*const*/ struct handlebars_string** parts
) HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL;

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
    const char * str, size_t str_len,
    const char * indent, size_t indent_len
) HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

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
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif

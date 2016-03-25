
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
#define HBS_STR_HASH_EX(str, len, hash) (hash ?: handlebars_string_hash(str, len))
#define HBS_STR_HASH(string) HBS_STR_HASH_EX(string->val, string->len, string->hash)

#define HANDLEBARS_STRING_SIZE(size) (offsetof(struct handlebars_string, val) + (size) + 1)

HBS_ATTR_NONNULL(1)
static inline unsigned long handlebars_string_hash_cont(const unsigned char * str, size_t len, unsigned long hash)
{
    unsigned long c;
    const unsigned char * end = str + len;
    for( c = *str++; str <= end && c; c = *str++ ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

HBS_ATTR_NONNULL(1)
static inline unsigned long handlebars_string_hash(const unsigned char * str, size_t len)
{
    unsigned long hash = 5381;
    return handlebars_string_hash_cont(str, len, hash);
}

HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_init(struct handlebars_context * context, size_t size)
{
    struct handlebars_string * st = handlebars_talloc_size(context, HANDLEBARS_STRING_SIZE(size));
    HANDLEBARS_MEMCHECK(st, context);
    st->len = 0;
    st->val[0] = 0;
    return st;
}

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

HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_ctor(
    struct handlebars_context * context,
    const char * str, size_t len
) {
    return handlebars_string_ctor_ex(context, str, len, 0);
}

HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_copy_ctor(
    struct handlebars_context * context,
    struct handlebars_string * string
) {
    size_t size = HANDLEBARS_STRING_SIZE(string->len);
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    memcpy(st, string, size);
    return st;
}

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
    size = HANDLEBARS_STRING_SIZE(len);
    if( size > talloc_get_size(string) ) {
        string = (struct handlebars_string *) handlebars_talloc_realloc_size(context, string, size);
        HANDLEBARS_MEMCHECK(string, context);
    }
    return string;
}

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

HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL
static inline struct handlebars_string * handlebars_string_compact(struct handlebars_string * string) {
    size_t size = HANDLEBARS_STRING_SIZE(string->len);
    if( talloc_get_size(string) > size ) {
        return (struct handlebars_string *) handlebars_talloc_realloc_size(NULL, string, size);
    } else {
        return string;
    }
}

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

HBS_ATTR_NONNULL(1, 2)
static inline bool handlebars_string_eq(struct handlebars_string * string1, struct handlebars_string * string2)
{
    return handlebars_string_eq_ex(string1->val, string1->len, string1->hash, string2->val, string2->len, string2->hash);
}

/**
 * @brief Implements strnstr
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
 *
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
 * @brief Adds slashes to as string for a list of specified characters. Returns a
 *        newly allocated string, or NULL on failure.
 *
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

struct handlebars_string * handlebars_string_stripcslashes(
    struct handlebars_string * string
) HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_asprintf(
    struct handlebars_context * context,
    const char * fmt,
    ...
) HBS_ATTR_PRINTF(2, 3) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_asprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    ...
) HBS_ATTR_PRINTF(3, 4) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_vasprintf(
    struct handlebars_context * context,
    const char * fmt,
    va_list ap
) HBS_ATTR_PRINTF(2, 0) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_vasprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    va_list ap
) HBS_ATTR_PRINTF(3, 0) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_htmlspecialchars(
    struct handlebars_context * context,
    const char * str, size_t len
) HBS_ATTR_NONNULL(1) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_htmlspecialchars_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_implode(
    struct handlebars_context * context,
    const char * sep,
    size_t sep_len,
    struct handlebars_string ** parts
) HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_indent(
    struct handlebars_context * context,
    const char * str, size_t str_len,
    const char * indent, size_t indent_len
) HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Trims a set of characters off the left end of string. Trims in
 *        place by setting a null terminator and moving the contents
 *
 * @param[in] str the string to trim
 * @param[in] str_length The length of the input string
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
 *
 * @param[in] str the string to trim
 * @param[in] str_length The length of the input string
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
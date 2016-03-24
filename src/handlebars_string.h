
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

#define HANDLEBARS_STRING_SIZE(size) (offsetof(struct handlebars_string, val) + (size) + 1)

static inline unsigned long handlebars_string_hash_cont(const unsigned char * str, size_t len, unsigned long hash)
{
    size_t c, i;
    for( i = 0, c = *str++; i < len && c; i++, c = *str++ ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static inline unsigned long handlebars_string_hash(const unsigned char * str, size_t len)
{
    unsigned long hash = 5381;
    return handlebars_string_hash_cont(str, len, hash);
}

static inline struct handlebars_string * handlebars_string_init(struct handlebars_context * context, size_t size)
{
    struct handlebars_string * st = handlebars_talloc_size(context, HANDLEBARS_STRING_SIZE(size));
    HANDLEBARS_MEMCHECK(st, context);
    st->len = 0;
    return st;
}

static inline struct handlebars_string * handlebars_string_ctor_ex(struct handlebars_context * context, const char * str, size_t len, unsigned long hash)
{
    struct handlebars_string * st = handlebars_string_init(context, len + 1);
    HANDLEBARS_MEMCHECK(st, context);
    st->len = len;
    memcpy(st->val, str, len);
    st->val[st->len] = 0;
    st->hash = hash;
    return st;
}

static inline struct handlebars_string * handlebars_string_ctor(struct handlebars_context * context, const char * str, size_t len)
{
    return handlebars_string_ctor_ex(context, str, len, handlebars_string_hash((const unsigned char *)str, len));
}

static inline struct handlebars_string * handlebars_string_copy_ctor(struct handlebars_context * context, struct handlebars_string * string)
{
    size_t size = HANDLEBARS_STRING_SIZE(string->len);
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    memcpy(st, string, size);
    return st;
}

static inline struct handlebars_string * handlebars_string_extend(struct handlebars_context * context, struct handlebars_string * string, size_t len)
{
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

static inline struct handlebars_string * handlebars_string_append(struct handlebars_context * context, struct handlebars_string * st2, const char * str, size_t len)
{
    unsigned long newhash = handlebars_string_hash_cont((const unsigned char *)str, len, st2->hash);
    st2 = handlebars_string_extend(context, st2, st2->len + len);
    memcpy(st2->val + st2->len, str, len);
    st2->len += len;
    st2->val[st2->len] = 0;
    st2->hash = newhash;
    return st2;
}

static inline struct handlebars_string * handlebars_string_compact(struct handlebars_string * string) {
    size_t size = HANDLEBARS_STRING_SIZE(string->len);
    if( talloc_get_size(string) > size ) {
        return (struct handlebars_string *) handlebars_talloc_realloc_size(NULL, string, size);
    } else {
        return string;
    }
}

static inline bool handlebars_string_eq_ex(
    HBS_ATTR_UNUSED const char * s1, size_t l1, unsigned long h1,
    HBS_ATTR_UNUSED const char * s2, size_t l2, unsigned long h2
) {
    // Only compare via length and hash for now
    return (l1 == l2 && h1 == h2/* && 0 == strncmp(s1, s2, l1)*/);
}

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
) PRINTF_ATTRIBUTE(2, 3) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_asprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    ...
) PRINTF_ATTRIBUTE(3, 4) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_vasprintf(
    struct handlebars_context * context,
    const char * fmt,
    va_list ap
) PRINTF_ATTRIBUTE(2, 0) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_vasprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    va_list ap
) PRINTF_ATTRIBUTE(3, 0) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_string_implode(
    struct handlebars_context * context,
    const char * sep,
    size_t sep_len,
    struct handlebars_string ** parts
) HBS_ATTR_NONNULL(1, 2, 4) HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif
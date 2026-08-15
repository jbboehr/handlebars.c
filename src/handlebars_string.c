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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <talloc.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-default"

#define XXH_PRIVATE_API
#define XXH_INLINE_ALL
#include "xxhash.h"

#pragma GCC diagnostic pop

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



struct handlebars_string {
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif
    size_t len;
    uint32_t hash;
    char val[];
};

struct htmlspecialchars_pair {
    const char * str;
    size_t len;
};

static const struct htmlspecialchars_pair htmlspecialchars[256] = {
    ['&']  = {HBS_STRL("&amp;")},
    ['"']  = {HBS_STRL("&quot;")},
    ['\''] = {HBS_STRL("&#x27;")}, //"&#039;"
    ['<']  = {HBS_STRL("&lt;")},
    ['>']  = {HBS_STRL("&gt;")},
    ['`']  = {HBS_STRL("&#x60;")},
};

const size_t HANDLEBARS_STRING_SIZE = sizeof(struct handlebars_string);

const char * HANDLEBARS_XXHASH_VERSION = HBS_S2(XXH_VERSION_MAJOR) "." HBS_S2(XXH_VERSION_MINOR) "." HBS_S2(XXH_VERSION_RELEASE);
const unsigned HANDLEBARS_XXHASH_VERSION_ID = (XXH_VERSION_MAJOR * 100 * 100) + (XXH_VERSION_MINOR * 100) + XXH_VERSION_RELEASE;



size_t handlebars_string_size(size_t length)
{
    if( unlikely(length > SIZE_MAX - HANDLEBARS_STRING_SIZE - 1) ) {
        return 0;
    }

    return HANDLEBARS_STRING_SIZE + length + 1;
}

static size_t string_size_or_throw(struct handlebars_context * context, size_t length)
{
    size_t size = handlebars_string_size(length);

    if( unlikely(size == 0) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, "String capacity is too large: %zu", length);
    }

    return size;
}

static size_t string_length_add(
    struct handlebars_context * context,
    size_t left,
    size_t right
) {
    if( unlikely(right > SIZE_MAX - left) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, "String length is too large");
    }

    return left + right;
}

static bool string_source_offset(
    const struct handlebars_string * string,
    const char * source,
    size_t length,
    size_t * offset
) {
    uintptr_t base = (uintptr_t) string->val;
    uintptr_t address = (uintptr_t) source;
    size_t source_offset;

    if( address < base ) {
        return false;
    }

    source_offset = address - base;
    if( source_offset > string->len || length > string->len - source_offset ) {
        return false;
    }

    *offset = source_offset;
    return true;
}



// {{{ Accessors

char * hbs_str_val(struct handlebars_string * str) {
    return str->val;
}

size_t hbs_str_len(struct handlebars_string * str) {
    return str->len;
}

uint32_t hbs_str_hash(struct handlebars_string * str) {
    if (str->hash == 0) {
        str->hash = handlebars_string_hash(str->val, str->len);
    }
    return str->hash;
}

// }}} Accessors

// {{{ Hash functions

uint32_t handlebars_hash_djbx33a(const char * str, size_t len)
{
    uint32_t hash = 5381;
    uint32_t c = 0;
    const unsigned char * ptr = (const unsigned char *) str;
    const unsigned char * end = ptr + len;
    while( ptr < end && (c = *ptr++) != 0 ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

uint64_t handlebars_hash_xxh3(const char * str, size_t len)
{
    XXH3_state_t state;
    XXH3_64bits_reset(&state);
    XXH3_64bits_update(&state, str, len);
    return XXH3_64bits_digest(&state);
}

uint32_t handlebars_hash_xxh3low(const char * str, size_t len)
{
    return (uint32_t) handlebars_hash_xxh3(str, len);
}

uint32_t handlebars_string_hash(const char * str, size_t len)
{
#if 0
    return handlebars_hash_djbx33a(str, len);
#else
    return handlebars_hash_xxh3low(str, len);
#endif
}

// }}} Hash functions

// {{{ Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
static void string_rc_dtor(struct handlebars_rc * rc)
{
#ifdef HANDLEBARS_ENABLE_DEBUG
    if (getenv("HANDLEBARS_RC_DEBUG")) {
        fprintf(stderr, "STR DTOR %p\n", hbs_container_of(rc, struct handlebars_string, rc));
    }
#endif
    struct handlebars_string * string = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_string, rc), struct handlebars_string);
    handlebars_talloc_free(string);
}
#endif

#undef handlebars_string_addref
#undef handlebars_string_delref

void handlebars_string_addref(struct handlebars_string * string)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&string->rc);
#endif
}

void handlebars_string_delref(struct handlebars_string * string)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&string->rc, string_rc_dtor);
#endif
}

void handlebars_string_addref_ex(struct handlebars_string * string, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&string->rc);
        fprintf(stderr, "STR ADDREF %p (%zu -> %zu) %s %s\n", string, rc, rc + 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_string_addref(string);
#endif
}

void handlebars_string_delref_ex(struct handlebars_string * string, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&string->rc);
        fprintf(stderr, "STR DELREF %p (%zu -> %zu) %s %s\n", string, rc, rc - 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_string_delref(string);
#endif
}

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_string_addref(string) handlebars_string_addref_ex(string, #string, HBS_LOC)
#define handlebars_string_delref(string) handlebars_string_delref_ex(string, #string, HBS_LOC)
#endif

static inline struct handlebars_string * separate_string(struct handlebars_string * string)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&string->rc) > 1) {
        struct handlebars_string * prev_string = string;
        void * parent = talloc_parent(string);
        assert(parent != NULL);
        string = handlebars_string_copy_ctor(HBSCTX(parent), string);
        if (handlebars_rc_refcount(&prev_string->rc) >= 1) { // ugh
            handlebars_string_addref(string);
        }
        handlebars_string_delref(prev_string);
    }
#endif

    return string;
}

void handlebars_string_immortalize(struct handlebars_string * string)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    string->rc.refcount = UINT32_MAX;
#endif
}
// }}} Reference Counting



struct handlebars_string * handlebars_string_init(
    struct handlebars_context * context,
    size_t length
) {
    size_t size = string_size_or_throw(context, length);
    struct handlebars_string * st = handlebars_talloc_zero_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    talloc_set_type(st, struct handlebars_string);
    st->val[0] = 0;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&st->rc);
#endif
    return st;
}

struct handlebars_string * handlebars_string_ctor_ex(
    struct handlebars_context * context,
    const char * str,
    size_t len,
    uint32_t hash
) {
    struct handlebars_string * st = handlebars_string_init(context, len);
    st->len = len;
    memcpy(st->val, str, len);
    st->val[st->len] = 0;
    st->hash = hash;
    return st;
}

struct handlebars_string * handlebars_string_ctor(
    struct handlebars_context * context,
    const char * str,
    size_t len
) {
    return handlebars_string_ctor_ex(context, str, len, 0);
}

struct handlebars_string * handlebars_string_copy_ctor(
    struct handlebars_context * context,
    const struct handlebars_string * string
) {
    size_t size = string_size_or_throw(context, string->len);
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    talloc_set_type(st, struct handlebars_string);
    memcpy(st, string, size);
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&st->rc);
#endif
    return st;
}

static struct handlebars_string * string_copy_for_capacity(
    struct handlebars_context * context,
    const struct handlebars_string * string,
    size_t capacity
) {
    size_t size = string_size_or_throw(context, capacity);
    size_t used_size = string_size_or_throw(context, string->len);
    void * parent = talloc_parent(string);
    struct handlebars_string * copy;

    if( size < used_size ) {
        size = used_size;
    }

    assert(parent != NULL);
    copy = handlebars_talloc_size(parent, size);
    HANDLEBARS_MEMCHECK(copy, context);
    talloc_set_type(copy, struct handlebars_string);
    memcpy(copy, string, used_size);
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&copy->rc);
#endif
    return copy;
}

static struct handlebars_string * string_take_copy(
    struct handlebars_string * string,
    struct handlebars_string * copy
) {
#ifndef HANDLEBARS_NO_REFCOUNT
    if( handlebars_rc_refcount(&string->rc) >= 1 ) {
        handlebars_string_addref(copy);
    }
    handlebars_string_delref(string);
#else
    handlebars_talloc_free(string);
#endif
    return copy;
}

static struct handlebars_string * string_reserve_for_write(
    struct handlebars_context * context,
    struct handlebars_string * string,
    size_t len
) {
    size_t size = string_size_or_throw(context, len);
#ifndef HANDLEBARS_NO_REFCOUNT
    if( handlebars_rc_refcount(&string->rc) > 1 ) {
        struct handlebars_string * copy = string_copy_for_capacity(context, string, len);
        return string_take_copy(string, copy);
    }
#endif
    if( size > talloc_get_size(string) ) {
        struct handlebars_string * resized = (struct handlebars_string *) handlebars_talloc_realloc_size(context, string, size);
        HANDLEBARS_MEMCHECK(resized, context);
        string = resized;
        talloc_set_type(resized, struct handlebars_string);
    }
    return string;
}

struct handlebars_string * handlebars_string_extend(
    struct handlebars_context * context,
    struct handlebars_string * string,
    size_t len
) {
    size_t size = string_size_or_throw(context, len);

    if( size > talloc_get_size(string) ) {
        string = string_reserve_for_write(context, string, len);
    }
    return string;
}

struct handlebars_string * handlebars_string_append_unsafe(
    struct handlebars_string * string,
    const char * str, size_t len
) {
    memmove(string->val + string->len, str, len);
    string->len += len;
    string->val[string->len] = 0;
    string->hash = 0;
    return string;
}

struct handlebars_string * handlebars_string_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) {
    size_t source_offset = 0;
    bool source_is_internal = string_source_offset(string, str, len, &source_offset);
    size_t new_length = string_length_add(context, string->len, len);

    string = string_reserve_for_write(context, string, new_length);
    if( source_is_internal ) {
        str = string->val + source_offset;
    }
    string = handlebars_string_append_unsafe(string, str, len);
    return string;
}

struct handlebars_string * handlebars_string_append_str(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const struct handlebars_string * string2
) {
    return handlebars_string_append(context, string, string2->val, string2->len);
}

struct handlebars_string * handlebars_string_compact(struct handlebars_string * string) {
    size_t size = handlebars_string_size(string->len);
    if( size != 0 && talloc_get_size(string) > size ) {
        struct handlebars_string * resized;
        string = separate_string(string);
        resized = (struct handlebars_string *) handlebars_talloc_realloc_size(NULL, string, size);
        if( resized != NULL ) {
            string = resized;
            talloc_set_type(resized, struct handlebars_string);
        }
    }
    return string;
}

bool handlebars_string_eq(
    /*const*/ struct handlebars_string * string1,
    /*const*/ struct handlebars_string * string2
) {
    if( string1->len != string2->len ) {
        return false;
    } else {
        return hbs_str_hash(string1) == hbs_str_hash(string2)
            && memcmp(string1->val, string2->val, string1->len) == 0;
    }
}

struct handlebars_string * handlebars_string_truncate(
    struct handlebars_string * string,
    size_t start,
    size_t end
) {
    string = separate_string(string);

    if( end > string->len ) {
        end = string->len;
    }
    if( start > end ) {
        start = end;
    }

    // Truncate right
    string->len = end;

    // Truncate left
    if (start > 0) {
        memmove(string->val, string->val + start, string->len - start);
        string->len -= start;
    }

    string->val[string->len] = 0;
    string->hash = 0;

    return string;
}

bool hbs_str_eq_strl(
    struct handlebars_string * string1,
    const char * str2,
    size_t len2
) {
    if (hbs_str_len(string1) != len2) {
        return false;
    }
    return 0 == memcmp(hbs_str_val(string1), str2, len2);
}

const char * handlebars_strnstr(const char * haystack, size_t haystack_len, const char * needle, size_t needle_len)
{
    if( !needle_len || !haystack_len || needle_len > haystack_len ) {
        return NULL;
    }

    const char * end = haystack + haystack_len - needle_len;

    for( ; haystack <= end; haystack++ ) {
        if( haystack[0] == needle[0] && memcmp(haystack, needle, needle_len) == 0 ) {
            return haystack;
        }
    }

    return NULL;
}

struct handlebars_string * handlebars_str_reduce(
    struct handlebars_string * string,
    const char * search, size_t search_len,
    const char * replacement, size_t replacement_len
) {
    char * tok;

    if( replacement_len > search_len || search_len <= 0 || string->len <= 0 ) {
        return string;
    }

    string = separate_string(string);
    tok = string->val;

    while( NULL != (tok = (char *) handlebars_strnstr(tok, string->len - (tok - string->val), search, search_len)) ) {
        memmove(tok, replacement, replacement_len);
        memmove(tok + replacement_len, tok + search_len, string->len - search_len - (tok - string->val));
        memset(string->val + string->len - search_len + replacement_len, 0, 1);
        string->len -= search_len - replacement_len;

        tok += replacement_len;
        if( tok >= string->val + string->len ) {
            break;
        }
    }

    assert(string->val[string->len] == 0);
    string->hash = 0;

    return string;
}

struct handlebars_string * handlebars_str_replace(
    struct handlebars_context * context,
    const struct handlebars_string * string,
    const char * search, size_t search_len,
    const char * replacement, size_t replacement_len
) {
    const char * tok = string->val;
    const char * last_tok = string->val;
    struct handlebars_string * new_string;
    size_t new_len = 0;

    if( search_len <= 0 || string->len <= 0 ) {
        return (struct handlebars_string *) string;
    }

    // Calculate the exact output size before allocating. Besides making the
    // arithmetic checkable, this avoids reallocating once per expanding match.
    while( NULL != (tok = handlebars_strnstr(tok, string->len - (tok - string->val), search, search_len)) ) {
        new_len = string_length_add(context, new_len, tok - last_tok);
        new_len = string_length_add(context, new_len, replacement_len);
        tok += search_len;
        last_tok = tok;
    }
    new_len = string_length_add(context, new_len, string->len - (last_tok - string->val));

    new_string = handlebars_string_init(context, new_len);

    tok = string->val;
    last_tok = string->val;

    while( NULL != (tok = handlebars_strnstr(tok, string->len - (tok - string->val), search, search_len)) ) {
        new_string = handlebars_string_append_unsafe(new_string, last_tok, tok - last_tok);
        new_string = handlebars_string_append_unsafe(new_string, replacement, replacement_len);

        tok += search_len;
        last_tok = tok;
    }

    new_string = handlebars_string_append_unsafe(new_string, last_tok, string->len - (last_tok - string->val));

    assert(new_string->val[new_string->len] == 0);
    new_string->hash = 0;

    new_string = handlebars_string_compact(new_string);
    return new_string;
}

struct handlebars_string * handlebars_string_addcslashes(struct handlebars_context * context, struct handlebars_string * string, const char * what, size_t what_length)
{
    char flags[256];
    struct handlebars_string * new_string;
    char * source;
    char * target;
    char * end;
    size_t i;
    size_t new_len = string->len;

    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    // Calculate the exact output size.
    for( i = 0; i < string->len; i++ ) {
        unsigned char c = (unsigned char) string->val[i];
        size_t extra = 0;

        if( flags[c] ) {
            extra = 1;
            if( c < 32 || c > 126 ) {
                switch( c ) {
                    case '\n':
                    case '\t':
                    case '\r':
                    case '\a':
                    case '\v':
                    case '\b':
                    case '\f':
                        break;
                    default:
                        extra = 3;
                        break;
                }
            }
        }
        new_len = string_length_add(context, new_len, extra);
    }

    // Allocate new string
    new_string = handlebars_string_init(context, new_len);

    // Perform replace
    source = string->val;
    target = new_string->val;
    end = string->val + string->len;
    for( ; source < end ; source++ ) {
        unsigned char c = (unsigned char) *source;
        if( flags[c] ) {
            if (c < 32 || c > 126) {
                *target++ = '\\';
                switch( c ) {
                    case '\n': *target++ = 'n'; break;
                    case '\t': *target++ = 't'; break;
                    case '\r': *target++ = 'r'; break;
                    case '\a': *target++ = 'a'; break;
                    case '\v': *target++ = 'v'; break;
                    case '\b': *target++ = 'b'; break;
                    case '\f': *target++ = 'f'; break;
                    default: target += sprintf(target, "%03o", c);
                }
                continue;
            }
            *target++ = '\\';

        }
        *target++ = c;
    }
    *target = 0;

    new_string->len = target - new_string->val;
    new_string->hash = 0;

    return handlebars_string_compact(new_string);
}

struct handlebars_string * handlebars_string_stripcslashes(struct handlebars_string * string)
{
    size_t nlen = string->len;
    size_t i;
    char numtmp[4];

    string = separate_string(string);

    char * source = string->val;
    char * target = string->val;
    char * end = string->val + string->len;

    for( ; source < end; source++ ) {
        if( *source == '\\' && source + 1 < end ) {
            source++;
            switch (*source) {
                case 'n':  *target++='\n'; nlen--; break;
                case 'r':  *target++='\r'; nlen--; break;
                case 'a':  *target++='\a'; nlen--; break;
                case 't':  *target++='\t'; nlen--; break;
                case 'v':  *target++='\v'; nlen--; break;
                case 'b':  *target++='\b'; nlen--; break;
                case 'f':  *target++='\f'; nlen--; break;
                case '\\': *target++='\\'; nlen--; break;
                case 'x':
                    if( source+1 < end && isxdigit((unsigned char) *(source+1)) ) {
                        numtmp[0] = *++source;
                        if( source+1 < end && isxdigit((unsigned char) *(source+1)) ) {
                            numtmp[1] = *++source;
                            numtmp[2] = '\0';
                            nlen -= 3;
                        } else {
                            numtmp[1] = '\0';
                            nlen -= 2;
                        }
                        *target++ = (char) strtol(numtmp, NULL, 16);
                        break;
                    }
                    /* fallthrough */
                default:
                    i = 0;
                    while( source < end && *source >= '0' && *source <= '7' && i<3 ) {
                        numtmp[i++] = *source++;
                    }
                    if( i ) {
                        numtmp[i] = '\0';
                        *target++ = (char) strtol(numtmp, NULL, 8);
                        nlen -= i;
                        source--;
                    } else {
                        *target++ = *source;
                        nlen--;
                    }
            }
        } else {
            *target ++= *source;
        }
    }

    //if (nlen != 0) {
        *target = '\0';
    //}

    string->len = nlen;
    string->hash = 0;

    return handlebars_string_compact(string);
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wformat-nonliteral"
#endif

struct handlebars_string * handlebars_string_asprintf(
    struct handlebars_context * context,
    const char * fmt,
    ...
) {
    va_list ap;
    struct handlebars_string * string = handlebars_string_init(context, strlen(fmt));

    va_start(ap, fmt);
    string = handlebars_string_vasprintf_append(context, string, fmt, ap);
    va_end(ap);

    return string;
}

struct handlebars_string * handlebars_string_asprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    ...
) {
    va_list ap;

    va_start(ap, fmt);
    string = handlebars_string_vasprintf_append(context, string, fmt, ap);
    va_end(ap);

    return string;
}

struct handlebars_string * handlebars_string_vasprintf(
    struct handlebars_context * context,
    const char * fmt,
    va_list ap
) {
    struct handlebars_string * string = handlebars_string_init(context, strlen(fmt));
    return handlebars_string_vasprintf_append(context, string, fmt, ap);
}

struct handlebars_string * handlebars_string_vasprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    va_list ap
) {
    va_list ap2;
    int result;
    size_t len;
    size_t new_length;
    char * formatted;

    // Calculate size
    va_copy(ap2, ap);
    result = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    // Nothing to do
    if( result <= 0 ) {
        return string;
    }
    len = (size_t) result;
    new_length = string_length_add(context, string->len, len);

    // Keep the destination unchanged while formatting because the format or
    // any of its arguments may point into it.
    formatted = handlebars_talloc_size(context, len + 1);
    HANDLEBARS_MEMCHECK(formatted, context);

    va_copy(ap2, ap);
    result = vsnprintf(formatted, len + 1, fmt, ap2);
    va_end(ap2);
    if( result <= 0 || (size_t) result > len ) {
        handlebars_talloc_free(formatted);
        return string;
    }

    string = string_reserve_for_write(context, string, new_length);
    string = handlebars_string_append_unsafe(string, formatted, (size_t) result);
    handlebars_talloc_free(formatted);
    return string;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

struct handlebars_string * handlebars_string_htmlspecialchars(
    struct handlebars_context * context,
    const char * str, size_t len
) {
    struct handlebars_string * string = handlebars_string_init(context, len);
    string = handlebars_string_htmlspecialchars_append(context, string, str, len);
    return handlebars_string_compact(string);
}

struct handlebars_string * handlebars_string_htmlspecialchars_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) {
    size_t new_len = len;
    size_t new_total;
    size_t source_offset = 0;
    bool source_is_internal;
    const char * p;
    const char * end;
    const struct htmlspecialchars_pair * pair;

    if( len <= 0 ) {
        return string;
    }

    source_is_internal = string_source_offset(string, str, len, &source_offset);

    // Calculate new size
    for( p = str, end = str + len; p < end; p++ ) {
        pair = &htmlspecialchars[(unsigned char) *p];
        if( pair->len ) {
            new_len = string_length_add(context, new_len, pair->len - 1);
        }
    }

    // If new len is equal to len, nothing to escape, just append
    if( new_len == len ) {
        return handlebars_string_append(context, string, str, len);
    }

    // Realloc original buffer
    new_total = string_length_add(context, string->len, new_len);
    string = string_reserve_for_write(context, string, new_total);
    if( source_is_internal ) {
        str = string->val + source_offset;
    }

    // Copy
    for( p = str, end = str + len; p < end; p++ ) {
        pair = &htmlspecialchars[(unsigned char) *p];
        if( pair->len ) {
            string = handlebars_string_append_unsafe(string, pair->str, pair->len);
        } else {
            string = handlebars_string_append_unsafe(string, p, 1);
        }
    }

    return string;
}

struct handlebars_string * handlebars_string_implode(
    struct handlebars_context * context,
    const char * sep,
    size_t sep_len,
    /*const*/ struct handlebars_string** parts
) {
    struct handlebars_string * string;
    /*const*/ struct handlebars_string** ptr;
    size_t len = 0;
    bool first = true;

    // Calc new size
    for( ptr = parts; *ptr; ptr++ ) {
        if( !first ) {
            len = string_length_add(context, len, sep_len);
        }
        len = string_length_add(context, len, (*ptr)->len);
        first = false;
    }
    if( first ) {
        return handlebars_string_init(context, 0);
    }

    // Allocate
    string = handlebars_string_init(context, len);

    // Append
    ptr = parts;
    string = handlebars_string_append_unsafe(string, (*ptr)->val, (*ptr)->len);
    for( ptr++; *ptr; ptr++ ) {
        string = handlebars_string_append_unsafe(string, sep, sep_len);
        string = handlebars_string_append_unsafe(string, (*ptr)->val, (*ptr)->len);
    }

    return string;
}

struct handlebars_string * handlebars_string_indent(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const struct handlebars_string * indent_str
) {
    struct handlebars_string * new_string = handlebars_string_init(context, string->len);
#ifndef HANDLEBARS_NO_REFCOUNT
    if (handlebars_rc_refcount(&string->rc) > 0) {
        handlebars_string_addref(new_string);
    }
#endif
    return handlebars_string_indent_append(context, new_string, string, indent_str);
}

struct handlebars_string * handlebars_string_indent_append(
    struct handlebars_context * context,
    struct handlebars_string * append_to_string,
    struct handlebars_string * input_string,
    const struct handlebars_string * indent_str
) {
    bool copied_input = append_to_string == input_string;
    const char * str;
    size_t str_len;
    bool endsInLine;
    size_t i;
    char tmp[2] = "\0";

    if( copied_input ) {
        input_string = handlebars_string_copy_ctor(context, input_string);
    }

    str = input_string->val;
    str_len = input_string->len;
    endsInLine = str_len > 0 && (str[str_len - 1] == '\n');

    if( endsInLine ) {
        str_len--;
    }

    append_to_string = handlebars_string_append_str(context, append_to_string, indent_str);

    for( i = 0; i < str_len; i++ ) {
        if( str[i] == '\n' ) {
            append_to_string = handlebars_string_append(context, append_to_string, HBS_STRL("\n"));
            append_to_string = handlebars_string_append_str(context, append_to_string, indent_str);
        } else {
            tmp[0] = str[i];
            append_to_string = handlebars_string_append(context, append_to_string, tmp, 1);
        }
    }

    if( endsInLine ) {
        append_to_string = handlebars_string_append(context, append_to_string, HBS_STRL("\n"));
    }

    if( copied_input ) {
        handlebars_talloc_free(input_string);
    } else {
        handlebars_string_delref(input_string);
    }

    return append_to_string;
}

struct handlebars_string * handlebars_string_ltrim(struct handlebars_string * string, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * ptr;

    assert(string != NULL);

    if( unlikely(string->len <= 0) ) {
        return string;
    }

    string = separate_string(string);

    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    ptr = string->val;
    while( *ptr && flags[(unsigned char) *ptr] ) {
        ++ptr;
        --string->len;
    }

    if( ptr > string->val ) {
        memmove(string->val, ptr, string->len + 1);
    }

    return string;
}

struct handlebars_string * handlebars_string_rtrim(struct handlebars_string * string, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * original;

    assert(string != NULL);

    if( unlikely(string->len <= 0) ) {
        return string;
    }

    string = separate_string(string);

    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    original = string->val + string->len;
    while( original > string->val && flags[(unsigned char) *--original] ) {
        --string->len;
        *original = '\0';
    }

    return string;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: et sw=4 ts=4
 */

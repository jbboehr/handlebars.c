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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <ctype.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



struct handlebars_string {
    size_t len;
    unsigned long hash;
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

size_t HANDLEBARS_STRING_SIZE = sizeof(struct handlebars_string);



// {{{ Accessors

char * hbs_str_val(struct handlebars_string * str) {
    return str->val;
}

size_t hbs_str_len(struct handlebars_string * str) {
    return str->len;
}

unsigned long hbs_str_hash(struct handlebars_string * str) {
    if (str->hash == 0) {
        str->hash = handlebars_string_hash(str->val, str->len);
    }
    return str->hash;
}

// }}} Accessors



unsigned long handlebars_string_hash_cont(const char * str, size_t len, unsigned long hash)
{
    unsigned long c;
    const unsigned char * ptr = (const unsigned char *) str;
    const unsigned char * end = ptr + len;
    for( c = *ptr++; ptr <= end && c; c = *ptr++ ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

unsigned long handlebars_string_hash(const char * str, size_t len)
{
    unsigned long hash = 5381;
    return handlebars_string_hash_cont(str, len, hash);
}

struct handlebars_string * handlebars_string_init(
    struct handlebars_context * context,
    size_t length
) {
    struct handlebars_string * st = handlebars_talloc_zero_size(context, HBS_STR_SIZE(length));
    HANDLEBARS_MEMCHECK(st, context);
    st->val[0] = 0;
    return st;
}

struct handlebars_string * handlebars_string_ctor_ex(
    struct handlebars_context * context,
    const char * str,
    size_t len,
    unsigned long hash
) {
    struct handlebars_string * st = handlebars_string_init(context, len + 1);
    HANDLEBARS_MEMCHECK(st, context);
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
    size_t size = HBS_STR_SIZE(string->len);
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    memcpy(st, string, size);
    return st;
}

struct handlebars_string * handlebars_string_extend(
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

struct handlebars_string * handlebars_string_append_unsafe(
    struct handlebars_string * string,
    const char * str, size_t len
) {
    memcpy(string->val + string->len, str, len);
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
    string = handlebars_string_extend(context, string, string->len + len);
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
    size_t size = HBS_STR_SIZE(string->len);
    if( talloc_get_size(string) > size ) {
        return (struct handlebars_string *) handlebars_talloc_realloc_size(NULL, string, size);
    } else {
        return string;
    }
}

bool handlebars_string_eq_ex(
    const char * string1, size_t length1, unsigned long hash1,
    const char * string2, size_t length2, unsigned long hash2
) {
    if( length1 != length2 ) {
        return false;
    } else {
        return HBS_STR_HASH_EX(string1, length1, hash1) == HBS_STR_HASH_EX(string2, length2, hash2);
    }
}

bool handlebars_string_eq(
    /*const*/ struct handlebars_string * string1,
    /*const*/ struct handlebars_string * string2
) {
    if( string1->len != string2->len ) {
        return false;
    } else {
        return HBS_STR_HASH(string1) == HBS_STR_HASH(string2);
    }
}

void handlebars_string_truncate(
    struct handlebars_string * string,
    size_t start,
    size_t end
) {
    // Truncate right
    if (end < string->len) {
        string->len = end;
    }
    // Truncate left
    if (start > 0) {
        memmove(string->val, string->val + start, string->len - start);
        string->len -= start;
    }
    string->val[string->len] = 0;
    string->hash = 0;
}

bool hbs_str_eq_strl(
    struct handlebars_string * string1,
    const char * str2,
    size_t len2
) {
    if (hbs_str_len(string1) != len2) {
        return false;
    }
    return 0 == strncmp(hbs_str_val(string1), str2, len2);
}

const char * handlebars_strnstr(const char * haystack, size_t haystack_len, const char * needle, size_t needle_len)
{
    const char * end = haystack + haystack_len - needle_len;

    if( !needle_len || !haystack_len ) {
        return NULL;
    }

    for( ; haystack <= end; haystack++ ) {
        if( haystack[0] == needle[0] && strncmp(haystack, needle, needle_len) == 0 ) {
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
    char * tok = string->val;

    if( replacement_len > search_len || search_len <= 0 || string->len <= 0 ) {
        return string;
    }

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

    if( search_len <= 0 || string->len <= 0 ) {
        return handlebars_string_copy_ctor(context, string);
    }

    struct handlebars_string *new_string = handlebars_string_init(context, string->len * 4 + 1);

    while( NULL != (tok = (char *) handlebars_strnstr(tok, string->len - (tok - string->val), search, search_len)) ) {
        new_string = handlebars_string_append(context, new_string, last_tok, tok - last_tok);
        new_string = handlebars_string_append(context, new_string, replacement, replacement_len);

        tok += search_len;
        last_tok = tok;
        if( tok >= string->val + string->len ) {
            break;
        }
    }

    new_string = handlebars_string_append(context, new_string, last_tok, string->len - (last_tok - string->val));

    assert(new_string->val[new_string->len] == 0);
    new_string->hash = 0;

    return handlebars_string_compact(new_string);
}

struct handlebars_string * handlebars_string_addcslashes(struct handlebars_context * context, struct handlebars_string * string, const char * what, size_t what_length)
{
    char flags[256];
    struct handlebars_string * new_string;
    char * source;
    char * target;
    char * end;
    size_t i;

    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    // Allocate new string
    new_string = handlebars_string_init(context, string->len * 4 + 1);

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
    char * source = string->val;
    char * target = string->val;
    char * end = string->val + string->len;
    size_t nlen = string->len;
    size_t i;
    char numtmp[4];

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
                    if( source+1 < end && isxdigit((int)(*(source+1))) ) {
                        numtmp[0] = *++source;
                        if( source+1 < end && isxdigit((int)(*(source+1))) ) {
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
                    /* no break */
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

struct handlebars_string * handlebars_string_asprintf(
    struct handlebars_context * context,
    const char * fmt,
    ...
) {
    va_list ap;
    struct handlebars_string * string;

    va_start(ap, fmt);
    string = handlebars_string_vasprintf_append(context, NULL, fmt, ap);
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
    return handlebars_string_vasprintf_append(context, NULL, fmt, ap);
}

struct handlebars_string * handlebars_string_vasprintf_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * fmt,
    va_list ap
) {
    va_list ap2;
    size_t len;
    size_t slen = string ? string->len : 0;

    // Calculate size
    va_copy(ap2, ap);
    len = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    // Nothing to do
    if( len <= 0 ) {
        return string;
    }

    // Resize
    string = handlebars_string_extend(context, string, slen + len);

    // Print
    va_copy(ap2, ap);
    vsnprintf(string->val + slen, len + 1, fmt, ap2);
    va_end(ap2);

    string->len += len;
    string->hash = 0;

    return string;
}

struct handlebars_string * handlebars_string_htmlspecialchars(
    struct handlebars_context * context,
    const char * str, size_t len
) {
    struct handlebars_string * string = handlebars_string_init(context, len * 4);
    string = handlebars_string_htmlspecialchars_append(context, string, str, len);
    return handlebars_string_compact(string);
}

struct handlebars_string * handlebars_string_htmlspecialchars_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    const char * str, size_t len
) {
    size_t new_len = len;
    const char * p;
    const char * end;
    const struct htmlspecialchars_pair * pair;

    if( len <= 0 ) {
        return string;
    }

    // Calculate new size
    for( p = str + len - 1; p >= str; p-- ) {
        pair = &htmlspecialchars[(unsigned char) *p];
        if( pair->len ) {
            new_len += pair->len - 1; // @todo check if sizeof works
        }
    }

    // If new len is equal to len, nothing to escape, just append
    if( new_len == len ) {
        return handlebars_string_append(context, string, str, len);
    }

    // Realloc original buffer
    string = handlebars_string_extend(context, string, string->len + new_len);

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

    // Calc new size
    for( ptr = parts; *ptr; ptr++ ) {
        len += (*ptr)->len + sep_len;
    }
    if( len > 0 ) {
        len -= sep_len;
    } else {
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
    const char * str, size_t str_len,
    const char * indent, size_t indent_len
) {
    struct handlebars_string * string = handlebars_string_init(context, str_len);
    bool endsInLine = (str[str_len - 1] == '\n');
    size_t i;
    char tmp[2] = "\0";

    if( endsInLine ) {
        str_len--;
    }

    string = handlebars_string_append(context, string, indent, indent_len);

    for( i = 0; i < str_len; i++ ) {
        if( str[i] == '\n' ) {
            string = handlebars_string_append(context, string, HBS_STRL("\n"));
            string = handlebars_string_append(context, string, indent, indent_len);
        } else {
            tmp[0] = str[i];
            string = handlebars_string_append(context, string, tmp, 1);
        }
    }

    if( endsInLine ) {
        string = handlebars_string_append(context, string, HBS_STRL("\n"));
    }

    return string;
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

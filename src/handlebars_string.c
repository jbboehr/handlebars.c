
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
        memcpy(tok, replacement, replacement_len);
        memcpy(tok + replacement_len, tok + search_len, string->len - search_len - (tok - string->val));
        memset(string->val + string->len - search_len + replacement_len, 0, 1);
        string->len -= search_len - replacement_len;

        tok += replacement_len;
        if( tok >= string->val + string->len ) {
            break;
        }
    }

    assert(string->val[string->len] == 0);

    return string;
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
            *target++=*source;
        }
    }

    //if (nlen != 0) {
        *target = '\0';
    //}

    string->len = nlen;

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
    return string;
}

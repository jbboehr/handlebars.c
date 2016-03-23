
#ifndef HANDLEBARS_STRING_H
#define HANDLEBARS_STRING_H

#include <assert.h>
#include <stddef.h>
#include <memory.h>

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
    struct handlebars_string * st = handlebars_talloc_size(context, offsetof(struct handlebars_string, val) + size);
    HANDLEBARS_MEMCHECK(st, context);
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
    size_t size = offsetof(struct handlebars_string, val) + string->len + 1;
    struct handlebars_string * st = handlebars_talloc_size(context, size);
    HANDLEBARS_MEMCHECK(st, context);
    memcpy(st, string, size);
    return st;
}

static inline struct handlebars_string * handlebars_string_append(struct handlebars_context * context, struct handlebars_string * st2, const char * str, size_t len)
{
    unsigned long newhash = handlebars_string_hash_cont((const unsigned char *)str, len, st2->hash);
    size_t newsize = offsetof(struct handlebars_string, val) + st2->len + len + 1;
    if( newsize > talloc_get_size(st2) ) {
        st2 = (struct handlebars_string *) talloc_realloc_size(context, st2, newsize);
        HANDLEBARS_MEMCHECK(st2, context);
    }
    memcpy(st2->val + st2->len, str, len);
    st2->len += len;
    st2->val[st2->len] = 0;
    st2->hash = newhash;
    return st2;
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

#ifdef	__cplusplus
}
#endif

#endif

#ifndef HANDLEBARS_STRING_H
#define HANDLEBARS_STRING_H

#include <stddef.h>
#include "handlebars.h"
#include "handlebars_memory.h"

#define HBS_STRVAL(str) str->val
#define HBS_STRLEN(str) str->len

struct handlebars_string {
    size_t len;
    unsigned long hash;
    char val[];
};

static inline unsigned long handlebars_string_hash_cont(unsigned char * str, unsigned long hash)
{
    int c;
    while( c = *str++ ) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static inline unsigned long handlebars_string_hash(unsigned char * str)
{
    unsigned long hash = 5381;
    return handlebars_string_hash_cont(str, hash);
}

static inline struct handlebars_string * handlebars_string_ctor_ex(struct handlebars_context * context, const char * str, size_t len, unsigned long hash)
{
    struct handlebars_string * st = handlebars_talloc_size(context, offsetof(struct handlebars_string, val) + len + 1);
    HANDLEBARS_MEMCHECK(st, context);
    st->len = len;
    memcpy(st->val, str, len);
    st->val[st->len] = 0;
    st->hash = hash;
    return st;
}

static inline struct handlebars_string * handlebars_string_ctor(struct handlebars_context * context, const char * str, size_t len)
{
    return handlebars_string_ctor_ex(context, str, len, handlebars_string_hash(str));
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
    unsigned long newhash = handlebars_string_hash_cont(str, st2->hash);
    st2 = (struct handlebars_string *) handlebars_talloc_strndup_append_buffer((void *) st2, str, len);
    HANDLEBARS_MEMCHECK(st2, context);
    st2->len += len;
    st2->val[st2->len] = 0;
    st2->hash = newhash;
    return st2;
}

#endif
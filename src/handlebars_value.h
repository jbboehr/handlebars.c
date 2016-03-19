
#ifndef HANDLEBARS_VALUE_H
#define HANDLEBARS_VALUE_H

#include "handlebars.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value_handlers.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;
struct handlebars_map;
struct handlebars_options;
struct handlebars_stack;
struct handlebars_value;
struct handlebars_value_handlers;
struct json_object;
struct yaml_document_s;
struct yaml_node_s;

enum handlebars_value_type {
    HANDLEBARS_VALUE_TYPE_NULL = 0,
    HANDLEBARS_VALUE_TYPE_BOOLEAN = 1,
    HANDLEBARS_VALUE_TYPE_INTEGER = 2,
    HANDLEBARS_VALUE_TYPE_FLOAT = 3,
    HANDLEBARS_VALUE_TYPE_STRING = 4,
    HANDLEBARS_VALUE_TYPE_ARRAY = 5,
    HANDLEBARS_VALUE_TYPE_MAP = 6,
    HANDLEBARS_VALUE_TYPE_USER = 7,
    HANDLEBARS_VALUE_TYPE_PTR = 8,
    HANDLEBARS_VALUE_TYPE_HELPER = 9
};

enum handlebars_value_flags {
    HANDLEBARS_VALUE_FLAG_NONE = 0,
    HANDLEBARS_VALUE_FLAG_SAFE_STRING = 1,
    HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED = 2
};

struct handlebars_value_iterator {
    size_t length;
    size_t index;
    struct handlebars_string * key;
    struct handlebars_value * value;
    struct handlebars_value * current;
    void * usr;
};

struct handlebars_value_user {
    struct handlebars_value_handlers * handlers;
    void * ptr;
};

struct handlebars_value {
	enum handlebars_value_type type;
    unsigned long flags;
	union {
        long lval;
        bool bval;
        double dval;
        struct handlebars_string * string;
        struct handlebars_map * map;
        struct handlebars_stack * stack;
        struct handlebars_value_user usr;
        void * ptr;
        handlebars_helper_func helper;
        struct handlebars_options * options;
	} v;
    int refcount;
	struct handlebars_context * ctx;
};

enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value);
char * handlebars_value_get_strval(struct handlebars_value * value) HBS_ATTR_RETURNS_NONNULL;
size_t handlebars_value_get_strlen(struct handlebars_value * value);
bool handlebars_value_get_boolval(struct handlebars_value * value);
long handlebars_value_get_intval(struct handlebars_value * value);
double handlebars_value_get_floatval(struct handlebars_value * value);

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index);

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, struct handlebars_string * key);
struct handlebars_value * handlebars_value_map_str_find(struct handlebars_value * value, const char * key, size_t len);

char * handlebars_value_expression(struct handlebars_value * value, bool escape) HBS_ATTR_RETURNS_NONNULL;
char * handlebars_value_expression_append_buffer(char * buf, struct handlebars_value * value, bool escape) HBS_ATTR_RETURNS_NONNULL;
char * handlebars_value_dump(struct handlebars_value * value, size_t depth) HBS_ATTR_RETURNS_NONNULL;

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx) HBS_ATTR_RETURNS_NONNULL;
struct handlebars_value * handlebars_value_copy(struct handlebars_value * value) HBS_ATTR_RETURNS_NONNULL;
void handlebars_value_dtor(struct handlebars_value * value);

#define handlebars_value_convert(value) handlebars_value_convert_ex(value, 1);
void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse);
bool handlebars_value_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value);
bool handlebars_value_iterator_next(struct handlebars_value_iterator * it);

struct handlebars_value * handlebars_value_call(
        struct handlebars_value * value,
        int argc,
        struct handlebars_value * argv[],
        struct handlebars_options * options
);

#if 0
static inline int _handlebars_value_addref(struct handlebars_value * value, const char * loc) {
    fprintf(stderr, "ADDREF [%p] [%d] %s\n", value, value->refcount, loc);
    return ++value->refcount;
}
#define handlebars_value_addref(value) _handlebars_value_addref(value, "[" HBS_S2(__FILE__) ":" HBS_S2(__LINE__) "]")
#elif !defined(HANDLEBARS_NO_REFCOUNT)
static inline int handlebars_value_addref(struct handlebars_value * value) {
    return ++value->refcount;
}
static inline struct handlebars_value * handlebars_value_addref2(struct handlebars_value * value) {
    handlebars_value_addref(value);
    return value;
}
#else
#define handlebars_value_addref
#define handlebars_value_addref2(value) (value)
#endif

#if 0
static inline int _handlebars_value_delref(struct handlebars_value * value, const char * loc) {
    fprintf(stderr, "DELREF [%p] [%d] %s\n", value, value->refcount, loc);
    if( value->refcount <= 1 ) {
        handlebars_value_dtor(value);
        if( value->flags & HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED ) {
            handlebars_talloc_free(value);
        }
        return 0;
    }
    return --value->refcount;
}
#define handlebars_value_delref(value) _handlebars_value_delref(value, "[" HBS_S2(__FILE__) ":" HBS_S2(__LINE__) "]")
#elif !defined(HANDLEBARS_NO_REFCOUNT)
static inline int handlebars_value_delref(struct handlebars_value * value) {
    if( value->refcount <= 1 ) {
        handlebars_value_dtor(value);
        if( value->flags & HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED ) {
            handlebars_talloc_free(value);
        }
        return 0;
    }
    return --value->refcount;
}
#else
#define handlebars_value_delref
#endif

#if !defined(HANDLEBARS_NO_REFCOUNT)
static inline int handlebars_value_try_delref(struct handlebars_value * value) {
    if( value ) {
        return handlebars_value_delref(value);
    }
    return -1;
}
#else
#define handlebars_value_try_delref
#endif

#if !defined(HANDLEBARS_NO_REFCOUNT)
static inline int handlebars_value_try_addref(struct handlebars_value * value) {
    if( value ) {
        return handlebars_value_addref(value);
    }
    return -1;
}
#else
#define handlebars_value_try_addref
#endif

#if !defined(HANDLEBARS_NO_REFCOUNT)
static inline int handlebars_value_refcount(struct handlebars_value * value) {
    return value->refcount;
}
#else
#define handlebars_value_refcount(v) 999
#endif

static inline struct handlebars_value_handlers * handlebars_value_get_handlers(struct handlebars_value * value) {
    return value->v.usr.handlers;
}

static inline void handlebars_value_init(struct handlebars_context * ctx, struct handlebars_value * value)
{
    value->ctx = ctx;
}

static inline bool handlebars_value_is_scalar(struct handlebars_value * value) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
        case HANDLEBARS_VALUE_TYPE_FLOAT:
        case HANDLEBARS_VALUE_TYPE_INTEGER:
        case HANDLEBARS_VALUE_TYPE_STRING:
            return 1;
        default:
            return 0;
    }
}

static inline bool handlebars_value_is_callable(struct handlebars_value * value ) {
    return handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_HELPER;
}

static inline long handlebars_value_count(struct handlebars_value * value) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_length(value->v.stack);
        case HANDLEBARS_VALUE_TYPE_MAP:
            return value->v.map->i;
        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->count(value);
        default:
            return -1;
    }
}

static inline bool handlebars_value_is_empty(struct handlebars_value * value) {
    return !handlebars_value_get_boolval(value);
}

static inline void handlebars_value_null(struct handlebars_value * value) {
    handlebars_value_dtor(value);
}

static inline void handlebars_value_boolean(struct handlebars_value * value, bool bval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_BOOLEAN;
    value->v.bval = bval;
}

static inline void handlebars_value_integer(struct handlebars_value * value, long lval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_INTEGER;
    value->v.lval = lval;
}

static inline void handlebars_value_float(struct handlebars_value * value, double dval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_FLOAT;
    value->v.dval = dval;
}

static inline void handlebars_value_str(struct handlebars_value * value, struct handlebars_string * string)
{
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = handlebars_string_copy_ctor(value->ctx, string);
}

static inline void handlebars_value_string(struct handlebars_value * value, const char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen(strval))/*)*/;
}


static inline void handlebars_value_string_steal(struct handlebars_value * value, char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen(strval))/*)*/;
    talloc_free(strval); // meh
}

static inline void handlebars_value_stringl(struct handlebars_value * value, const char * strval, size_t strlen) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.string = /*talloc_steal(value,*/ handlebars_string_ctor(value->ctx, strval, strlen)/*)*/;
}

static inline void handlebars_value_map_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = /*talloc_steal(value,*/ handlebars_map_ctor(value->ctx)/*)*/;
}

static inline void handlebars_value_array_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    value->v.stack = /*talloc_steal(value,*/ handlebars_stack_ctor(value->ctx)/*)*/;
}

#ifdef	__cplusplus
}
#endif

#endif

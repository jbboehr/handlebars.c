
#ifndef HANDLEBARS_VALUE_H
#define HANDLEBARS_VALUE_H

#include "handlebars.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_stack.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_map;
struct handlebars_options;
struct handlebars_stack;
struct handlebars_value;
struct handlebars_value_handlers;
struct json_object;

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
    HANDLEBARS_VALUE_TYPE_HELPER = 9,
    HANDLEBARS_VALUE_TYPE_OPTIONS = 10
};

enum handlebars_value_flags {
    HANDLEBARS_VALUE_FLAG_NONE = 0,
    HANDLEBARS_VALUE_FLAG_TALLOC_DTOR = 1,
    HANDLEBARS_VALUE_FLAG_SAFE_STRING = 2
};

struct handlebars_value_iterator {
    size_t index;
    const char * key;
    struct handlebars_value * value;
    struct handlebars_value * current;
    void * usr;
};

struct handlebars_value {
	enum handlebars_value_type type;
    unsigned long flags;
	struct handlebars_value_handlers * handlers;
	union {
		long lval;
        bool bval;
		double dval;
        char * strval;
        struct handlebars_map * map;
        struct handlebars_stack * stack;
		void * usr;
		void * ptr;
        handlebars_helper_func helper;
        struct handlebars_options * options;
	} v;
    int refcount;
	void * ctx;
};

enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value);
struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, const char * key);
struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index);
const char * handlebars_value_get_strval(struct handlebars_value * value);
size_t handlebars_value_get_strlen(struct handlebars_value * value);
bool handlebars_value_get_boolval(struct handlebars_value * value);
long handlebars_value_get_intval(struct handlebars_value * value);
double handlebars_value_get_floatval(struct handlebars_value * value);

char * handlebars_value_expression(void * ctx, struct handlebars_value * value, bool escape);

struct handlebars_value * handlebars_value_ctor(void * ctx);
struct handlebars_value * handlebars_value_copy(struct handlebars_value * value);
void handlebars_value_dtor(struct handlebars_value * value);
struct handlebars_value * handlebars_value_from_json_string(void *ctx, const char * json);
struct handlebars_value * handlebars_value_from_json_object(void *ctx, struct json_object *json);

#define handlebars_value_convert(value) handlebars_value_convert_ex(value, 1);
void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse);
struct handlebars_value_iterator * handlebars_value_iterator_ctor(struct handlebars_value * value);
bool handlebars_value_iterator_next(struct handlebars_value_iterator * it);

char * handlebars_value_dump(struct handlebars_value * value, size_t depth);

static inline int handlebars_value_addref(struct handlebars_value * value) {
    return ++value->refcount;
}

static inline int handlebars_value_delref(struct handlebars_value * value) {
    if( value->refcount <= 1 ) {
        if( !(value->flags & HANDLEBARS_VALUE_FLAG_TALLOC_DTOR) ) {
            handlebars_value_dtor(value);
        }
        handlebars_talloc_free(value);
        return 0;
    }
    return --value->refcount;
}

static inline int handlebars_value_refcount(struct handlebars_value * value) {
    return value->refcount;
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

static inline bool handlebars_value_is_empty(struct handlebars_value * value) {
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            return 1;
        case HANDLEBARS_VALUE_TYPE_BOOLEAN:
            return 0 == handlebars_value_get_boolval(value);
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return 0 == handlebars_value_get_floatval(value);
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return 0 == handlebars_value_get_intval(value);
        case HANDLEBARS_VALUE_TYPE_STRING:
            return 0 == handlebars_value_get_strlen(value);
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return 0 == handlebars_stack_length(value->v.stack);
        case HANDLEBARS_VALUE_TYPE_MAP:
            return NULL == value->v.map->first;
        case HANDLEBARS_VALUE_TYPE_USER:
        default:
            return 0;
    }
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

static inline void handlebars_value_string(struct handlebars_value * value, const char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.strval = handlebars_talloc_strdup(value, strval);
}

static inline void handelbars_value_string_steal(struct handlebars_value * value, char * strval) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.strval = talloc_steal(value, strval);
}

static inline void handlebars_value_stringl(struct handlebars_value * value, const char * strval, size_t strlen) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_STRING;
    value->v.strval = handlebars_talloc_strndup(value, strval, strlen);
}

static inline void handlebars_value_map_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_MAP;
    value->v.map = handlebars_map_ctor(value);
}

static inline void handlebars_value_array_init(struct handlebars_value * value) {
    handlebars_value_null(value);
    value->type = HANDLEBARS_VALUE_TYPE_ARRAY;
    value->v.stack = handlebars_stack_ctor(value);
}

#ifdef	__cplusplus
}
#endif

#endif

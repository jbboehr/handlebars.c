
#ifndef HANDLEBARS_DATA_H
#define HANDLEBARS_DATA_H

#include <stddef.h>

struct handlebars_value;
struct json_object;

enum handlebars_value_type {
	HANDLEBARS_VALUE_TYPE_NULL = 0,
	HANDLEBARS_VALUE_TYPE_MAP,
	HANDLEBARS_VALUE_TYPE_ARRAY,
	HANDLEBARS_VALUE_TYPE_STRING,
	HANDLEBARS_VALUE_TYPE_BOOLEAN,
	HANDLEBARS_VALUE_TYPE_FLOAT,
	HANDLEBARS_VALUE_TYPE_INTEGER,
	HANDLEBARS_VALUE_TYPE_USER
};

typedef enum handlebars_value_type (*handlebars_value_type_func)(struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, const char * key, size_t len);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index);
typedef const char * (*handlebars_strval_func)(struct handlebars_value * value);
typedef size_t (*handlebars_strlen_func)(struct handlebars_value * value);
typedef short (*handlebars_boolbal_func)(struct handlebars_value * value);
typedef long (*handlebars_intval_func)(struct handlebars_value * value);
typedef double (*handlebars_floatval_func)(struct handlebars_value * value);

struct handlebars_value_handlers {
	handlebars_value_type_func type;
	handlebars_map_find_func map_find;
	handlebars_array_find_func array_find;
	handlebars_strval_func strval;
	handlebars_strlen_func strlen;
	handlebars_boolbal_func boolval;
	handlebars_intval_func intval;
	handlebars_floatval_func floatval;
};

struct handlebars_value {
	enum handlebars_value_type type;
	struct handlebars_value_handlers * handlers;
	union {
		long lval;
		double dval;
		short bval;
		union {
			char * str;
			size_t len;
		} strval;
		void * usr;
	} v;
};



enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value);
struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, const char * key, size_t len);
struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index);
const char * handlebars_value_get_strval(struct handlebars_value * value);
size_t handlebars_value_get_strlen(struct handlebars_value * value);
short handlebars_value_get_boolval(struct handlebars_value * value);
long handlebars_value_get_intval(struct handlebars_value * value);
double handlebars_value_get_floatval(struct handlebars_value * value);


char * handlebars_value_expression(void * ctx, struct handlebars_value * value, short escape);



struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void);

struct handlebars_value * handlebars_value_from_json_string(void *ctx, const char * json);
struct handlebars_value * handlebars_value_from_json_object(void *ctx, struct json_object *json);
struct handlebars_value * handlebars_value_ctor(void * ctx);

#endif

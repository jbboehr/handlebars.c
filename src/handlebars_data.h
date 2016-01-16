
#ifndef HANDLEBARS_DATA_H
#define HANDLEBARS_DATA_H

#include <stddef.h>

struct handlebars_value;

enum handlebars_value_type {
	HANDLEBARS_VALUE_TYPE_NULL = 0,
	HANDLEBARS_VALUE_TYPE_MAP,
	HANDLEBARS_VALUE_TYPE_ARRAY,
	HANDLEBARS_VALUE_TYPE_STRING,
	HANDLEBARS_VALUE_TYPE_BOOLEAN,
	HANDLEBARS_VALUE_TYPE_NUMBER
};

typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, const char * key, size_t len);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index);
typedef const char * (*handlebars_strval_func)(struct handlebars_value * value);
typedef size_t (*handlebars_strlen_func)(struct handlebars_value * value);
typedef short (*handlebars_boolbal_func)(struct handlebars_value * value);
typedef long (*handlebars_longval_func)(struct handlebars_value * value);

struct handlevars_value_handlers {
	handlebars_map_find_func map_get;
	handlebars_array_find_func array_get;
	handlebars_strval_func string_value;
	handlebars_strlen_func string_length;
	handlebars_boolbal_func boolean_value;
	handlebars_longval_func long_value;
};

struct handlebars_value {
	enum handlebars_value_type type;
	void *usr;
	struct handlevars_value_handlers handlers;
};

#endif

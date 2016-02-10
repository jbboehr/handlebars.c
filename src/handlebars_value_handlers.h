
#ifndef HANDLEBARS_VALUE_HANDLERS_H
#define HANDLEBARS_VALUE_HANDLERS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_value_iterator;

typedef struct handlebars_value * (*handlebars_copy_func)(struct handlebars_value * value);
typedef void (*handlebars_value_dtor_func)(struct handlebars_value * value);
typedef void (*handlebars_value_convert_func)(struct handlebars_value * value, bool recurse);
typedef enum handlebars_value_type (*handlebars_value_type_func)(struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, const char * key);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index);
typedef struct handlebars_value_iterator * (*handlebars_iterator_ctor_func)(struct handlebars_value * value);
typedef bool (*handlebars_iterator_next_func)(struct handlebars_value_iterator * it);

struct handlebars_value_handlers {
    handlebars_copy_func copy;
    handlebars_value_dtor_func dtor;
    handlebars_value_convert_func convert;
    handlebars_value_type_func type;
    handlebars_map_find_func map_find;
    handlebars_array_find_func array_find;
    handlebars_iterator_ctor_func iterator;
    handlebars_iterator_next_func next;
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void) HBSARN;

#ifdef	__cplusplus
}
#endif

#endif

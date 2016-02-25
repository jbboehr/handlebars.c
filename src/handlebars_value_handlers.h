
#ifndef HANDLEBARS_VALUE_HANDLERS_H
#define HANDLEBARS_VALUE_HANDLERS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_string;
struct handlebars_value_iterator;

typedef struct handlebars_value * (*handlebars_copy_func)(struct handlebars_value * value);
typedef void (*handlebars_value_dtor_func)(struct handlebars_value * value);
typedef void (*handlebars_value_convert_func)(struct handlebars_value * value, bool recurse);
typedef enum handlebars_value_type (*handlebars_value_type_func)(struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, struct handlebars_string * key);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index);
typedef bool (*handlebars_iterator_init_func)(struct handlebars_value_iterator * it, struct handlebars_value * value);
typedef bool (*handlebars_iterator_next_func)(struct handlebars_value_iterator * it);
typedef struct handlebars_value * (*handlebars_call_func)(struct handlebars_value * value, struct handlebars_options * options);
typedef long (*handlebars_count_func)(struct handlebars_value * value);

struct handlebars_value_handlers {
    handlebars_copy_func copy;
    handlebars_value_dtor_func dtor;
    handlebars_value_convert_func convert;
    handlebars_value_type_func type;
    handlebars_map_find_func map_find;
    handlebars_array_find_func array_find;
    handlebars_iterator_init_func iterator;
    handlebars_iterator_next_func next;
    handlebars_call_func call;
    handlebars_count_func count;
};

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void) HBSARN;

#ifdef	__cplusplus
}
#endif

#endif

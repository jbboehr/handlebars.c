
#ifndef HANDLEBARS_VALUE_HANDLERS_H
#define HANDLEBARS_VALUE_HANDLERS_H

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

void handlebars_value_json_dtor(struct handlebars_value * value);
struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void);

#endif

#ifndef HANDLEBARS_HELPERS_H
#define HANDLEBARS_HELPERS_H

#include <stddef.h>

struct handlebars_stack;
struct handlebars_value;
struct handlebars_vm;

struct handlebars_options {
    int inverse;
    int program;
    char * name;
    struct handlebars_stack * params;
    struct handlebars_value * scope;
    struct handlebars_value * data;
    struct handlebars_value * hash;
    struct handlebars_vm * vm;
};

typedef struct handlebars_value * (*handlebars_helper_func)(struct handlebars_options * options);

#endif
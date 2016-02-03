
#ifndef HANDLEBARS_HELPERS_H
#define HANDLEBARS_HELPERS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_stack;
struct handlebars_value;
struct handlebars_vm;

struct handlebars_options {
    long inverse;
    long program;
    char * name;
    struct handlebars_stack * params;
    struct handlebars_value * scope;
    struct handlebars_value * data;
    struct handlebars_value * hash;
    struct handlebars_vm * vm;
};

typedef struct handlebars_value * (*handlebars_helper_func)(struct handlebars_options * options);

#ifdef	__cplusplus
}
#endif

#endif


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
    struct handlebars_value * hash_types;
    struct handlebars_value * hash_contexts;
    struct handlebars_vm * vm;
};

typedef struct handlebars_value * (*handlebars_helper_func)(struct handlebars_options * options);

const char ** handlebars_builtins_names(void) HBSARN;
struct handlebars_value * handlebars_builtins(struct handlebars_context * ctx);

struct handlebars_value * handlebars_builtin_block_helper_missing(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_each(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_helper_missing(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_lookup(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_if(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_unless(struct handlebars_options * options) HBSARN;
struct handlebars_value * handlebars_builtin_with(struct handlebars_options * options) HBSARN;

#ifdef	__cplusplus
}
#endif

#endif

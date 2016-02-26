
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
    struct handlebars_vm * vm;
    long inverse;
    long program;
    struct handlebars_string * name;
    struct handlebars_value * scope;
    struct handlebars_value * data;
    struct handlebars_value * hash;
};

typedef struct handlebars_value * (*handlebars_helper_func)(
        int argc,
        struct handlebars_value * argv[],
        struct handlebars_options * options
);

#define HANDLEBARS_HELPER_ARGS int argc, struct handlebars_value * argv[], struct handlebars_options * options

void handlebars_options_deinit(struct handlebars_options * options);
const char ** handlebars_builtins_names(void) HBSARN;
handlebars_helper_func * handlebars_builtins();
handlebars_helper_func handlebars_builtins_find(const char * str, unsigned int len);

struct handlebars_value * handlebars_builtin_block_helper_missing(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_each(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_helper_missing(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_lookup(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_log(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_if(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_unless(HANDLEBARS_HELPER_ARGS) HBSARN;
struct handlebars_value * handlebars_builtin_with(HANDLEBARS_HELPER_ARGS) HBSARN;

#ifdef	__cplusplus
}
#endif

#endif

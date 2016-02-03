
#ifndef HANDLEBARS_BUILTINS_H
#define HANDLEBARS_BUILTINS_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_options;
struct handlebars_value;

struct handlebars_value * handlebars_builtin_block_helper_missing(struct handlebars_options * options);

const char ** handlebars_builtins_names();
struct handlebars_value * handlebars_builtins(void * ctx);

#ifdef	__cplusplus
}
#endif

#endif

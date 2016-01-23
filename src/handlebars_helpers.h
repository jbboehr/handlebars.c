
#ifndef HANDLEBARS_HELPERS_H
#define HANDLEBARS_HELPERS_H

struct handlebars_value;

struct handlebars_options {
    size_t argc;
    struct handlebars_value ** argv;
};

typedef struct handlebars_value * (*handlebars_helper_func)(struct handlebars_options * options);

#endif

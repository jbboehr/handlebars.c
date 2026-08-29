#ifndef HANDLEBARSC_HELPERS_H
#define HANDLEBARSC_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

struct handlebars_compiler;
struct handlebars_context;
struct handlebars_vm;

enum handlebarsc_helper_mode {
    handlebarsc_helper_mode_exec = 0,
    handlebarsc_helper_mode_json = 1
};

struct handlebarsc_helper_registration {
    char * name;
    char * command;
    enum handlebarsc_helper_mode mode;
};

struct handlebarsc_helper_registry {
    void * ctx;
    struct handlebarsc_helper_registration * registrations;
    size_t count;
    size_t capacity;
    unsigned long timeout_ms;
    size_t output_limit;
    bool allow_override;
    bool options_used;
};

void handlebarsc_helper_registry_init(
    struct handlebarsc_helper_registry * registry,
    void * ctx
);

bool handlebarsc_helper_registry_add(
    struct handlebarsc_helper_registry * registry,
    enum handlebarsc_helper_mode mode,
    const char * specification,
    const char ** error
);

bool handlebarsc_helper_registry_validate(
    struct handlebarsc_helper_registry * registry,
    bool supported_mode,
    const char ** error
);

void handlebarsc_helper_registry_apply_compiler(
    struct handlebarsc_helper_registry * registry,
    struct handlebars_context * context,
    struct handlebars_compiler * compiler
);

void handlebarsc_helper_registry_apply_vm(
    struct handlebarsc_helper_registry * registry,
    struct handlebars_vm * vm
);

#endif

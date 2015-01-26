
#ifndef HANDLEBARS_TOKEN_H
#define HANDLEBARS_TOKEN_H

#include <stdlib.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_token {
    int token;
    size_t length;
    char * text;
};

struct handlebars_token * handlebars_token_ctor(int token_int, const char * text, size_t length, void * ctx);

void handlebars_token_dtor(struct handlebars_token * token);

const char * handlebars_token_readable_type(int type);

int handlebars_token_reverse_readable_type(const char * type);

#ifdef	__cplusplus
}
#endif

#endif

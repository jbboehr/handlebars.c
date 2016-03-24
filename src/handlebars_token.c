
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"


#undef CONTEXT
#define CONTEXT context

struct handlebars_token * handlebars_token_ctor(struct handlebars_context * context, int token_int, struct handlebars_string * string)
{
    struct handlebars_token * token = MC(handlebars_talloc_zero(context, struct handlebars_token));
    token->token = token_int;
    token->string = talloc_steal(token, string); // probably safe
    return token;
}

void handlebars_token_dtor(struct handlebars_token * token)
{
    assert(token != NULL);

    handlebars_talloc_free(token);
}

int handlebars_token_get_type(struct handlebars_token * token)
{
    if( likely(token != NULL) ) {
        return token->token;
    } else {
        return -1;
    }
}

struct handlebars_string * handlebars_token_get_text(struct handlebars_token * token)
{
    if( likely(token != NULL) ) {
        return token->string;
    } else {
        return NULL;
    }
}

const char * handlebars_token_readable_type(int type)
{
#define _RTYPE_STR(x) #x
#define _RTYPE_CASE(str) \
    case str: return _RTYPE_STR(str)
  switch( type ) {
    _RTYPE_CASE(BOOLEAN);
    _RTYPE_CASE(CLOSE);
    _RTYPE_CASE(CLOSE_RAW_BLOCK);
    _RTYPE_CASE(CLOSE_SEXPR);
    _RTYPE_CASE(CLOSE_UNESCAPED);
    _RTYPE_CASE(COMMENT);
    _RTYPE_CASE(CONTENT);
    _RTYPE_CASE(DATA);
    _RTYPE_CASE(END);
    _RTYPE_CASE(END_RAW_BLOCK);
    _RTYPE_CASE(EQUALS);
    _RTYPE_CASE(ID);
    _RTYPE_CASE(INVALID);
    _RTYPE_CASE(INVERSE);
    _RTYPE_CASE(NUMBER);
    _RTYPE_CASE(OPEN);
    _RTYPE_CASE(OPEN_BLOCK);
    _RTYPE_CASE(OPEN_ENDBLOCK);
    _RTYPE_CASE(OPEN_INVERSE);
    _RTYPE_CASE(OPEN_PARTIAL);
    _RTYPE_CASE(OPEN_RAW_BLOCK);
    _RTYPE_CASE(OPEN_SEXPR);
    _RTYPE_CASE(OPEN_UNESCAPED);
    _RTYPE_CASE(SEP);
    _RTYPE_CASE(STRING);
    // Added in v3
    _RTYPE_CASE(CLOSE_BLOCK_PARAMS);
    _RTYPE_CASE(OPEN_BLOCK_PARAMS);
    _RTYPE_CASE(OPEN_INVERSE_CHAIN);
    _RTYPE_CASE(UNDEFINED);
    // Added in v4
    _RTYPE_CASE(OPEN_PARTIAL_BLOCK);
    case NUL: return "NULL";
  }
  
  return "UNKNOWN";
}

int handlebars_token_reverse_readable_type(const char * type)
{
#define _RTYPE_REV_STR(x) #x
#define _RTYPE_REV_CMP(str) \
    if( strcmp(type, _RTYPE_REV_STR(str)) == 0 ) { \
        return str; \
    }
    switch( type[0] ) {
        case 'B':
            _RTYPE_REV_CMP(BOOLEAN);
            break;
        case 'C':
            _RTYPE_REV_CMP(CLOSE);
            _RTYPE_REV_CMP(CLOSE_RAW_BLOCK);
            _RTYPE_REV_CMP(CLOSE_SEXPR);
            _RTYPE_REV_CMP(CLOSE_UNESCAPED);
            _RTYPE_REV_CMP(COMMENT);
            _RTYPE_REV_CMP(CONTENT);
            // Added in v3
            _RTYPE_REV_CMP(CLOSE_BLOCK_PARAMS);
            break;
        case 'D':
            _RTYPE_REV_CMP(DATA);
            break;
        case 'E':
            _RTYPE_REV_CMP(END);
            _RTYPE_REV_CMP(END_RAW_BLOCK);
            _RTYPE_REV_CMP(EQUALS);
            break;
        case 'I':
            _RTYPE_REV_CMP(ID);
            _RTYPE_REV_CMP(INVALID);
            _RTYPE_REV_CMP(INVERSE);
            break;
        case 'N':
            _RTYPE_REV_CMP(NUMBER);
            if( strcmp(type, "NULL") == 0 ) {
                return NUL;
            }
            break;
        case 'O':
            _RTYPE_REV_CMP(OPEN);
            _RTYPE_REV_CMP(OPEN_BLOCK);
            _RTYPE_REV_CMP(OPEN_ENDBLOCK);
            _RTYPE_REV_CMP(OPEN_INVERSE);
            _RTYPE_REV_CMP(OPEN_PARTIAL);
            _RTYPE_REV_CMP(OPEN_RAW_BLOCK);
            _RTYPE_REV_CMP(OPEN_SEXPR);
            _RTYPE_REV_CMP(OPEN_UNESCAPED);
            // Added in v3
            _RTYPE_REV_CMP(OPEN_INVERSE_CHAIN);
            _RTYPE_REV_CMP(OPEN_BLOCK_PARAMS);
            // Added in v4
            _RTYPE_REV_CMP(OPEN_PARTIAL_BLOCK);
            break;
        case 'S':
            _RTYPE_REV_CMP(SEP);
            _RTYPE_REV_CMP(STRING);
            break;
        case 'U':
        	// Added in v3
            _RTYPE_REV_CMP(UNDEFINED);
            break;
    }
    
    // Unknown :(
    return -1;
}

struct handlebars_string * handlebars_token_print(
        struct handlebars_context * context,
        struct handlebars_token * token,
        int flags
) {
    const char * name = handlebars_token_readable_type(token->token);
    size_t name_len = strlen(name);
    const char * sep = flags & handlebars_token_print_flag_newlines ? "\n" : " ";
    struct handlebars_string * text = handlebars_string_addcslashes(context, token->string, HBS_STRL("\r\n\t\v"));
    size_t size = name_len + 2 + text->len + 1 + 1;
    struct handlebars_string * ret = handlebars_string_init(context, size);
    handlebars_string_append(context, ret, name, name_len);
    handlebars_string_append(context, ret, HBS_STRL(" ["));
    handlebars_string_append(context, ret, text->val, text->len);
    handlebars_string_append(context, ret, HBS_STRL("]"));
    handlebars_string_append(context, ret, sep, 1);
    handlebars_talloc_free(text);
    return ret;
}

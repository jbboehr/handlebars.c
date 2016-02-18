
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"


#undef CONTEXT
#define CONTEXT parser->ctx

struct handlebars_token * handlebars_token_ctor(struct handlebars_parser * parser, int token_int, const char * text, size_t length)
{
    struct handlebars_token * token = MC(handlebars_talloc_zero(parser, struct handlebars_token));
    token->token = token_int;
    token->text = MC(handlebars_talloc_strndup(token, text, length));
    token->length = length;
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

const char * handlebars_token_get_text(struct handlebars_token * token)
{
    if( likely(token != NULL) ) {
        return (const char *) token->text;
    } else {
        return NULL;
    }
}

void handlebars_token_get_text_ex(struct handlebars_token * token, const char ** text, size_t * length)
{
    if( likely(token != NULL) ) {
        *text = (const char *) token->text;
        *length = (const size_t) token->length;
    } else {
        *text = NULL;
        *length = 0;
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

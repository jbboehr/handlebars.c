
#include <errno.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_token.h"
#include "handlebars.tab.h"

struct handlebars_token * handlebars_token_ctor(int token_int, const char * text, size_t length, void * ctx)
{
    struct handlebars_token * token = NULL;
    
    // Allocate token
    // @todo doesn't take into account the alignment
    // note: +1 for \0, -1 for char[1]
    token = handlebars_talloc_zero(ctx, struct handlebars_token);
    if( token == NULL ) {
        errno = ENOMEM;
        goto error;
    }
    
    // Assign int token and length
    token->token = token_int;
    
    // Copy string and null terminate
    char * textdup = talloc_strndup(token, text, length);
    if( textdup == NULL ) {
        errno = ENOMEM;
        goto error;
    }
    token->text = textdup;
    token->length = length;
    
    return token;
error:
    if( token ) {
        talloc_free(token);
        token = NULL;
    }
    return token;
}

void handlebars_token_dtor(struct handlebars_token * token)
{
    handlebars_talloc_free(token);
}

const char * handlebars_token_readable_type(int type)
{
#define _RTYPE_CASE(str) \
    case str: return handlebars_stringify(str); break
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
  }
  
  return "UNKNOWN";
}

int handlebars_token_reverse_readable_type(const char * type)
{
#define _RTYPE_CMP(str) \
    if( strcmp(type, handlebars_stringify(str)) == 0 ) { \
        return str; \
    }
    switch( type[0] ) {
        case 'B':
            _RTYPE_CMP(BOOLEAN);
            break;
        case 'C':
            _RTYPE_CMP(CLOSE);
            _RTYPE_CMP(CLOSE_RAW_BLOCK);
            _RTYPE_CMP(CLOSE_SEXPR);
            _RTYPE_CMP(CLOSE_UNESCAPED);
            _RTYPE_CMP(COMMENT);
            _RTYPE_CMP(CONTENT);
            break;
        case 'D':
            _RTYPE_CMP(DATA);
            break;
        case 'E':
            _RTYPE_CMP(END);
            _RTYPE_CMP(END_RAW_BLOCK);
            _RTYPE_CMP(EQUALS);
            break;
        case 'I':
            _RTYPE_CMP(ID);
            _RTYPE_CMP(INVALID);
            _RTYPE_CMP(INVERSE);
            break;
        case 'N':
            _RTYPE_CMP(NUMBER);
            break;
        case 'O':
            _RTYPE_CMP(OPEN);
            _RTYPE_CMP(OPEN_BLOCK);
            _RTYPE_CMP(OPEN_ENDBLOCK);
            _RTYPE_CMP(OPEN_INVERSE);
            _RTYPE_CMP(OPEN_PARTIAL);
            _RTYPE_CMP(OPEN_RAW_BLOCK);
            _RTYPE_CMP(OPEN_SEXPR);
            _RTYPE_CMP(OPEN_UNESCAPED);
            break;
        case 'S':
            _RTYPE_CMP(SEP);
            _RTYPE_CMP(STRING);
            break;
    }
    
    // Unknown :(
    return -1;
}

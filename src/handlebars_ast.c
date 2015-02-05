
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_utils.h"

struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_node_type type, void * ctx)
{
    struct handlebars_ast_node * ast_node;
    
    ast_node = handlebars_talloc_zero(ctx, struct handlebars_ast_node);
    if( ast_node == NULL ) {
        errno = ENOMEM;
        goto done;
    }
    ast_node->type = type;
    
done:
    return ast_node;
}

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    handlebars_talloc_free(ast_node);
}

const char * handlebars_ast_node_readable_type(int type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_CASE(str) \
    case _RTYPE_MK(str): return _RTYPE_STR(str); break
  switch( type ) {
    _RTYPE_CASE(NIL);
    _RTYPE_CASE(PROGRAM);
    _RTYPE_CASE(MUSTACHE);
    _RTYPE_CASE(SEXPR);
    _RTYPE_CASE(PARTIAL);
    _RTYPE_CASE(BLOCK);
    _RTYPE_CASE(RAW_BLOCK);
    _RTYPE_CASE(CONTENT);
    _RTYPE_CASE(HASH);
    _RTYPE_CASE(HASH_SEGMENT);
    _RTYPE_CASE(ID);
    _RTYPE_CASE(PARTIAL_NAME);
    _RTYPE_CASE(DATA);
    _RTYPE_CASE(STRING);
    _RTYPE_CASE(NUMBER);
    _RTYPE_CASE(BOOLEAN);
    _RTYPE_CASE(COMMENT);
    _RTYPE_CASE(PATH_SEGMENT);
  }
  
  return "UNKNOWN";
}


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
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

const char * handlebars_ast_node_get_id_name(struct handlebars_ast_node * ast_node)
{
    const char * ret;
    
    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_ID:
            ret = (const char *) ast_node->node.id.id_name;
            break;
        case HANDLEBARS_AST_NODE_DATA: {
            if( ast_node->node.data.id_name == NULL ) {
                const char * id_string_mode_value = handlebars_ast_node_get_string_mode_value(ast_node->node.data.id);
                ast_node->node.data.id_name = talloc_asprintf(ast_node, "@%s", id_string_mode_value ? id_string_mode_value : "");
            }
            ret = ast_node->node.data.id_name;
            break;
        }
        default:
            ret = NULL;
            break;
    }
    
    return ret;
}

const char * handlebars_ast_node_get_id_part(struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    
    assert(ast_node != NULL);
    assert(ast_node->type == HANDLEBARS_AST_NODE_ID);
    
    parts = ast_node->node.id.parts;
    if( parts == NULL || parts->first == NULL || parts->first->data == NULL ) {
        return NULL;
    }
    
    path_segment = parts->first->data;
    
    assert(path_segment->type == HANDLEBARS_AST_NODE_PATH_SEGMENT);
    
    return (const char *) path_segment->node.path_segment.part;
}

const char * handlebars_ast_node_get_string_mode_value(struct handlebars_ast_node * ast_node)
{
    const char * ret;
    
    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_ID:
            ret = (const char *) ast_node->node.id.string;
            break;
        case HANDLEBARS_AST_NODE_DATA:
            ret = handlebars_ast_node_get_string_mode_value(ast_node->node.data.id);
            break;
        case HANDLEBARS_AST_NODE_STRING:
            ret = (const char *) ast_node->node.string.string;
            break;
        case HANDLEBARS_AST_NODE_NUMBER:
            ret = (const char *) ast_node->node.number.string;
            break;
        case HANDLEBARS_AST_NODE_BOOLEAN:
            ret = (const char *) ast_node->node.boolean.string;
            break;
        default:
            ret = NULL;
            break;
    }
    
    return ret;
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

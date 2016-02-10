
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(ptr) \
    do { \
        if( unlikely(ptr == NULL) ) { \
            handlebars_context_throw(context, HANDLEBARS_NOMEM, "Out of memory  [" __S2(__FILE__) ":" __S2(__LINE__) "]"); \
        } \
    } while(0)



struct handlebars_ast_node * handlebars_ast_node_ctor(struct handlebars_context * context, enum handlebars_ast_node_type type)
{
    struct handlebars_ast_node * ast_node;
    
    ast_node = handlebars_talloc_zero(context, struct handlebars_ast_node);
    __MEMCHECK(ast_node);

    ast_node->type = type;
    return ast_node;
}

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    assert(ast_node != NULL);
    
    handlebars_talloc_free(ast_node);
}

const char * handlebars_ast_node_get_id_part(struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    
    assert(ast_node != NULL);
    assert(ast_node->type == HANDLEBARS_AST_NODE_PATH);
    
    parts = ast_node->node.path.parts;
    if( parts == NULL || parts->first == NULL || parts->first->data == NULL ) {
        return NULL;
    }
    
    path_segment = parts->first->data;
    
    assert(path_segment->type == HANDLEBARS_AST_NODE_PATH_SEGMENT);
    return (const char *) path_segment->node.path_segment.part;
}

char ** handlebars_ast_node_get_id_parts(struct handlebars_context * context, struct handlebars_ast_node * ast_node)
{
    size_t num;
    char ** arr;
    char ** arrptr;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    if( !ast_node || ast_node->type != HANDLEBARS_AST_NODE_PATH ) {
        return NULL;
    }
    
    num = handlebars_ast_list_count(ast_node->node.path.parts);
    if( num == 0 ) {
        return NULL;
    }

    arrptr = arr = handlebars_talloc_array(context, char *, num + 1);
    if( unlikely(arr == NULL) ) {
        return NULL;
    }
    
    handlebars_ast_list_foreach(ast_node->node.path.parts, item, tmp) {
        assert(item->data);
        if( likely(item->data->type == HANDLEBARS_AST_NODE_PATH_SEGMENT) ) {
            *arrptr = handlebars_talloc_strdup(arr, item->data->node.path_segment.part);
            if( unlikely(*arrptr == NULL) ) {
                handlebars_talloc_free(arr);
                return NULL;
            }
            ++arrptr;
        }
    }
    *arrptr++ = NULL;
    
    return arr;
}

const char * handlebars_ast_node_get_string_mode_value(struct handlebars_ast_node * node)
{
    const char * ret;
    
    assert(node != NULL);
    
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_PATH:
            ret = (const char *) node->node.path.original;
            break;
        case HANDLEBARS_AST_NODE_STRING:
            ret = (const char *) node->node.string.value;
            break;
        case HANDLEBARS_AST_NODE_NUMBER:
            ret = (const char *) node->node.number.value;
            break;
        case HANDLEBARS_AST_NODE_BOOLEAN:
            ret = (const char *) node->node.boolean.value;
            break;
        case HANDLEBARS_AST_NODE_UNDEFINED:
            //ret = (const char *) node->node.undefined.value;
            ret = "undefined";
            break;
        case HANDLEBARS_AST_NODE_NUL:
            //ret = (const char *) node->node.nul.value;
            ret = "null";
            break;
        default:
            ret = NULL;
            break;
    }
    
    if( likely(ret != NULL) ) {
        return ret;
    } else {
        return "";
    }
}

struct handlebars_ast_node * handlebars_ast_node_get_path(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.path;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.path;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.path;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.name;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.path;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.path;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.path;
        default:
            return NULL;
    }
}

struct handlebars_ast_list * handlebars_ast_node_get_params(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.params;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.params;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.params;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.params;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.params;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.params;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.params;
        default:
            return NULL;
    }
}

struct handlebars_ast_node * handlebars_ast_node_get_hash(struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK:
            return node->node.block.hash;
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
            return node->node.intermediate.hash;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return node->node.mustache.hash;
        case HANDLEBARS_AST_NODE_PARTIAL:
            return node->node.partial.hash;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return node->node.partial_block.hash;
        case HANDLEBARS_AST_NODE_SEXPR:
            return node->node.sexpr.hash;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return node->node.raw_block.hash;
        default:
            return NULL;
    }
}

const char * handlebars_ast_node_readable_type(int type)
{
#define _RTYPE_STR(str) #str
#define _RTYPE_MK(str) HANDLEBARS_AST_NODE_ ## str
#define _RTYPE_CASE(str, name) \
    case _RTYPE_MK(str): return _RTYPE_STR(name); break
  switch( type ) {
    _RTYPE_CASE(NIL, NIL);
    _RTYPE_CASE(BLOCK, block);
    _RTYPE_CASE(BOOLEAN, BooleanLiteral);
    _RTYPE_CASE(COMMENT, comment);
    _RTYPE_CASE(CONTENT, content);
    _RTYPE_CASE(HASH, hash);
    _RTYPE_CASE(HASH_PAIR, HASH_PAIR);
    _RTYPE_CASE(INTERMEDIATE, INTERMEDIATE);
    _RTYPE_CASE(INVERSE, INVERSE);
    _RTYPE_CASE(MUSTACHE, mustache);
    _RTYPE_CASE(NUMBER, NumberLiteral);
    _RTYPE_CASE(PARTIAL, partial);
    _RTYPE_CASE(PARTIAL_BLOCK, PartialBlockStatement);
    _RTYPE_CASE(PATH, PathExpression);
    _RTYPE_CASE(PATH_SEGMENT, PATH_SEGMENT);
    _RTYPE_CASE(PROGRAM, program);
    _RTYPE_CASE(RAW_BLOCK, raw_block);
    _RTYPE_CASE(SEXPR, SubExpression);
    _RTYPE_CASE(STRING, StringLiteral);
    _RTYPE_CASE(UNDEFINED, UNDEFINED);
    case HANDLEBARS_AST_NODE_NUL: return "NULL";
  }
  
  return "UNKNOWN";
}


struct handlebars_ast_node * handlebars_ast_node_ctor_block(
    struct handlebars_context * context, struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * program, struct handlebars_ast_node * inverse,
    unsigned open_strip, unsigned inverse_strip, unsigned close_strip,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_BLOCK);
    ast_node->loc = *locinfo;

    if( intermediate ) {
        ast_node->node.block.path = talloc_steal(ast_node, intermediate->node.intermediate.path);
        ast_node->node.block.params = talloc_steal(ast_node, intermediate->node.intermediate.params);
        ast_node->node.block.hash = talloc_steal(ast_node, intermediate->node.intermediate.hash);
        
        // We can free the intermediate
        // @todo it seems to not be safe to free it... used in prepare_block
        //handlebars_talloc_free(intermediate);
        talloc_steal(ast_node, intermediate);
    }

    ast_node->node.block.program = talloc_steal(ast_node, program);
    ast_node->node.block.inverse = talloc_steal(ast_node, inverse);
    ast_node->node.block.open_strip = open_strip;
    ast_node->node.block.inverse_strip = inverse_strip;
    ast_node->node.block.close_strip = close_strip;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_boolean(
    struct handlebars_context * context, const char * boolean,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_BOOLEAN);
    ast_node->loc = *locinfo;
    ast_node->node.boolean.value = handlebars_talloc_strdup(ast_node, boolean);
    __MEMCHECK(ast_node->node.boolean.value);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_comment(
        struct handlebars_context * context, const char * comment,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_COMMENT);
    ast_node->loc = *locinfo;
    ast_node->strip |= handlebars_ast_strip_flag_set | handlebars_ast_strip_flag_inline_standalone;
    ast_node->node.comment.value = handlebars_talloc_strdup(ast_node, comment);
    __MEMCHECK(ast_node->node.comment.value);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_content(
        struct handlebars_context * context, const char * content,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_CONTENT);
    ast_node->loc = *locinfo;
    ast_node->node.content.value = handlebars_talloc_strdup(ast_node, content);
    __MEMCHECK(ast_node->node.content.value);
    ast_node->node.content.original = handlebars_talloc_strdup(ast_node, content);
    __MEMCHECK(ast_node->node.content.original);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_hash_pair(
    struct handlebars_context * context, const char * key,
    struct handlebars_ast_node * value, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_HASH_PAIR);
    ast_node->loc = *locinfo;
    ast_node->node.hash_pair.key = handlebars_talloc_strdup(ast_node, key);
    __MEMCHECK(ast_node->node.hash_pair.key);
    ast_node->node.hash_pair.value = talloc_steal(ast_node, value);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_intermediate(
	    struct handlebars_context * context, struct handlebars_ast_node * path,
	    struct handlebars_ast_list * params, struct handlebars_ast_node * hash,
	    unsigned strip, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_INTERMEDIATE);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.intermediate.path = talloc_steal(ast_node, path);
    ast_node->node.intermediate.params = talloc_steal(ast_node, params);
    ast_node->node.intermediate.hash = talloc_steal(ast_node, hash);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_inverse(
	    struct handlebars_context * context, struct handlebars_ast_node * program,
        bool chained, unsigned strip, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_INVERSE);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.inverse.program = talloc_steal(ast_node, program);
    ast_node->node.inverse.chained = chained;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_null(
    struct handlebars_context * context, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_NUL);
    ast_node->loc = *locinfo;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_number(
    struct handlebars_context * context, const char * number,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_NUMBER);
    ast_node->loc = *locinfo;
    ast_node->node.number.value = handlebars_talloc_strdup(ast_node, number);
    __MEMCHECK(ast_node->node.number.value);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial(
        struct handlebars_context * context, struct handlebars_ast_node * name,
        struct handlebars_ast_list * params, struct handlebars_ast_node * hash,
        unsigned strip, struct handlebars_locinfo * yylloc)
{
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PARTIAL);
    ast_node->loc = *yylloc;
    ast_node->strip = strip;
    ast_node->node.partial.name = talloc_steal(ast_node, name);
    ast_node->node.partial.params = talloc_steal(ast_node, params);
    ast_node->node.partial.hash = talloc_steal(ast_node, hash);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial_block(
    struct handlebars_context * context, struct handlebars_ast_node * open,
    struct handlebars_ast_node * program, struct handlebars_ast_node * close,
    struct handlebars_locinfo * loc)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PARTIAL_BLOCK);
    ast_node->loc = *loc;

    if( open ) {
        ast_node->node.block.path = talloc_steal(ast_node, open->node.intermediate.path);
        ast_node->node.block.params = talloc_steal(ast_node, open->node.intermediate.params);
        ast_node->node.block.hash = talloc_steal(ast_node, open->node.intermediate.hash);

        // We can free the intermediate
        // @todo it seems to not be safe to free it... used in prepare_block
        //handlebars_talloc_free(intermediate);
        talloc_steal(ast_node, open);
    }

    ast_node->node.block.program = talloc_steal(ast_node, program);
    ast_node->node.block.open_strip = open->strip;
    ast_node->node.block.close_strip = close->strip;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_program(
    struct handlebars_context * context, struct handlebars_ast_list * statements,
    char * block_param1, char * block_param2, unsigned strip,
    bool chained, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PROGRAM);
    ast_node->loc = *locinfo;
    ast_node->strip = strip;
    ast_node->node.program.statements = talloc_steal(ast_node, statements);
    ast_node->node.program.block_param1 = talloc_steal(ast_node, block_param1);
    ast_node->node.program.block_param2 = talloc_steal(ast_node, block_param2);
    ast_node->node.program.chained = chained;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_path(
    struct handlebars_context * context, struct handlebars_ast_list * parts,
    char * original, int depth, bool data, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    assert(parts != NULL);
    assert(original != NULL);

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PATH);
    ast_node->loc = *locinfo;
    ast_node->node.path.parts = talloc_steal(ast_node, parts);
    ast_node->node.path.original = talloc_steal(ast_node, original);
    ast_node->node.path.data = data;
    ast_node->node.path.depth = depth;

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_path_segment(
    struct handlebars_context * context, const char * part, const char * separator,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    char * newpart;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PATH_SEGMENT);
    ast_node->loc = *locinfo;

    ast_node->node.path_segment.original = handlebars_talloc_strdup(ast_node, part);
    __MEMCHECK(ast_node->node.path_segment.original);

    newpart = handlebars_talloc_strdup(ast_node, part);
    __MEMCHECK(newpart);
    newpart = handlebars_ast_helper_strip_id_literal(newpart);
    ast_node->node.path_segment.part = newpart;
    __MEMCHECK(ast_node->node.path_segment.part);
    
    if( separator != NULL ) {
        ast_node->node.path_segment.separator = handlebars_talloc_strdup(ast_node, separator);
        __MEMCHECK(ast_node->node.path_segment.separator);
    }

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_raw_block(
    struct handlebars_context * context, struct handlebars_ast_node * intermediate,
    struct handlebars_ast_node * content,  struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;
    struct handlebars_ast_node * program;
    struct handlebars_ast_list * statements;

    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_RAW_BLOCK);
    ast_node->loc = *locinfo;
    
    // Assign the content
    assert(content != NULL);
    assert(content->type == HANDLEBARS_AST_NODE_CONTENT);
    assert(intermediate != NULL);
    
    path = intermediate->node.intermediate.path;
    params = intermediate->node.intermediate.params;
    hash = intermediate->node.intermediate.hash;
    
    assert(path == NULL || path->type == HANDLEBARS_AST_NODE_PATH);
    assert(hash == NULL || hash->type == HANDLEBARS_AST_NODE_HASH);

    // Create the program node
    program = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_PROGRAM);
    statements = talloc_steal(program, handlebars_ast_list_ctor(context));
    __MEMCHECK(statements);
    program->node.program.statements = statements;
    handlebars_ast_list_append(statements, talloc_steal(program, content));
    ast_node->node.raw_block.program = program;
    //ast_node->node.raw_block.program = talloc_steal(ast_node, content);
    
    // Assign the other nodes
    ast_node->node.raw_block.path = talloc_steal(ast_node, path);
    ast_node->node.raw_block.params = talloc_steal(ast_node, params);
    ast_node->node.raw_block.hash = talloc_steal(ast_node, hash);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_sexpr(
	    struct handlebars_context * context,
	    struct handlebars_ast_node * intermediate,
	    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    assert(intermediate != NULL);

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_SEXPR);
    ast_node->loc = *locinfo;
    ast_node->node.sexpr.path = talloc_steal(ast_node, intermediate->node.intermediate.path);
    ast_node->node.sexpr.params = talloc_steal(ast_node, intermediate->node.intermediate.params);
    ast_node->node.sexpr.hash = talloc_steal(ast_node, intermediate->node.intermediate.hash);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_string(
    struct handlebars_context * context, const char * string,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_STRING);
    ast_node->loc = *locinfo;
    ast_node->node.string.value = handlebars_talloc_strdup(ast_node, string);
    __MEMCHECK(ast_node->node.string.value);

    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_undefined(
    struct handlebars_context * context, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;

    ast_node = handlebars_ast_node_ctor(context, HANDLEBARS_AST_NODE_UNDEFINED);
    ast_node->loc = *locinfo;

    return ast_node;
}


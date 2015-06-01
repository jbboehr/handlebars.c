
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
#include "handlebars_private.h"
#include "handlebars_utils.h"

#define __MEMCHECK(ptr) \
    do { \
        if( unlikely(ptr == NULL) ) { \
            ast_node = NULL; \
            context->errnum = HANDLEBARS_NOMEM; \
            goto error; \
        } \
    } while(0)

struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_ast_node_type type, void * ctx)
{
    struct handlebars_ast_node * ast_node;
    
    ast_node = handlebars_talloc_zero(ctx, struct handlebars_ast_node);
    if( unlikely(ast_node == NULL) ) {
        errno = ENOMEM;
        goto done;
    }
    ast_node->type = type;
    
done:
    return ast_node;
}

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    assert(ast_node != NULL);
    handlebars_talloc_free(ast_node);
}

const char * handlebars_ast_node_get_id_name(struct handlebars_ast_node * ast_node)
{
    const char * ret;
    
    assert(ast_node != NULL);

    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_ID:
            ret = (const char *) ast_node->node.id.id_name;
            break;
        case HANDLEBARS_AST_NODE_DATA: {
            if( ast_node->node.data.id_name == NULL ) {
                const char * id_string_mode_value = handlebars_ast_node_get_string_mode_value(ast_node->node.data.id);
                ast_node->node.data.id_name = handlebars_talloc_asprintf(ast_node, "@%s", id_string_mode_value ? id_string_mode_value : "");
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

char ** handlebars_ast_node_get_id_parts(void * ctx, struct handlebars_ast_node * ast_node)
{
    size_t num;
    char ** arr;
    char ** arrptr;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    if( !ast_node || ast_node->type != HANDLEBARS_AST_NODE_ID ) {
        return NULL;
    }
    
    num = handlebars_ast_list_count(ast_node->node.id.parts);
    if( num == 0 ) {
        return NULL;
    }

    arrptr = arr = handlebars_talloc_array(ctx, char *, num + 1);
    if( unlikely(arr == NULL) ) {
        return NULL;
    }
    
    handlebars_ast_list_foreach(ast_node->node.id.parts, item, tmp) {
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

const char * handlebars_ast_node_get_string_mode_value(struct handlebars_ast_node * ast_node)
{
    const char * ret;
    
    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_ID:
            ret = (const char *) ast_node->node.id.string;
            if( unlikely(ret == NULL) ) {
                ret = "";
            }
            break;
        case HANDLEBARS_AST_NODE_DATA:
            //ret = handlebars_talloc_asprintf(ast_node, "@%s", handlebars_ast_node_get_string_mode_value(ast_node->node.data.id));
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
#define _RTYPE_CASE(str, name) \
    case _RTYPE_MK(str): return _RTYPE_STR(name); break
  switch( type ) {
    _RTYPE_CASE(NIL, NIL);
    _RTYPE_CASE(PROGRAM, program);
    _RTYPE_CASE(MUSTACHE, mustache);
    _RTYPE_CASE(SEXPR, sexpr);
    _RTYPE_CASE(PARTIAL, partial);
    _RTYPE_CASE(BLOCK, block);
    _RTYPE_CASE(RAW_BLOCK, raw_block);
    _RTYPE_CASE(CONTENT, content);
    _RTYPE_CASE(HASH, hash);
    _RTYPE_CASE(HASH_SEGMENT, HASH_SEGMENT);
    _RTYPE_CASE(ID, ID);
    _RTYPE_CASE(PARTIAL_NAME, PARTIAL_NAME);
    _RTYPE_CASE(DATA, DATA);
    _RTYPE_CASE(STRING, STRING);
    _RTYPE_CASE(NUMBER, NUMBER);
    _RTYPE_CASE(BOOLEAN, BOOLEAN);
    _RTYPE_CASE(COMMENT, comment);
    _RTYPE_CASE(PATH_SEGMENT, PATH_SEGMENT);
    _RTYPE_CASE(INVERSE_AND_PROGRAM, INVERSE_AND_PROGRAM);
    // Added in v3.0.3
    case HANDLEBARS_AST_NODE_NUL: return "NULL";
    _RTYPE_CASE(UNDEFINED, UNDEFINED);
  }
  
  return "UNKNOWN";
}


struct handlebars_ast_node * handlebars_ast_node_ctor_boolean(
    struct handlebars_context * context, const char * boolean,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_BOOLEAN, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.boolean.string = handlebars_talloc_strdup(ast_node, boolean);
    __MEMCHECK(ast_node->node.boolean.string);
    ast_node->node.boolean.length = strlen(boolean);
    
    // Now steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_comment(
        struct handlebars_context * context, const char * comment,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_COMMENT, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    ast_node->strip |= handlebars_ast_strip_flag_set | handlebars_ast_strip_flag_inline_standalone;
    
    // Assign the content
    ast_node->node.comment.comment = handlebars_talloc_strdup(ast_node, comment);
    __MEMCHECK(ast_node->node.comment.comment);
    ast_node->node.comment.length = strlen(comment);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_content(
        struct handlebars_context * context, const char * content,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_CONTENT, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.content.string = handlebars_talloc_strdup(ast_node, content);
    __MEMCHECK(ast_node->node.content.string);
    ast_node->node.content.length = strlen(content);
    ast_node->node.content.original = handlebars_talloc_strdup(ast_node, content);
    __MEMCHECK(ast_node->node.content.original);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_data(
    struct handlebars_context * context, struct handlebars_ast_node * id,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_DATA, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.data.id = talloc_steal(ast_node, id);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_hash_segment(
    struct handlebars_context * context, const char * key,
    struct handlebars_ast_node * value, struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_HASH_SEGMENT, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.hash_segment.key = handlebars_talloc_strdup(ast_node, key);
    __MEMCHECK(ast_node->node.hash_segment.key);
    ast_node->node.hash_segment.key_length = strlen(key);
    
    ast_node->node.hash_segment.value = talloc_steal(ast_node, value);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_id(
    struct handlebars_context * context, struct handlebars_ast_list * parts,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_ID, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.id.parts = talloc_steal(ast_node, parts);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_inverse_and_program(
    struct handlebars_context * context, struct handlebars_ast_node * program,
    unsigned strip, struct handlebars_locinfo * yylloc)
{
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_INVERSE_AND_PROGRAM, context);
    __MEMCHECK(ast_node);
    ast_node->loc = *yylloc;
    ast_node->strip = strip;
    if( !program ) {
        ast_node->node.inverse_and_program.program = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PROGRAM, ast_node);
    } else {
        ast_node->node.inverse_and_program.program = talloc_steal(ast_node, program);
    }
error:
      return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_number(
    struct handlebars_context * context, const char * number,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_NUMBER, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.number.string = handlebars_talloc_strdup(ast_node, number);
    __MEMCHECK(ast_node->node.number.string);
    ast_node->node.number.length = strlen(number);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial(
        struct handlebars_context * context, struct handlebars_ast_node * partial_name,
        struct handlebars_ast_node * param, struct handlebars_ast_node * hash, 
        unsigned strip, struct handlebars_locinfo * yylloc)
{
    struct handlebars_ast_node * ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL, context);
    __MEMCHECK(ast_node);
    ast_node->loc = *yylloc;
    ast_node->strip = strip | handlebars_ast_strip_flag_inline_standalone;
    if( partial_name ) {
        ast_node->node.partial.partial_name = talloc_steal(ast_node, partial_name);
    }
    if( param ) {
        ast_node->node.partial.context = param;
    }
    if( hash ) {
        ast_node->node.partial.hash = hash;
    }
error:
      return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_partial_name(
    struct handlebars_context * context, struct handlebars_ast_node * name,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PARTIAL_NAME, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.partial_name.name = talloc_steal(ast_node, name);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_path_segment(
    struct handlebars_context * context, const char * part, const char * separator,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PATH_SEGMENT, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.path_segment.part = handlebars_talloc_strdup(ast_node, part);
    __MEMCHECK(ast_node->node.path_segment.part);
    ast_node->node.path_segment.part_length = strlen(part);
    
    if( separator != NULL ) {
        ast_node->node.path_segment.separator = handlebars_talloc_strdup(ast_node, separator);
        __MEMCHECK(ast_node->node.path_segment.separator);
        ast_node->node.path_segment.separator_length = strlen(separator);
    }
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_raw_block(
        struct handlebars_context * context, struct handlebars_ast_node * mustache, 
        struct handlebars_ast_node * content, const char * close,
        struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_RAW_BLOCK, ctx);
    __MEMCHECK(ast_node);
    
    // Assign the location info
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.raw_block.program = talloc_steal(ast_node, content);
    ast_node->node.raw_block.close = handlebars_talloc_strdup(ast_node, close);
    __MEMCHECK(ast_node->node.raw_block.close);
    ast_node->node.raw_block.mustache = talloc_steal(ast_node, mustache);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

struct handlebars_ast_node * handlebars_ast_node_ctor_string(
    struct handlebars_context * context, const char * string,
    struct handlebars_locinfo * locinfo)
{
    struct handlebars_ast_node * ast_node;
    TALLOC_CTX * ctx;
    
    // Initialize temporary talloc context
    ctx = talloc_new(context);
    if( unlikely(ctx == NULL) ) {
        return NULL;
    }
    
    // Construct the ast node
    ast_node = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_STRING, ctx);
    __MEMCHECK(ast_node);
    
    ast_node->loc = *locinfo;
    
    // Assign the content
    ast_node->node.string.string = handlebars_talloc_strdup(ast_node, string);
    __MEMCHECK(ast_node->node.string.string);
    ast_node->node.string.length = strlen(string);
    
    // Steal the node so it won't be freed below
    talloc_steal(context, ast_node);
    
error:
    handlebars_talloc_free(ctx);
    return ast_node;
}

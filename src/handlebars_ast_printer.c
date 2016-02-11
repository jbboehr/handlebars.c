
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_ast_printer.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"



#define __APPEND(ptr) \
    do { \
        if( likely(ptr != NULL) ) { \
            ctx->output = MC(_handlebars_talloc_strdup_append_buffer(ctx->output, ptr)); \
        } \
    } while(0)

#define __PAD(str) \
    _handlebars_ast_print_pad(str, ctx)

#define __PAD_HEAD() \
    do { \
        char tmp[32]; \
        char * tmp2 = tmp; \
        if( ctx->padding > 0 && ctx->padding < 16 ) { \
            memset(tmp, ' ', ctx->padding * 2); \
            tmp[ctx->padding * 2] = 0; \
            __APPEND(tmp2); \
        } \
    } while(0)

#define __PAD_FOOT() \
    __APPEND("\n")

#define __PRINT(node) \
    _handlebars_ast_print(node, ctx)

static void _handlebars_ast_print(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx);
void _handlebars_ast_print_pad(char * str, struct handlebars_ast_printer_context * ctx);



#undef CONTEXT
#define CONTEXT ctx->ctx

void _handlebars_ast_print_pad(char * str, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND(str);
    __PAD_FOOT();
}

static void _handlebars_ast_print_list(struct handlebars_ast_list * ast_list, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    if( unlikely(ast_list == NULL) ) {
        return;
    }
    
    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __PRINT(item->data);
    }
}

static void _handlebars_ast_print_path_params_hash(
    struct handlebars_ast_node * path, struct handlebars_ast_list * params,
    struct handlebars_ast_node * hash, struct handlebars_ast_printer_context * ctx)
{
    if( path ) {
        __PRINT(path);
    }
    __APPEND(" [");
    
    if( params ) {
        struct handlebars_ast_list * ast_list = params;
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;
        
        handlebars_ast_list_foreach(ast_list, item, tmp) {
            __PRINT(item->data);
            if( item->next ) {
                __APPEND(", ");
            }
        }
    }
    
    __APPEND("]");
    
    if( hash ) {
        __APPEND(" ");
        __PRINT(hash);
    }
}




static void _handlebars_ast_print_program(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( ast_node->node.program.block_param1 ) {
        __PAD_HEAD();
        __APPEND("BLOCK PARAMS: [ ");
        __APPEND(ast_node->node.program.block_param1);
        if( ast_node->node.program.block_param2 ) {
            __APPEND(" ");
            __APPEND(ast_node->node.program.block_param2);
        }
        __APPEND(" ]");
        __PAD_FOOT();
    }

    _handlebars_ast_print_list(ast_node->node.program.statements, ctx);
    
    ctx->padding--;
}

static void _handlebars_ast_print_mustache(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("{{ ");
    
    if( ast_node->node.mustache.is_decorator ) {
    	__APPEND("DIRECTIVE ");
    }

    // Sexpr
    _handlebars_ast_print_path_params_hash(ast_node->node.mustache.path,
            ast_node->node.mustache.params, ast_node->node.mustache.hash, ctx);
    
    __APPEND(" }}");
    __PAD_FOOT();
}

static void _handlebars_ast_print_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
	if( ast_node->node.block.is_decorator ) {
		__PAD("DIRECTIVE BLOCK:");
	} else {
		__PAD("BLOCK:");
	}
    
    ctx->padding++;
    
    // Sexpr
    __PAD_HEAD();
    _handlebars_ast_print_path_params_hash(ast_node->node.block.path,
            ast_node->node.block.params, ast_node->node.block.hash, ctx);
    __PAD_FOOT();
    
    // Program
    if( ast_node->node.block.program ) {
        __PAD("PROGRAM:");
        ctx->padding++;
        __PRINT(ast_node->node.block.program);
        ctx->padding--;
    }
    
    // Inverse
    if( ast_node->node.block.inverse ) {
        if( ast_node->node.block.program ) {
            ctx->padding++;
        }
        __PAD("{{^}}");
        ctx->padding++;
        __PRINT(ast_node->node.block.inverse);
        ctx->padding--;
        if( ast_node->node.block.program ) {
            ctx->padding--;
        }
    }
    
    ctx->padding--;
}

static void _handlebars_ast_print_partial(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    assert(ast_node->node.partial.name != NULL);
    
    __PAD_HEAD();
    __APPEND("{{> PARTIAL:");
    
    ctx->in_partial = 1;
    __PRINT(ast_node->node.partial.name);
    ctx->in_partial = 0;
    //__APPEND(ast_node->node.partial.name->node.path.original);
    
   if( ast_node->node.partial.params != NULL && 
            handlebars_ast_list_count(ast_node->node.partial.params) ) {
        __APPEND(" ");
        __PRINT(ast_node->node.partial.params->first->data);
    }
    
    if( ast_node->node.partial.hash ) {
        __APPEND(" ");
        __PRINT(ast_node->node.partial.hash);
    }
    
    __APPEND(" }}");
    __PAD_FOOT();
}


static void _handlebars_ast_print_partial_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
	__PAD_HEAD();
    __APPEND("{{> PARTIAL BLOCK:");

    ctx->padding++;
    ctx->in_partial = 1;

    // Sexpr
    _handlebars_ast_print(ast_node->node.partial_block.path, ctx);
    if( ast_node->node.partial_block.params ) {
    	__APPEND(" PATH:");
        _handlebars_ast_print_list(ast_node->node.partial_block.params, ctx);
    }
    if( ast_node->node.partial_block.hash ) {
    	__APPEND(" ");
        _handlebars_ast_print(ast_node->node.partial_block.hash, ctx);
    }

    // Program
    if( ast_node->node.partial_block.program ) {
        __APPEND(" PROGRAM:");
        __PAD_FOOT();

        //ctx->padding++;
        __PRINT(ast_node->node.partial_block.program);
        //ctx->padding--;
    }

    ctx->in_partial = 0;
    ctx->padding--;

    __APPEND(" }}");
    __PAD_FOOT();
}

static void _handlebars_ast_print_content(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("CONTENT[ '");
    __APPEND(ast_node->node.content.value);
    __APPEND("' ]");
    __PAD_FOOT();
}

static void _handlebars_ast_print_comment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("{{! '");
    __APPEND(ast_node->node.comment.value);
    __APPEND("' }}");
    __PAD_FOOT();
}

static void _handlebars_ast_print_sexpr(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    // Sexpr
    _handlebars_ast_print_path_params_hash(ast_node->node.sexpr.path,
            ast_node->node.sexpr.params, ast_node->node.sexpr.hash, ctx);
}

static void _handlebars_ast_print_string(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( !ctx->in_partial ) {
        __APPEND("\"");
    }
    __APPEND(ast_node->node.string.value);
    if( !ctx->in_partial ) {
        __APPEND("\"");
    }
}

static void _handlebars_ast_print_number(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( !ctx->in_partial ) {
        __APPEND("NUMBER{");
    }
    __APPEND(ast_node->node.number.value);
    if( !ctx->in_partial ) {
        __APPEND("}");
    }
}

static void _handlebars_ast_print_boolean(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("BOOLEAN{");
    __APPEND(ast_node->node.boolean.value);
    __APPEND("}");
}

static void _handlebars_ast_print_undefined(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("UNDEFINED");
}

static void _handlebars_ast_print_null(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("NULL");
}

static void _handlebars_ast_print_hash(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.hash.pairs;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    short prev_in_partial = ctx->in_partial;
    ctx->in_partial = 0;
    
    __APPEND("HASH{");
    
    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND(item->data->node.hash_pair.key);
        __APPEND("=");
        __PRINT(item->data->node.hash_pair.value);
        if( item->next ) {
            __APPEND(", ");
        }
    }
    
    __APPEND("}");

    ctx->in_partial = prev_in_partial;
}




static void _handlebars_ast_print_path(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.path.parts;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    assert(ast_node->node.path.data == 0 || ast_node->node.path.data == 1);
    
    if( ast_node->node.path.data ) {
        __APPEND("@");
    }
    if( !ctx->in_partial ) {
        __APPEND("PATH:");
    }
    
    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND(item->data->node.path_segment.part);
        //__PRINT(item->data);
        if( item->next ) {
            __APPEND(item->next->data->node.path_segment.separator);
            //__APPEND("/");
        }
    }
}

static void _handlebars_ast_print(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( unlikely(ast_node == NULL) ) {
        return;
    }
    
    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_BOOLEAN: 
            return _handlebars_ast_print_boolean(ast_node, ctx);
        case HANDLEBARS_AST_NODE_BLOCK: 
            return _handlebars_ast_print_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_COMMENT: 
            return _handlebars_ast_print_comment(ast_node, ctx);
        case HANDLEBARS_AST_NODE_CONTENT: 
            return _handlebars_ast_print_content(ast_node, ctx);
        case HANDLEBARS_AST_NODE_HASH: 
            return _handlebars_ast_print_hash(ast_node, ctx);
        case HANDLEBARS_AST_NODE_MUSTACHE: 
            return _handlebars_ast_print_mustache(ast_node, ctx);
        case HANDLEBARS_AST_NODE_NUL:
            return _handlebars_ast_print_null(ast_node, ctx);
        case HANDLEBARS_AST_NODE_NUMBER: 
            return _handlebars_ast_print_number(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PARTIAL: 
            return _handlebars_ast_print_partial(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return _handlebars_ast_print_partial_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PATH:
            return _handlebars_ast_print_path(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PROGRAM: 
            return _handlebars_ast_print_program(ast_node, ctx);
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return _handlebars_ast_print_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_SEXPR: 
            return _handlebars_ast_print_sexpr(ast_node, ctx);
        case HANDLEBARS_AST_NODE_STRING: 
            return _handlebars_ast_print_string(ast_node, ctx);
        case HANDLEBARS_AST_NODE_UNDEFINED:
            return _handlebars_ast_print_undefined(ast_node, ctx);
        // LCOV_EXCL_START
        // Note: these should never be printed, intermediate nodes
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
        case HANDLEBARS_AST_NODE_INVERSE:
        // Note: these are currently implemented within their parent
        case HANDLEBARS_AST_NODE_HASH_PAIR:
        case HANDLEBARS_AST_NODE_PATH_SEGMENT:
        case HANDLEBARS_AST_NODE_NIL:
            assert(0);
            break;
        // LCOV_EXCL_STOP
    }
}

#undef CONTEXT
#define CONTEXT context

char * handlebars_ast_print(struct handlebars_context * context, struct handlebars_ast_node * ast_node, int flags)
{
    char * output;
    struct handlebars_ast_printer_context * ctx = MC(handlebars_talloc_zero(context, struct handlebars_ast_printer_context));
    ctx->ctx = context;
    ctx->flags = flags;
    ctx->output = MC(handlebars_talloc_strdup(context, ""));
    _handlebars_ast_print(ast_node, ctx);
    output = ctx->output;
    handlebars_talloc_free(ctx);
    handlebars_rtrim(output, " \t\r\n");
    return output;
}

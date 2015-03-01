
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_ast_printer.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"



static void _handlebars_ast_print(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx);
void _handlebars_ast_print_pad(char * str, struct handlebars_ast_printer_context * ctx);



#define __APPEND(ptr) \
    do { \
        if( likely(ptr != NULL) ) { \
            ctx->output = _handlebars_talloc_strdup_append_buffer(ctx->output, ptr); \
            if( unlikely(ctx->output == NULL) ) { \
                ctx->error = errno = ENOMEM; \
                return; \
            } \
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




static void _handlebars_ast_print_program(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    _handlebars_ast_print_list(ast_node->node.program.statements, ctx);
    
    ctx->padding--;
}

static void _handlebars_ast_print_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD("BLOCK:");
    
    ctx->padding++;
    
    // Mustache
    __PRINT(ast_node->node.block.mustache);
    
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

static void _handlebars_ast_print_sexpr(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( ast_node->node.sexpr.id ) {
        __PRINT(ast_node->node.sexpr.id);
    }
    __APPEND(" [");
    
    if( ast_node->node.sexpr.params ) {
        struct handlebars_ast_list * ast_list = ast_node->node.sexpr.params;
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
    
    if( ast_node->node.sexpr.hash ) {
        __APPEND(" ");
        __PRINT(ast_node->node.sexpr.hash);
    }
}

static void _handlebars_ast_print_mustache(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("{{ ");
    __PRINT(ast_node->node.mustache.sexpr);
    __APPEND(" }}");
    __PAD_FOOT();
}


static void _handlebars_ast_print_partial(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("{{> ");
    
    ctx->in_partial = 1;
    __PRINT(ast_node->node.partial.partial_name);
    ctx->in_partial = 0;
    
    if( ast_node->node.partial.context ) {
        __APPEND(" ");
        __PRINT(ast_node->node.partial.context);
    }
    
    if( ast_node->node.partial.hash ) {
        __APPEND(" ");
        __PRINT(ast_node->node.partial.hash);
    }
    
    __APPEND(" }}");
    __PAD_FOOT();
}

static void _handlebars_ast_print_hash(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.hash.segments;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    __APPEND("HASH{");
    
    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND(item->data->node.hash_segment.key);
        __APPEND("=");
        __PRINT(item->data->node.hash_segment.value);
        if( item->next ) {
            __APPEND(", ");
        }
    }
    
    __APPEND("}");
}

static void _handlebars_ast_print_string(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("\"");
    __APPEND(ast_node->node.string.string);
    __APPEND("\"");
}

static void _handlebars_ast_print_number(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("NUMBER{");
    __APPEND(ast_node->node.number.string);
    __APPEND("}");
}

static void _handlebars_ast_print_boolean(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("BOOLEAN{");
    __APPEND(ast_node->node.boolean.string);
    __APPEND("}");
}

static void _handlebars_ast_print_id(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.id.parts;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    
    if( !ctx->in_partial ) {
        if( !ast_list->first || !ast_list->first->next ) {
            __APPEND("ID:");
        } else {
            __APPEND("PATH:");
        }
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

static void _handlebars_ast_print_partial_name(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("PARTIAL:");
    __PRINT(ast_node->node.partial_name.name);
}

static void _handlebars_ast_print_data(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND("@");
    __PRINT(ast_node->node.data.id);
}

static void _handlebars_ast_print_content(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("CONTENT[ '");
    __APPEND(ast_node->node.content.string);
    __APPEND("' ]");
    __PAD_FOOT();
}

static void _handlebars_ast_print_comment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPEND("{{! '");
    __APPEND(ast_node->node.comment.comment);
    __APPEND("' }}");
    __PAD_FOOT();
}

/*
static void _handlebars_ast_print_path_segment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( ast_node->node.path_segment.separator ) {
        __APPEND(ast_node->node.path_segment.separator);
    }
    __APPEND(ast_node->node.path_segment.part);
}

static void _handlebars_ast_print_hash_segment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    ;
}
*/

static void _handlebars_ast_print(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( unlikely(ast_node == NULL) ) {
        return;
    }
    
    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_PROGRAM: 
            return _handlebars_ast_print_program(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_MUSTACHE: 
            return _handlebars_ast_print_mustache(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_SEXPR: 
            return _handlebars_ast_print_sexpr(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_PARTIAL: 
            return _handlebars_ast_print_partial(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
        case HANDLEBARS_AST_NODE_BLOCK: 
            return _handlebars_ast_print_block(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_CONTENT: 
            return _handlebars_ast_print_content(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_HASH: 
            return _handlebars_ast_print_hash(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_ID: 
            return _handlebars_ast_print_id(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_PARTIAL_NAME: 
            return _handlebars_ast_print_partial_name(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_DATA: 
            return _handlebars_ast_print_data(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_STRING: 
            return _handlebars_ast_print_string(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_NUMBER: 
            return _handlebars_ast_print_number(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_BOOLEAN: 
            return _handlebars_ast_print_boolean(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_COMMENT: 
            return _handlebars_ast_print_comment(ast_node, ctx);
            break;
        // LCOV_EXCL_START
        // Note: these are currently implemented within their parent
        case HANDLEBARS_AST_NODE_HASH_SEGMENT: 
            //return _handlebars_ast_print_hash_segment(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_PATH_SEGMENT: 
            //return _handlebars_ast_print_path_segment(ast_node, ctx);
            break;
        case HANDLEBARS_AST_NODE_INVERSE_AND_PROGRAM: 
            break;
        case HANDLEBARS_AST_NODE_NIL:
            break;
        // LCOV_EXCL_STOP
    }
}

struct handlebars_ast_printer_context handlebars_ast_print2(struct handlebars_ast_node * ast_node, int flags)
{
    struct handlebars_ast_printer_context ctx;
    
    // Setup print context
    ctx.error = 0;
    ctx.length = 0;
    ctx.padding = 0;
    ctx.flags = flags;
    ctx.in_partial = 0;
    
    // Allocate initial string
    ctx.output = handlebars_talloc_strdup(NULL, "");
    if( unlikely(ctx.output == NULL) ) {
        ctx.error = errno = ENOMEM;
        goto error;
    }
    
    // Main print
    _handlebars_ast_print(ast_node, &ctx);
    
    // Trim whitespace off right end of output
    handlebars_rtrim(ctx.output, " \t\r\n");
    
    // Check for error and free
    if( unlikely(ctx.error && ctx.output != NULL) )  {
        handlebars_talloc_free(ctx.output);
        ctx.output = NULL;
    }

error:
    return ctx;
}

char * handlebars_ast_print(struct handlebars_ast_node * ast_node, int flags)
{
    return handlebars_ast_print2(ast_node, flags).output;
}

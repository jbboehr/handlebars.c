/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_ast_printer.h"
#include "handlebars_string.h"
#include "handlebars_utils.h"



#define __APPEND(ptr) \
    do { \
        if( likely(ptr != NULL) ) { \
            ctx->output = handlebars_string_append(ctx->ctx, ctx->output, ptr, strlen(ptr)); \
        } \
    } while(0)

#define __APPENDS(str) \
    do { \
        ctx->output = handlebars_string_append(ctx->ctx, ctx->output, str, sizeof(str) - 1); \
    } while(0)

#define __APPENDL(str, len) \
    do { \
        if( likely(str != NULL) ) { \
            ctx->output = handlebars_string_append(ctx->ctx, ctx->output, str, len); \
        } \
    } while(0)

#define __APPEND_STR(str) \
    do { \
        if( likely(str != NULL) ) { \
            ctx->output = handlebars_string_append_str(ctx->ctx, ctx->output, str); \
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
    __APPENDS("\n")

#define __PRINT(node) \
    _handlebars_ast_print(node, ctx)

/**
 * @brief AST printer context object
 */
struct handlebars_ast_printer_context {
    struct handlebars_context * ctx;
    int padding;
    int error;
    struct handlebars_string * output;
    bool in_partial;
    bool in_raw_block;
};



static void _handlebars_ast_print(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx);
void _handlebars_ast_print_pad(char * str, struct handlebars_ast_printer_context * ctx);



#undef CONTEXT
#define CONTEXT HBSCTX(ctx->ctx)

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
    __APPENDS(" [");

    if( params ) {
        struct handlebars_ast_list * ast_list = params;
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;

        handlebars_ast_list_foreach(ast_list, item, tmp) {
            __PRINT(item->data);
            if( item->next ) {
                __APPENDS(", ");
            }
        }
    }

    __APPENDS("]");

    if( hash ) {
        __APPENDS(" ");
        __PRINT(hash);
    }
}




static void _handlebars_ast_print_program(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( ast_node->node.program.block_param1 ) {
        __PAD_HEAD();
        __APPENDS("BLOCK PARAMS: [ ");
        __APPEND_STR(ast_node->node.program.block_param1);
        if( ast_node->node.program.block_param2 ) {
            __APPENDS(" ");
            __APPEND_STR(ast_node->node.program.block_param2);
        }
        __APPENDS(" ]");
        __PAD_FOOT();
    }

    _handlebars_ast_print_list(ast_node->node.program.statements, ctx);

    ctx->padding--;
}

static void _handlebars_ast_print_mustache(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPENDS("{{ ");

    if( ast_node->node.mustache.is_decorator ) {
    	__APPENDS("DIRECTIVE ");
    }

    // Sexpr
    _handlebars_ast_print_path_params_hash(ast_node->node.mustache.path,
            ast_node->node.mustache.params, ast_node->node.mustache.hash, ctx);

    __APPENDS(" }}");
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
    __APPENDS("{{> PARTIAL:");

    ctx->in_partial = true;
    __PRINT(ast_node->node.partial.name);
    ctx->in_partial = false;
    //__APPEND(ast_node->node.partial.name->node.path.original);

   if( ast_node->node.partial.params != NULL &&
            handlebars_ast_list_count(ast_node->node.partial.params) ) {
        __APPENDS(" ");
        __PRINT(ast_node->node.partial.params->first->data);
    }

    if( ast_node->node.partial.hash ) {
        __APPENDS(" ");
        __PRINT(ast_node->node.partial.hash);
    }

    __APPENDS(" }}");
    __PAD_FOOT();
}


static void _handlebars_ast_print_partial_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
	__PAD_HEAD();
    __APPENDS("{{> PARTIAL BLOCK:");

    ctx->padding++;
    ctx->in_partial = true;

    // Sexpr
    _handlebars_ast_print(ast_node->node.partial_block.path, ctx);
    if( ast_node->node.partial_block.params ) {
    	__APPENDS(" PATH:");
        _handlebars_ast_print_list(ast_node->node.partial_block.params, ctx);
    }
    if( ast_node->node.partial_block.hash ) {
    	__APPENDS(" ");
        _handlebars_ast_print(ast_node->node.partial_block.hash, ctx);
    }

    // Program
    if( ast_node->node.partial_block.program ) {
        __APPENDS(" PROGRAM:");
        __PAD_FOOT();

        //ctx->padding++;
        __PRINT(ast_node->node.partial_block.program);
        //ctx->padding--;
    }

    ctx->in_partial = false;
    ctx->padding--;

    __APPENDS(" }}");
    __PAD_FOOT();
}

static void _handlebars_ast_print_content(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPENDS("CONTENT[ '");
    __APPEND_STR(ast_node->node.content.value);
    __APPENDS("' ]");
    __PAD_FOOT();
}

static void _handlebars_ast_print_comment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __PAD_HEAD();
    __APPENDS("{{! '");
    __APPEND_STR(ast_node->node.comment.value);
    __APPENDS("' }}");
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
        __APPENDS("\"");
    }
    __APPEND_STR(ast_node->node.string.value);
    if( !ctx->in_partial ) {
        __APPENDS("\"");
    }
}

static void _handlebars_ast_print_number(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( !ctx->in_partial ) {
        __APPENDS("NUMBER{");
    }
    __APPEND_STR(ast_node->node.number.value);
    if( !ctx->in_partial ) {
        __APPENDS("}");
    }
}

static void _handlebars_ast_print_boolean(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("BOOLEAN{");
    __APPEND_STR(ast_node->node.boolean.value);
    __APPENDS("}");
}

static void _handlebars_ast_print_undefined(HANDLEBARS_ATTR_UNUSED struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("UNDEFINED");
}

static void _handlebars_ast_print_null(HANDLEBARS_ATTR_UNUSED struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("NULL");
}

static void _handlebars_ast_print_hash(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.hash.pairs;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    bool prev_in_partial = ctx->in_partial;
    ctx->in_partial = false;

    __APPENDS("HASH{");

    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND_STR(item->data->node.hash_pair.key);
        __APPENDS("=");
        __PRINT(item->data->node.hash_pair.value);
        if( item->next ) {
            __APPENDS(", ");
        }
    }

    __APPENDS("}");

    ctx->in_partial = prev_in_partial;
}




static void _handlebars_ast_print_path(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.path.parts;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;

    assert(ast_node->node.path.data == 0 || ast_node->node.path.data == 1);

    if( ast_node->node.path.data ) {
        __APPENDS("@");
    }
    if( !ctx->in_partial ) {
        __APPENDS("PATH:");
    }

    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND_STR(item->data->node.path_segment.part);
        //__PRINT(item->data);
        if( item->next ) {
            __APPEND_STR(item->next->data->node.path_segment.separator);
            //__APPENDS("/");
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

struct handlebars_string * handlebars_ast_print(struct handlebars_context * context, struct handlebars_ast_node * ast_node)
{
    struct handlebars_string * output;
    struct handlebars_ast_printer_context * ctx = MC(handlebars_talloc_zero(context, struct handlebars_ast_printer_context));
    ctx->ctx = context;
    ctx->output = handlebars_string_ctor(context, "", 0);
    _handlebars_ast_print(ast_node, ctx);
    output = talloc_steal(context, ctx->output);
    handlebars_talloc_free(ctx);
    return handlebars_string_rtrim(output, HBS_STRL(" \t\r\n"));
}















#undef __PRINT
#define __PRINT(node) \
    _handlebars_ast_to_string(node, ctx)

#undef CONTEXT
#define CONTEXT HBSCTX(ctx->ctx)

static void _handlebars_ast_to_string(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx);
static void _handlebars_ast_to_string_path(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx, bool first_only);

static void _handlebars_ast_to_string_list(struct handlebars_ast_list * ast_list, struct handlebars_ast_printer_context * ctx)
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

static void _handlebars_ast_to_string_path_params_hash(
    struct handlebars_ast_node * path, struct handlebars_ast_list * params,
    struct handlebars_ast_node * hash, struct handlebars_ast_printer_context * ctx)
{
    int depth = ctx->padding++;

    if (depth > 0) {
        __APPENDS("(");
    }

    if( path ) {
        __PRINT(path);
    }

    if( params ) {
        struct handlebars_ast_list * ast_list = params;
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;

        __APPENDS(" ");

        handlebars_ast_list_foreach(ast_list, item, tmp) {
            __PRINT(item->data);
            if( item->next ) {
                __APPENDS(" ");
            }
        }
    }

    if( hash ) {
        __APPENDS(" ");
        __PRINT(hash);
    }

    if (depth > 0) {
        __APPENDS(")");
    }

    ctx->padding--;
}




static void _handlebars_ast_to_string_program(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    _handlebars_ast_to_string_list(ast_node->node.program.statements, ctx);
}

static void _handlebars_ast_to_string_mustache(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    short unescaped = ast_node->node.mustache.unescaped;

    __APPENDS("{{");
    if (unescaped == 1) {
        __APPENDS("{");
    } /*else if (unescaped == 3) {
        __APPENDS("&");
    } */
	if( ast_node->node.mustache.is_decorator ) {
		__APPENDS("*");
	}
    _handlebars_ast_to_string_path_params_hash(ast_node->node.mustache.path,
            ast_node->node.mustache.params, ast_node->node.mustache.hash, ctx);
    if (unescaped == 1) {
        __APPENDS("}");
    }
    __APPENDS("}}");
}

static void _handlebars_ast_to_string_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    bool is_raw = ast_node->type == HANDLEBARS_AST_NODE_RAW_BLOCK;
    if (is_raw) {
        ctx->in_raw_block = 1;
    }

    // Use an inverted section
    if (ast_node->node.block.inverse && !ast_node->node.block.program) {
        if (is_raw) { __APPENDS("{{"); }
        __APPENDS("{{^");
        _handlebars_ast_to_string_path_params_hash(ast_node->node.block.path,
                ast_node->node.block.params, ast_node->node.block.hash, ctx);
        __APPENDS("}}");
        if (is_raw) { __APPENDS("}}"); }

        __PRINT(ast_node->node.block.inverse);

        if (is_raw) { __APPENDS("{{"); }
        __APPENDS("{{/");
        _handlebars_ast_to_string_path(ast_node->node.block.path, ctx, 1);
        __APPENDS("}}");
        if (is_raw) { __APPENDS("}}"); }
        goto done;
    }


    // Sexpr
    if (is_raw) { __APPENDS("{{{{"); }
    else { __APPENDS("{{#"); }
	if( ast_node->node.block.is_decorator ) {
		__APPENDS("*");
	}
    _handlebars_ast_to_string_path_params_hash(ast_node->node.block.path,
            ast_node->node.block.params, ast_node->node.block.hash, ctx);

    if (ast_node->node.block.program) {
        if( ast_node->node.block.program->node.program.block_param1 ) {
            __APPENDS("as |");
            __APPEND_STR(ast_node->node.block.program->node.program.block_param1);
            if( ast_node->node.block.program->node.program.block_param2 ) {
                __APPENDS(" ");
                __APPEND_STR(ast_node->node.block.program->node.program.block_param2);
            }
            __APPENDS("|");
        }
    }

    __APPENDS("}}");
    if (is_raw) { __APPENDS("}}"); }

    // Program
    if( ast_node->node.block.program ) {
        __PRINT(ast_node->node.block.program);
    }

    // Inverse
    if( ast_node->node.block.inverse ) {
        __APPENDS("{{^}}");
        __PRINT(ast_node->node.block.inverse);
    }

    // end
    if (is_raw) { __APPENDS("{{"); }
    __APPENDS("{{/");
    _handlebars_ast_to_string_path_params_hash(ast_node->node.block.path, NULL, NULL, ctx);
    // _handlebars_ast_to_string_path(ast_node->node.block.path, ctx, 1);
    __APPENDS("}}");
    if (is_raw) { __APPENDS("}}"); }

done:
    ctx->in_raw_block = 0;
}

static void _handlebars_ast_to_string_partial(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    ctx->in_partial = true;

    assert(ast_node->node.partial.name != NULL);

    __APPENDS("{{>");

    __PRINT(ast_node->node.partial.name);

   if( ast_node->node.partial.params != NULL &&
            handlebars_ast_list_count(ast_node->node.partial.params) ) {
        __APPENDS(" ");
        __PRINT(ast_node->node.partial.params->first->data);
    }

    if( ast_node->node.partial.hash ) {
        __APPENDS(" ");
        __PRINT(ast_node->node.partial.hash);
    }

    __APPENDS(" }}");

    ctx->in_partial = false;
}


static void _handlebars_ast_to_string_partial_block(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    ctx->in_partial = true;

    // Sexpr
    __APPENDS("{{#> ");
    _handlebars_ast_to_string(ast_node->node.partial_block.path, ctx);
    if( ast_node->node.partial_block.params ) {
        _handlebars_ast_to_string_list(ast_node->node.partial_block.params, ctx);
    }
    if( ast_node->node.partial_block.hash ) {
    	__APPENDS(" ");
        _handlebars_ast_to_string(ast_node->node.partial_block.hash, ctx);
    }
    __APPENDS("}}");

    // Program
    if( ast_node->node.partial_block.program ) {
        __PRINT(ast_node->node.partial_block.program);
    }

    // end
    __APPENDS("{{/");
    _handlebars_ast_to_string(ast_node->node.partial_block.path, ctx);
    __APPENDS("}}");

    ctx->in_partial = false;
}

static void _handlebars_ast_to_string_content(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    // Need to escape {{ if not in a raw block
    if (ctx->in_raw_block) {
        __APPEND_STR(ast_node->node.content.original);
    } else {
        struct handlebars_string *escaped = handlebars_str_replace(CONTEXT, ast_node->node.content.original, HBS_STRL("{{"), HBS_STRL("\\{{"));
        __APPEND_STR(escaped);
        handlebars_talloc_free(escaped);
    }
}

static void _handlebars_ast_to_string_comment(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("{{!");
    __APPEND_STR(ast_node->node.comment.value);
    __APPENDS("}}");
}

static void _handlebars_ast_to_string_sexpr(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    // Sexpr
    _handlebars_ast_to_string_path_params_hash(ast_node->node.sexpr.path,
            ast_node->node.sexpr.params, ast_node->node.sexpr.hash, ctx);
}

static void _handlebars_ast_to_string_string(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("\"");
    struct handlebars_string *tmp = handlebars_string_addcslashes(ctx->ctx, ast_node->node.string.value, HBS_STRL("\""));
    __APPEND_STR(tmp);
    handlebars_talloc_free(tmp);
    __APPENDS("\"");
}

static void _handlebars_ast_to_string_number(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND_STR(ast_node->node.number.value);
}

static void _handlebars_ast_to_string_boolean(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPEND_STR(ast_node->node.boolean.value);
}

static void _handlebars_ast_to_string_undefined(HANDLEBARS_ATTR_UNUSED struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("undefined");
}

static void _handlebars_ast_to_string_null(HANDLEBARS_ATTR_UNUSED struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    __APPENDS("null");
}

static void _handlebars_ast_to_string_hash(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    struct handlebars_ast_list * ast_list = ast_node->node.hash.pairs;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    bool prev_in_partial = ctx->in_partial;
    ctx->in_partial = false;

    handlebars_ast_list_foreach(ast_list, item, tmp) {
        __APPEND_STR(item->data->node.hash_pair.key);
        __APPENDS("=");
        __PRINT(item->data->node.hash_pair.value);
        if( item->next ) {
            __APPENDS(" ");
        }
    }

    ctx->in_partial = prev_in_partial;
}




static void _handlebars_ast_to_string_path(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx, bool first_only)
{
    struct handlebars_ast_list * ast_list = ast_node->node.path.parts;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;

    assert(ast_node->node.path.data == 0 || ast_node->node.path.data == 1);

    if (ast_node->node.path.original) {
        __APPEND_STR(ast_node->node.path.original);
    } else {
        if( ast_node->node.path.data ) {
            __APPENDS("@");
        }

        if( !ast_list || handlebars_ast_list_count(ast_list) <= 0 ) {
            __APPENDS(".");
            return;
        }

        handlebars_ast_list_foreach(ast_list, item, tmp) {
            __APPEND_STR(item->data->node.path_segment.part);
            // if (first_only) {
            //     return;
            // }
            if( item->next ) {
                __APPEND_STR(item->next->data->node.path_segment.separator);
            }
        }
    }
}

static void _handlebars_ast_to_string(struct handlebars_ast_node * ast_node, struct handlebars_ast_printer_context * ctx)
{
    if( unlikely(ast_node == NULL) ) {
        return;
    }

    switch( ast_node->type ) {
        case HANDLEBARS_AST_NODE_BOOLEAN:
            return _handlebars_ast_to_string_boolean(ast_node, ctx);
        case HANDLEBARS_AST_NODE_BLOCK:
            return _handlebars_ast_to_string_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_COMMENT:
            return _handlebars_ast_to_string_comment(ast_node, ctx);
        case HANDLEBARS_AST_NODE_CONTENT:
            return _handlebars_ast_to_string_content(ast_node, ctx);
        case HANDLEBARS_AST_NODE_HASH:
            return _handlebars_ast_to_string_hash(ast_node, ctx);
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return _handlebars_ast_to_string_mustache(ast_node, ctx);
        case HANDLEBARS_AST_NODE_NUL:
            return _handlebars_ast_to_string_null(ast_node, ctx);
        case HANDLEBARS_AST_NODE_NUMBER:
            return _handlebars_ast_to_string_number(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PARTIAL:
            return _handlebars_ast_to_string_partial(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            return _handlebars_ast_to_string_partial_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_PATH:
            return _handlebars_ast_to_string_path(ast_node, ctx, 0);
        case HANDLEBARS_AST_NODE_PROGRAM:
            return _handlebars_ast_to_string_program(ast_node, ctx);
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return _handlebars_ast_to_string_block(ast_node, ctx);
        case HANDLEBARS_AST_NODE_SEXPR:
            return _handlebars_ast_to_string_sexpr(ast_node, ctx);
        case HANDLEBARS_AST_NODE_STRING:
            return _handlebars_ast_to_string_string(ast_node, ctx);
        case HANDLEBARS_AST_NODE_UNDEFINED:
            return _handlebars_ast_to_string_undefined(ast_node, ctx);
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

struct handlebars_string * handlebars_ast_to_string(
    struct handlebars_context * context,
    struct handlebars_ast_node * ast_node
) {
    // return handlebars_string_ctor(context, "", 0);
    // return handlebars_ast_print(context, ast_node);
    struct handlebars_string * output;
    struct handlebars_ast_printer_context * ctx = MC(handlebars_talloc_zero(context, struct handlebars_ast_printer_context));
    ctx->ctx = context;
    ctx->output = handlebars_string_ctor(context, "", 0);
    _handlebars_ast_to_string(ast_node, ctx);
    output = talloc_steal(context, ctx->output);
    handlebars_talloc_free(ctx);
    return output;
}

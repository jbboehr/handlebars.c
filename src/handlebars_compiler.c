/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_AST_LIST_PRIVATE
#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_ast_list.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_private.h"
#include "handlebars_string.h"

// @TODO fix these?
#pragma GCC diagnostic warning "-Winline"



#define __MK(type) handlebars_opcode_type_ ## type
#define __PUSH(opcode) handlebars_compiler_opcode(compiler, opcode)

#define __OPN(type) __PUSH(handlebars_opcode_ctor(CONTEXT, __MK(type)))

#define __OPB(type, arg) do { \
        struct handlebars_opcode * ____opcode = handlebars_opcode_ctor(CONTEXT, __MK(type)); \
        handlebars_operand_set_boolval(&____opcode->op1, arg); \
        __PUSH(____opcode); \
    } while(0)

#define __OPL(type, arg) do { \
        struct handlebars_opcode * ____opcode = handlebars_opcode_ctor(CONTEXT, __MK(type)); \
        handlebars_operand_set_longval(&____opcode->op1, arg); \
        __PUSH(____opcode); \
    } while(0)

#define __OPS(type, arg) do { \
        struct handlebars_opcode * ____opcode = handlebars_opcode_ctor(CONTEXT, __MK(type)); \
        handlebars_operand_set_stringval(CONTEXT, ____opcode, &____opcode->op1, arg); \
        __PUSH(____opcode); \
    } while(0)

#undef CONTEXT
#define CONTEXT HBSCTX(compiler)

enum handlebars_compiler_sexpr_type {
    SEXPR_AMBIG = 0,
    SEXPR_HELPER = 1,
    SEXPR_SIMPLE = 2
};

/**
 * @brief Main compiler state struct
 */
struct handlebars_compiler {
    struct handlebars_context ctx;
    struct handlebars_program * program;
    struct handlebars_block_param_stack * bps;
    struct handlebars_source_node_stack * sns;

    /**
     * @brief Array of known helpers
     */
    const char ** known_helpers;

    /**
     * @brief Symbol index counter
     */
    long guid;

    /**
     * @brief Compiler flags
     */
    unsigned long flags;
};

struct handlebars_block_param_pair {
    struct handlebars_string * block_param1;
    struct handlebars_string * block_param2;
};

struct handlebars_block_param_stack {
    struct handlebars_block_param_pair s[HANDLEBARS_COMPILER_STACK_SIZE];
    int i;
};

struct handlebars_source_node_stack {
    struct handlebars_ast_node * s[HANDLEBARS_COMPILER_STACK_SIZE];
    int i;
};




static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
);

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr
);

static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr,
        long programGuid,
        long inverseGuid
);

static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr
);

static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr,
        long programGuid,
        long inverseGuid,
        struct handlebars_ast_node * programNode
);

static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
);

static inline bool handlebars_compiler_block_param_index(
        struct handlebars_compiler * compiler,
        struct handlebars_string * name,
        int * depth,
        int * param
);

static inline void handlebars_compiler_accept_decorator(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * ast_node,
        long programGuid,
        long inverseGuid
);

const size_t HANDLEBARS_COMPILER_SIZE = sizeof(struct handlebars_compiler);
const size_t HANDLEBARS_PROGRAM_SIZE = sizeof(struct handlebars_compiler);



struct handlebars_compiler * handlebars_compiler_ctor(struct handlebars_context * context)
{
    struct handlebars_compiler * compiler;
    compiler = handlebars_talloc_zero(context, struct handlebars_compiler);
    HANDLEBARS_MEMCHECK(compiler, context);
    handlebars_context_bind(context, HBSCTX(compiler));
    compiler->program = MC(handlebars_talloc_zero(compiler, struct handlebars_program));
    compiler->program->main = compiler->program;
    compiler->known_helpers = handlebars_builtins_names();
    return compiler;
};

void handlebars_compiler_dtor(struct handlebars_compiler * compiler)
{
    assert(compiler != NULL);

    handlebars_talloc_free(compiler);
};

unsigned long handlebars_compiler_get_flags(struct handlebars_compiler * compiler)
{
    assert(compiler != NULL);

    return compiler->flags;
}

void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, unsigned long flags)
{
    assert(compiler != NULL);

    // Only allow option flags to be changed
    flags = flags & handlebars_compiler_flag_all;
    compiler->flags = compiler->flags & ~handlebars_compiler_flag_all;
    compiler->flags = compiler->flags | flags;
}

struct handlebars_program * handlebars_compiler_get_program(
    struct handlebars_compiler * compiler
) {
    return compiler->program;
}

const char ** handlebars_compiler_get_known_helpers(
    struct handlebars_compiler * compiler
) {
    return compiler->known_helpers;
}

void handlebars_compiler_set_known_helpers(
    struct handlebars_compiler * compiler,
    const char ** known_helpers
) {
    compiler->known_helpers = known_helpers;
}



// Utilities

struct handlebars_string ** handlebars_ast_node_get_id_parts(struct handlebars_compiler * compiler, struct handlebars_ast_node * ast_node)
{
    size_t num;
    struct handlebars_string ** arr;
    struct handlebars_string ** arrptr;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;

    assert(ast_node != NULL);

    if( ast_node->type != HANDLEBARS_AST_NODE_PATH ) {
        return NULL;
    }

    num = handlebars_ast_list_count(ast_node->node.path.parts);
    if( num == 0 ) {
        return NULL;
    }

    arrptr = arr = MC(handlebars_talloc_array(compiler, struct handlebars_string *, num + 1));

    handlebars_ast_list_foreach(ast_node->node.path.parts, item, tmp) {
        assert(item->data);
        if( likely(item->data->type == HANDLEBARS_AST_NODE_PATH_SEGMENT) ) {
            struct handlebars_string * part = item->data->node.path_segment.part;
            *arrptr++ = handlebars_string_copy_ctor(CONTEXT, part);
        }
    }
    *arrptr++ = NULL;

    return arr;
}

static inline void handlebars_compiler_add_depth(
        struct handlebars_compiler * compiler,
        int depth
) {
    assert(compiler != NULL);

    if( depth > 0 ) {
        compiler->program->result_flags |= handlebars_compiler_result_flag_use_depths;
    }
}

bool handlebars_compiler_is_known_helper(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * path
) {
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    struct handlebars_string * helper_name;
    const char ** ptr;

    assert(compiler != NULL);
    assert(path != NULL);

    if( path->type != HANDLEBARS_AST_NODE_PATH ||
        NULL == (parts = path->node.path.parts) ||
        NULL == parts->first ||
        NULL == (path_segment = parts->first->data) ||
        NULL == (helper_name = path_segment->node.path_segment.part) ) {
        return false;
    }

    if( NULL != handlebars_builtins_find(hbs_str_val(helper_name), (unsigned int) hbs_str_len(helper_name)) ) {
        return true;
    }

    for( ptr = compiler->known_helpers ; *ptr ; ++ptr ) {
        if( hbs_str_eq_strl(helper_name, *ptr, strlen(*ptr)) ) {
            return true;
        }
    }

    return false;
}

static inline enum handlebars_compiler_sexpr_type handlebars_compiler_classify_sexpr(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr
) {
    struct handlebars_ast_node * path;
    struct handlebars_string * part;
    bool is_simple;
    bool is_block_param;
    bool is_helper;
    bool is_eligible;
    int ignoreme;

    assert(compiler != NULL);
    assert(sexpr != NULL);

    path = handlebars_ast_node_get_path(sexpr);

    assert(path != NULL);
    // Should be ok to put this back now that transform to path is working
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    assert(path->node.path.parts != NULL);

    part = handlebars_ast_node_get_id_part(path);

    is_simple = handlebars_ast_helper_simple_id(path);
    is_block_param = is_simple && part && handlebars_compiler_block_param_index(compiler, part, &ignoreme, &ignoreme);
    is_helper = !is_block_param && handlebars_ast_helper_helper_expression(sexpr);
    is_eligible = !is_block_param && (is_simple || is_helper);

    if( is_eligible && !is_helper ) {
        if( handlebars_compiler_is_known_helper(compiler, path) ) {
            is_helper = 1;
        } else if( compiler->flags & handlebars_compiler_flag_known_helpers_only ) {
            is_eligible = 0;
        }
    }

    if( is_helper ) {
        return SEXPR_HELPER;
    } else if( is_eligible ) {
        return SEXPR_AMBIG;
    } else {
        return SEXPR_SIMPLE;
    }
}

static inline long handlebars_compiler_compile_program(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
) {
    struct handlebars_program * program;
    struct handlebars_compiler * subcompiler;
    long guid;

    if( node == NULL ) {
        return -1;
    }

    assert(compiler != NULL);
    assert(node->type == HANDLEBARS_AST_NODE_PROGRAM ||
                   node->type == HANDLEBARS_AST_NODE_CONTENT);

    program = compiler->program;
    subcompiler = MC(handlebars_compiler_ctor(HBSCTX(compiler)));
    subcompiler->program->main = program->main;

    // copy compiler flags, bps, and options
    handlebars_compiler_set_flags(subcompiler, handlebars_compiler_get_flags(compiler));
    subcompiler->bps = compiler->bps;
    subcompiler->sns = compiler->sns;
    subcompiler->known_helpers = compiler->known_helpers;

    // compile
    handlebars_compiler_compile(subcompiler, node);
    subcompiler->program->flags = subcompiler->flags;
    guid = compiler->guid++;

    // Don't propogate use_decorators
    program->result_flags |= (subcompiler->program->result_flags & ~handlebars_compiler_result_flag_use_decorators);

    // Realloc children array
    if( program->children_size <= program->children_length ) {
        program->children_size += 2;
        program->children = MC(handlebars_talloc_realloc(program, program->children,
                    struct handlebars_program *, program->children_size));
    }

    // Append child
    program->children[program->children_length++] = talloc_steal(program, subcompiler->program);

    handlebars_talloc_free(subcompiler);
    return guid;
}

/**
 * We mark this as extern for the test suite...
 */
void handlebars_compiler_opcode(
        struct handlebars_compiler * compiler,
        struct handlebars_opcode * opcode
) {
    struct handlebars_program * program = compiler->program;

    // Realloc opcode array
    if( program->opcodes_size <= program->opcodes_length ) {
        program->opcodes = MC(handlebars_talloc_realloc(program, program->opcodes,
                    struct handlebars_opcode *, program->opcodes_size + 32));
        program->opcodes_size += 32;
    }

    // Get location from source node stack
    if (likely(compiler->sns && compiler->sns->i > 0)) {
        handlebars_opcode_set_loc(opcode, compiler->sns->s[compiler->sns->i - 1]->loc);
    }

    // Append opcode
    program->opcodes[program->opcodes_length++] = talloc_steal(program, opcode);
}

static inline void push_program_pair(struct handlebars_compiler * compiler, long programGuid, long inverseGuid)
{
    if( programGuid == -1 ) {
        __OPN(push_program);
    } else {
        __OPL(push_program, programGuid);
    }
    if( inverseGuid == -1 ) {
        __OPN(push_program);
    } else {
        __OPL(push_program, inverseGuid);
    }
}

static inline void handlebars_compiler_push_param(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * param
) {
    struct handlebars_string * string;

    assert(compiler != NULL);
    assert(param != NULL);
    if( !param ) {
        return;
    }

    if( compiler->flags & handlebars_compiler_flag_string_params ) {
        int depth = 0;
        struct handlebars_opcode * opcode;

        if( param->type == HANDLEBARS_AST_NODE_PATH ) {
            depth = param->node.path.depth;
        }

        if( depth ) {
            handlebars_compiler_add_depth(compiler, depth);
        }
        __OPL(get_context, depth);

        string = handlebars_ast_node_get_string_mode_value(CONTEXT, param);

        // sigh
        opcode = MC(handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_string_param));

        if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
            handlebars_operand_set_boolval(&opcode->op1, string && hbs_str_eq_strl(string, HBS_STRL("true")));
        } else {
            // @todo should be /^(\.\.?\/)/
            string = handlebars_string_copy_ctor(CONTEXT, string);
            string = handlebars_string_ltrim(string, HBS_STRL("./"));
            for( char * tmp2 = hbs_str_val(string); *tmp2; tmp2++ ) {
                if( *tmp2 == '/' ) {
                    *tmp2 = '.';
                }
            }
            handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, string);
        }
        const char * name = handlebars_ast_node_readable_type(param->type);
        handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, handlebars_string_ctor(CONTEXT, name, strlen(name)));
        __PUSH(opcode);

        if( param->type == HANDLEBARS_AST_NODE_SEXPR ) {
            handlebars_compiler_accept/*_sexpr*/(compiler, param);
        }
    } else {
        if( compiler->flags & handlebars_compiler_flag_track_ids ) {
            struct handlebars_string * part;
            struct handlebars_opcode * opcode;
            int block_param_depth = -1;
            int block_param_num = -1;
            char tmp[32];
            const char * block_param_arr[3];
            struct handlebars_string ** parts_arr;
            struct handlebars_string * block_param_parts;

            opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_id);

            if( param->type == HANDLEBARS_AST_NODE_PATH ) {
                part = handlebars_ast_node_get_id_part(param);
                if( part && !handlebars_ast_helper_scoped_id(param) && !param->node.path.depth ) {
                    handlebars_compiler_block_param_index(compiler, part, &block_param_depth, &block_param_num);
                }
            }

            if( block_param_num >= 0 ) {
                snprintf(tmp, 15, "%d", block_param_depth);
                snprintf(tmp + 16, 15, "%d", block_param_num);
                block_param_arr[0] = &tmp[0];
                block_param_arr[1] = &tmp[16];
                block_param_arr[2] = NULL;

                handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, handlebars_string_ctor(CONTEXT, HBS_STRL("BlockParam")));
                handlebars_operand_set_arrayval(CONTEXT, opcode, &opcode->op2, block_param_arr);
                parts_arr = MC(handlebars_ast_node_get_id_parts(compiler, param));
                if( *parts_arr ) {
                    block_param_parts = MC(handlebars_string_implode(CONTEXT, HBS_STRL("."), /*(const struct handlebars_string **)*/ parts_arr + 1));
                    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, block_param_parts);
                } else {
                    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, handlebars_string_init(CONTEXT, 0));
                }
                handlebars_talloc_free(parts_arr);
                __PUSH(opcode);
            } else {
                string = handlebars_ast_node_get_string_mode_value(CONTEXT, param);
                const char * strval = hbs_str_val(string);
                if( strval == strstr(strval, "this") ) {
                	strval += 4 + (*(strval + 4) == '.' || *(strval + 4) == '$' ? 1 : 0 );
                }
                if( *strval == '.' && *(strval + 1) == 0 ) {
                    strval = "";
                } else if( *strval == '.' && *(strval + 1) == '/' ) {
                    strval += 2;
                }
                const char * name = handlebars_ast_node_readable_type(param->type);
                handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, handlebars_string_ctor(CONTEXT, name, strlen(name)));
                // @todo need to remove leading .
                if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
                    handlebars_operand_set_boolval(&opcode->op2, strcmp(strval, "true") == 0);
                } else if( param->type == HANDLEBARS_AST_NODE_NUMBER ) {
                    long lval;
                    sscanf(strval, "%10ld", &lval);
                    handlebars_operand_set_longval(&opcode->op2, lval);
                } else {
                    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, handlebars_string_ctor(CONTEXT, strval, strlen(strval)));
                }
                __PUSH(opcode);
            }
        }
        handlebars_compiler_accept(compiler, param);
    }
}

static inline void handlebars_compiler_push_params(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_list * params
) {
    assert(compiler != NULL);

    if( likely(params != NULL) ) {
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;

        handlebars_ast_list_foreach(params, item, tmp) {
            handlebars_compiler_push_param(compiler, item->data);
        }
    }
}

static inline struct handlebars_ast_list * handlebars_compiler_setup_full_mustache_params(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr,
        long programGuid,
        long inverseGuid,
        bool omit_empty
) {
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;

    assert(compiler != NULL);
    assert(sexpr != NULL);

    params = handlebars_ast_node_get_params(sexpr);
    hash = handlebars_ast_node_get_hash(sexpr);

    handlebars_compiler_push_params(compiler, params);
    push_program_pair(compiler, programGuid, inverseGuid);

    if( hash ) {
        handlebars_compiler_accept/*_hash*/(compiler, hash);
    } else if( omit_empty ) {
        __OPB(empty_hash, omit_empty);
    } else {
        __OPN(empty_hash);
    }

    return params;
}

static inline void handlebars_compiler_transform_literal_to_path(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
) {
	struct handlebars_ast_node * path = handlebars_ast_node_get_path(node);
	struct handlebars_ast_node * part;
	struct handlebars_ast_list * parts;
	struct handlebars_string * val;
	switch( path->type ) {
		case HANDLEBARS_AST_NODE_UNDEFINED:
		case HANDLEBARS_AST_NODE_NUL:
		case HANDLEBARS_AST_NODE_BOOLEAN:
		case HANDLEBARS_AST_NODE_NUMBER:
		case HANDLEBARS_AST_NODE_STRING:
			val = handlebars_ast_node_get_string_mode_value(CONTEXT, path);
			// Make parts
			part = talloc_steal(node, handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PATH_SEGMENT));
			part->node.path_segment.part = talloc_steal(part, handlebars_string_copy_ctor(CONTEXT, val));
			parts = talloc_steal(node, handlebars_ast_list_ctor(CONTEXT));
		    handlebars_ast_list_append(parts, part);
		    // Re-jigger node
		    memset(path, 0, sizeof(struct handlebars_ast_node));
			path->type = HANDLEBARS_AST_NODE_PATH;
			path->node.path.original = talloc_steal(part, handlebars_string_copy_ctor(CONTEXT, val));
			path->node.path.parts = talloc_steal(path, parts);
			break;
        default:
            // do nothing
            break;
	}
}

static inline bool handlebars_compiler_block_param_index(
        struct handlebars_compiler * compiler,
        struct handlebars_string * name,
        int * depth,
        int * param
) {
    int i;
    int l = compiler->bps->i;
    struct handlebars_string * block_param1;
    struct handlebars_string * block_param2;
    *param = -1;

    if( !name ) { // @todo double-check
    	return 0;
    }

    for( i = 0; i < l; i++ ) {
        block_param1 = compiler->bps->s[l - i - 1].block_param1;
        block_param2 = compiler->bps->s[l - i - 1].block_param2;
        if( block_param1 && handlebars_string_eq(block_param1, name) ) {
            *param = 0;
            *depth = i;
            return 1;
        } else if( block_param2 && handlebars_string_eq(block_param2, name) ) {
            *param = 1;
            *depth = i;
            return 1;
        }
    }

    return 0;
}



// Acceptors

static inline void handlebars_compiler_accept_program(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
) {
    struct handlebars_ast_list * list = node->node.program.statements;
    struct handlebars_string * block_param1 = node->node.program.block_param1;
    struct handlebars_string * block_param2 = node->node.program.block_param2;
    size_t statement_count = 0;

    assert(compiler != NULL);

    if( compiler->bps->i > HANDLEBARS_COMPILER_STACK_SIZE ) {
        handlebars_throw(CONTEXT, HANDLEBARS_STACK_OVERFLOW, "Block param stack blown");
    }
    compiler->bps->s[compiler->bps->i].block_param1 = block_param1;
    compiler->bps->s[compiler->bps->i].block_param2 = block_param2;
    compiler->bps->i++;

    if( likely(list != NULL) ) {
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;

        handlebars_ast_list_foreach(list, item, tmp) {
            handlebars_compiler_accept(compiler, item->data);
            statement_count++;
        }
    }

    compiler->bps->i--;
    if( 1 == statement_count ) {
        compiler->program->result_flags |= handlebars_compiler_result_flag_is_simple;
    }
    if( block_param1 ) {
        compiler->program->block_params++;
    }
    if( block_param2 ) {
        compiler->program->block_params++;
    }
    // @todo fix
    // this.blockParams = program.blockParams ? program.blockParams.length : 0;
}

static inline void handlebars_compiler_accept_block(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * block
) {
    struct handlebars_ast_node * sexpr = block; // cough
    struct handlebars_ast_node * program = block->node.block.program;
    struct handlebars_ast_node * inverse = block->node.block.inverse;
    struct handlebars_ast_node * path = block->node.block.path;
    long programGuid = -1;
    long inverseGuid = -1;

    assert(compiler != NULL);

    handlebars_compiler_transform_literal_to_path(compiler, block);

    programGuid = handlebars_compiler_compile_program(compiler, program);

    if( block->node.block.is_decorator ) {
        // Decorator
    	handlebars_compiler_accept_decorator(compiler, block, programGuid, inverseGuid);
    } else {
		// Normal
        inverseGuid = handlebars_compiler_compile_program(compiler, inverse);

		switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
			case SEXPR_HELPER:
				handlebars_compiler_accept_sexpr_helper(compiler, sexpr, programGuid, inverseGuid);
				break;
			case SEXPR_SIMPLE:
				handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
                push_program_pair(compiler, programGuid, inverseGuid);
				__OPN(empty_hash);
				__OPS(block_value, path->node.path.original);
				break;
			case SEXPR_AMBIG:
				handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, programGuid, inverseGuid, program);
                push_program_pair(compiler, programGuid, inverseGuid);
				__OPN(empty_hash);
				__OPN(ambiguous_block_value);
				break;
            default: assert(0); break; // LCOV_EXCL_LINE
		}

		__OPN(append);
    }
}

static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * hash
) {
    struct handlebars_ast_list * params = hash->node.hash.pairs;
    size_t len = handlebars_ast_list_count(params);
    size_t i = 0;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    struct handlebars_string ** keys;

    assert(compiler != NULL);

    keys = MC(handlebars_talloc_array(compiler, struct handlebars_string *, len));

    __OPN(push_hash);

    handlebars_ast_list_foreach(params, item, tmp) {
        struct handlebars_ast_node * hash_pair = item->data;

        assert(hash_pair != NULL);
        assert(hash_pair->type == HANDLEBARS_AST_NODE_HASH_PAIR);

        if( likely(hash_pair->type == HANDLEBARS_AST_NODE_HASH_PAIR) ) {
            handlebars_compiler_push_param(compiler, hash_pair->node.hash_pair.value);
            keys[i++] = hash_pair->node.hash_pair.key;
        }
    }

    while( i-- ) {
        __OPS(assign_to_hash, keys[i]);
    }

    __OPN(pop_hash);

    handlebars_talloc_free(keys);
}

static inline void _handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node,
        struct handlebars_ast_node * name,
        struct handlebars_ast_list * params,
        struct handlebars_ast_node * program,
        struct handlebars_string * indent
) {
    size_t count = 0;
    bool is_dynamic = false;
    long programGuid = -1;
    struct handlebars_ast_node * tmp;

    assert(compiler != NULL);
    assert(node != NULL);
    assert(name != NULL);

    compiler->program->result_flags |= handlebars_compiler_result_flag_use_partial;

    count = (params ? handlebars_ast_list_count(params) : 0);

    if( count > 1 ) {
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_UNSUPPORTED_PARTIAL_ARGS,
            &node->loc,
            "Unsupported number of partial arguments"
        );
    } else if( !params || !handlebars_ast_list_count(params) ) {
    	if( compiler->flags & handlebars_compiler_flag_explicit_partial_context ) {
            struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_literal);
            handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, handlebars_string_ctor(CONTEXT, HBS_STRL("undefined")));
            __PUSH(opcode);
    		//__OPS(push_literal, "undefined";
    	} else {
			if( !params ) {
				params = talloc_steal(node, handlebars_ast_list_ctor(CONTEXT));
				if( node->type == HANDLEBARS_AST_NODE_PARTIAL ) {
					node->node.partial.params = params;
				} else if( node->type == HANDLEBARS_AST_NODE_PARTIAL_BLOCK ) {
					node->node.partial_block.params = params;
				}
			}
			tmp = talloc_steal(params, handlebars_ast_node_ctor(CONTEXT, HANDLEBARS_AST_NODE_PATH));
			handlebars_ast_list_append(params, tmp);
    	}
    }

    if( name->type == HANDLEBARS_AST_NODE_SEXPR ) {
    	is_dynamic = true;
    }

    if( is_dynamic ) {
    	handlebars_compiler_accept(compiler, name);
    }

    programGuid = handlebars_compiler_compile_program(compiler, program);

    handlebars_compiler_setup_full_mustache_params(compiler, node, programGuid, -1, 1);

    if( (compiler->flags & handlebars_compiler_flag_prevent_indent) && indent && hbs_str_len(indent) > 0 ) {
        __OPS(append_content, indent);
        indent = NULL;
        /*
        indent->len = 0;
        indent->val[0] = 0;
         */
    }

    do {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_invoke_partial);
        handlebars_operand_set_boolval(&opcode->op1, is_dynamic);
        if( !is_dynamic ) {
            struct handlebars_string * string = handlebars_ast_node_get_string_mode_value(CONTEXT, name);
            long lv = 0;
        	double fv;
            if( name->type == HANDLEBARS_AST_NODE_NUMBER ) {
                fv = strtod(hbs_str_val(string), NULL);
                lv = strtol(hbs_str_val(string), NULL, 10);
                if( !fv || !lv || fv != (double) lv ) {
                    lv = 0;
                }
            }
            if( lv ) {
                handlebars_operand_set_longval(&opcode->op2, lv);
            } else {
                handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, string);
            }
        }
        if( indent ) {
            handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, indent);
        } else {
            handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, handlebars_string_ctor(CONTEXT, HBS_STRL("")));
        }
        __PUSH(opcode);
    } while(0);

    __OPN(append);
}

static inline void handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * partial
) {
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    struct handlebars_string * indent;

    assert(compiler != NULL);
    assert(partial != NULL);
    assert(partial->type == HANDLEBARS_AST_NODE_PARTIAL);

    name = partial->node.partial.name;
    params = partial->node.partial.params;
    indent = partial->node.partial.indent;

    _handlebars_compiler_accept_partial(compiler, partial, name, params, NULL, indent);
}

static inline void handlebars_compiler_accept_partial_block(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * partial_block
) {
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * program;
    struct handlebars_string * indent;

    assert(compiler != NULL);
    assert(partial_block != NULL);
    assert(partial_block->type == HANDLEBARS_AST_NODE_PARTIAL_BLOCK);

    name = partial_block->node.partial_block.path;
    params = partial_block->node.partial_block.params;
    program = partial_block->node.partial_block.program;
    // @todo ?
    indent = NULL; //partial_block->node.partial_block.indent;

    _handlebars_compiler_accept_partial(compiler, partial_block, name, params, program, indent);
}

static inline void handlebars_compiler_accept_content(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * content
) {
    assert(compiler != NULL);
    assert(content != NULL);
    assert(content->type == HANDLEBARS_AST_NODE_CONTENT);

    if( likely(/* content && */ content->node.content.value && hbs_str_len(content->node.content.value) > 0) ) {
        __OPS(append_content, content->node.content.value);
    }
}

static inline void handlebars_compiler_accept_decorator(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * ast_node,
        long programGuid,
        long inverseGuid
) {
    struct handlebars_program * program = compiler->program;
    struct handlebars_ast_node * path = handlebars_ast_node_get_path(ast_node);
    struct handlebars_ast_list * params;
    struct handlebars_string * original;
    struct handlebars_compiler * origcompiler;
    struct handlebars_compiler * subcompiler;

    if( compiler->flags & handlebars_compiler_flag_alternate_decorators ) {
        // Realloc decorators array
        if( program->decorators_size <= program->decorators_length ) {
            program->decorators = MC(handlebars_talloc_realloc(program, program->decorators,
                        struct handlebars_program *, program->decorators_size + 8));
            program->decorators_size += 8;
        }

        origcompiler = compiler;
        subcompiler = talloc_steal(compiler, handlebars_compiler_ctor(CONTEXT));
        program->decorators[program->decorators_length++] = talloc_steal(program, subcompiler->program);
        compiler = subcompiler;

        origcompiler->program->result_flags |= handlebars_compiler_result_flag_use_decorators;
    } else {
        compiler->program->result_flags |= handlebars_compiler_result_flag_use_decorators;
    }

	original = handlebars_ast_node_get_string_mode_value(CONTEXT, path);
	params = handlebars_compiler_setup_full_mustache_params(
                compiler, ast_node, programGuid, inverseGuid, 0);

    struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_register_decorator);
    handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, original);
    __PUSH(opcode);
}

static inline void handlebars_compiler_accept_mustache(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * mustache
) {
    assert(compiler != NULL);
    assert(mustache != NULL);
    assert(mustache->type == HANDLEBARS_AST_NODE_MUSTACHE);

    if( mustache->node.mustache.is_decorator ) {
        handlebars_compiler_accept_decorator(compiler, mustache, -1, -1);
    } else {
        handlebars_compiler_accept_sexpr(compiler, mustache);

        if( !mustache->node.mustache.unescaped && !((compiler->flags & handlebars_compiler_flag_no_escape)) ) {
            __OPN(append_escaped);
        } else {
            __OPN(append);
        }
    }
}

static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr,
        long programGuid,
        long inverseGuid,
        struct handlebars_ast_node * programNode
) {
    struct handlebars_ast_node * path;
    struct handlebars_string * name;
    bool is_block = (programGuid >= 0 || inverseGuid >= 0);

    assert(compiler != NULL);
    assert(sexpr != NULL);

    path = handlebars_ast_node_get_path(sexpr);

    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);

    name = handlebars_ast_node_get_id_part(path);

    __OPL(get_context, path->node.path.depth);
    push_program_pair(compiler, programGuid, inverseGuid);

	path->node.path.strict = true;
    handlebars_compiler_accept/*_id*/(compiler, path);

    do {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_invoke_ambiguous);
        handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, name);
        handlebars_operand_set_boolval(&opcode->op2, is_block);
        if (compiler->flags & handlebars_compiler_flag_mustache_style_lambdas) {
            if (programNode) {
                handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, handlebars_ast_to_string(CONTEXT, programNode));
            } else {
                handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op3, handlebars_string_ctor(CONTEXT, HBS_STRL("")));
            }
        }
        __PUSH(opcode);
        //__OPSL(invoke_ambiguous, name, is_block);
    } while(0);
}

static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr
) {
    struct handlebars_ast_node * path;

    assert(compiler != NULL);
    assert(sexpr != NULL);

    path = handlebars_ast_node_get_path(sexpr);

    if( path ) {
    	path->node.path.strict = true;
        handlebars_compiler_accept/*_data*/(compiler, path);
    }

    __OPN(resolve_possible_lambda);
}

static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr,
        long programGuid,
        long inverseGuid
) {
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    struct handlebars_string * name;

    assert(compiler != NULL);
    assert(sexpr != NULL);

    path = handlebars_ast_node_get_path(sexpr);
    params = handlebars_compiler_setup_full_mustache_params(
                compiler, sexpr, programGuid, inverseGuid, 0);

    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);

    name = handlebars_ast_node_get_id_part(path);

    if( handlebars_compiler_is_known_helper(compiler, path) ) {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_invoke_known_helper);
        handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
        handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, name);
        __PUSH(opcode);
    } else if( compiler->flags & handlebars_compiler_flag_known_helpers_only ) {
        handlebars_throw_ex(
            CONTEXT,
            HANDLEBARS_UNKNOWN_HELPER,
            &path->loc,
            "You specified knownHelpersOnly, but used the unknown helper %.*s",
            (int) hbs_str_len(name), hbs_str_val(name)
        );
    } else {
        bool is_simple = handlebars_ast_helper_simple_id(path);

        path->node.path.falsy = true;
    	path->node.path.strict = true;
        handlebars_compiler_accept/*_id*/(compiler, path);

        struct handlebars_opcode * opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_invoke_helper);
        handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
        //handlebars_operand_set_stringval(compiler, &opcode->op2, name);
        handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op2, path->node.path.original);
        handlebars_operand_set_boolval(&opcode->op3, is_simple);
        handlebars_compiler_opcode(compiler, opcode);
    }
}

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr
) {
    assert(compiler != NULL);
    assert(sexpr != NULL);

    handlebars_compiler_transform_literal_to_path(compiler, sexpr);

    switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
        case SEXPR_HELPER:
            handlebars_compiler_accept_sexpr_helper(compiler, sexpr, -1, -1);
            break;
        case SEXPR_SIMPLE:
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
            break;
        case SEXPR_AMBIG:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, -1, -1, NULL);
            break;
        default:
        	assert(0);
    }
}

static inline void handlebars_compiler_accept_path(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * path
) {
    struct handlebars_string * name;
    bool is_scoped;
    struct handlebars_opcode * opcode;
    int block_param_depth = -1;
    int block_param_num = -1;
    struct handlebars_string ** parts_arr;

    assert(compiler != NULL);
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);

    handlebars_compiler_add_depth(compiler, path->node.path.depth);
    __OPL(get_context, path->node.path.depth);

    name = handlebars_ast_node_get_id_part(path);
    is_scoped = handlebars_ast_helper_scoped_id(path);
    if( !path->node.path.depth && !is_scoped ) {
        handlebars_compiler_block_param_index(compiler, name, &block_param_depth, &block_param_num);
    }

    if( block_param_depth >= 0 ) {
        char tmp[32];
        const char * block_param_arr[3];
        snprintf(tmp, 15, "%d", block_param_depth);
        snprintf(tmp + 16, 15, "%d", block_param_num);
        block_param_arr[0] = &tmp[0];
        block_param_arr[1] = &tmp[16];
        block_param_arr[2] = NULL;

        opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_lookup_block_param);
        handlebars_operand_set_arrayval(CONTEXT, opcode, &opcode->op1, block_param_arr);
        parts_arr = MC(handlebars_ast_node_get_id_parts(compiler, path));
        handlebars_operand_set_arrayval_string(CONTEXT, opcode, &opcode->op2, parts_arr);
        handlebars_talloc_free(parts_arr);
        __PUSH(opcode);
    } else if( name == NULL ) {
        __OPN(push_context);
    } else if( path->node.path.data ) {
        opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_lookup_data);
        handlebars_operand_set_longval(&opcode->op1, path->node.path.depth);
        parts_arr = MC(handlebars_ast_node_get_id_parts(compiler, path));
        handlebars_operand_set_arrayval_string(CONTEXT, opcode, &opcode->op2, parts_arr);
        handlebars_talloc_free(parts_arr);
        if( path->node.path.strict ) {
        	handlebars_operand_set_boolval(&opcode->op3, path->node.path.strict);
        }
        __PUSH(opcode);
    } else {
        opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_lookup_on_context);
        parts_arr = MC(handlebars_ast_node_get_id_parts(compiler, path));
        handlebars_operand_set_arrayval_string(CONTEXT, opcode, &opcode->op1, parts_arr);
        handlebars_talloc_free(parts_arr);

        if( path->node.path.falsy ) {
        	handlebars_operand_set_boolval(&opcode->op2, path->node.path.falsy);
        }
        if( path->node.path.strict ) {
        	handlebars_operand_set_boolval(&opcode->op3, path->node.path.strict);
        }
        handlebars_operand_set_boolval(&opcode->op4, is_scoped);
        __PUSH(opcode);
    }
}

static inline void handlebars_compiler_accept_string(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * string
) {
    assert(string != NULL);
    assert(string->type == HANDLEBARS_AST_NODE_STRING);

    __OPS(push_string, string->node.string.value);
}

static inline void handlebars_compiler_accept_number(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * number
) {
    struct handlebars_string * val = number->node.number.value;
	double fv;
	long lv;

    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_NUMBER);

    if (hbs_str_eq_strl(val, HBS_STRL("0"))) {
    	__OPL(push_literal, 0);
        return;
    }

    // Convert to float and long
    fv = strtod(hbs_str_val(val), NULL);
    lv = strtol(hbs_str_val(val), NULL, 10);

    if( fv && lv && fv == (double) lv ) {
    	// It's a long - @todo do we need to do float too?
    	__OPL(push_literal, lv);
    } else {
        __OPS(push_literal, number->node.number.value);
    }
}

static inline void handlebars_compiler_accept_boolean(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * boolean
) {
    struct handlebars_string * val = boolean->node.number.value;
    struct handlebars_opcode * opcode;

    assert(boolean != NULL);
    assert(boolean->type == HANDLEBARS_AST_NODE_BOOLEAN);

    opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_literal);
    handlebars_operand_set_boolval(&opcode->op1, !hbs_str_eq_strl(val, HBS_STRL("false")));
    __PUSH(opcode);
}


static inline void handlebars_compiler_accept_nul(
        struct handlebars_compiler * compiler,
        HBS_ATTR_UNUSED struct handlebars_ast_node * node
) {
    struct handlebars_opcode * opcode;

    assert(node != NULL);
    assert(node->type == HANDLEBARS_AST_NODE_NUL);

    opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_literal);
    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, handlebars_string_ctor(CONTEXT, HBS_STRL("null")));
    __PUSH(opcode);
}

static inline void handlebars_compiler_accept_undefined(
        struct handlebars_compiler * compiler,
        HBS_ATTR_UNUSED struct handlebars_ast_node * node
) {
    struct handlebars_opcode * opcode;

    assert(node != NULL);
    assert(node->type == HANDLEBARS_AST_NODE_UNDEFINED);

    opcode = handlebars_opcode_ctor(CONTEXT, handlebars_opcode_type_push_literal);
    handlebars_operand_set_stringval(CONTEXT, opcode, &opcode->op1, handlebars_string_ctor(CONTEXT, HBS_STRL("undefined")));
    __PUSH(opcode);
}

static inline void handlebars_compiler_accept_comment(
        HBS_ATTR_UNUSED struct handlebars_compiler * compiler,
        HBS_ATTR_UNUSED struct handlebars_ast_node * comment)
{
    assert(comment != NULL);
    assert(comment->type == HANDLEBARS_AST_NODE_COMMENT);
}

static inline void handlebars_compiler_accept_raw_block(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * raw_block
) {
    struct handlebars_ast_node * sexpr = raw_block;
    struct handlebars_ast_node * program = raw_block->node.raw_block.program;
    struct handlebars_ast_node * inverse = raw_block->node.raw_block.inverse;
    struct handlebars_ast_node * path = raw_block->node.raw_block.path;
    long programGuid;
    long inverseGuid;

    assert(compiler != NULL);

    handlebars_compiler_transform_literal_to_path(compiler, raw_block);

    programGuid = handlebars_compiler_compile_program(compiler, program);
    inverseGuid = handlebars_compiler_compile_program(compiler, inverse);

    switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
        case SEXPR_HELPER:
            handlebars_compiler_accept_sexpr_helper(compiler, sexpr, programGuid, inverseGuid);
            break;
        case SEXPR_SIMPLE:
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
            push_program_pair(compiler, programGuid, inverseGuid);
            __OPN(empty_hash);
            __OPS(block_value, path->node.path.original);
            break;
        case SEXPR_AMBIG:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, programGuid, inverseGuid, NULL);
            push_program_pair(compiler, programGuid, inverseGuid);
            __OPN(empty_hash);
            __OPN(ambiguous_block_value);
            break;
        default: assert(0); break; // LCOV_EXCL_LINE
    }

    __OPN(append);
}

static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
) {
    if( unlikely(node == NULL) ) {
        return;
    }

    // Add node to source node stack
    if( compiler->sns->i > HANDLEBARS_COMPILER_STACK_SIZE ) {
        handlebars_throw(CONTEXT, HANDLEBARS_STACK_OVERFLOW, "Source node stack blown");
    }
    compiler->sns->s[compiler->sns->i] = node;
    compiler->sns->i++;

    // Accept node
    switch( node->type ) {
		case HANDLEBARS_AST_NODE_BLOCK:
			handlebars_compiler_accept_block(compiler, node);
			break;
		case HANDLEBARS_AST_NODE_BOOLEAN:
			handlebars_compiler_accept_boolean(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_COMMENT:
            handlebars_compiler_accept_comment(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_CONTENT:
            handlebars_compiler_accept_content(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_HASH:
            handlebars_compiler_accept_hash(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_MUSTACHE:
            handlebars_compiler_accept_mustache(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_NUMBER:
            handlebars_compiler_accept_number(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_NUL:
			handlebars_compiler_accept_nul(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_PARTIAL:
            handlebars_compiler_accept_partial(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_PARTIAL_BLOCK:
            handlebars_compiler_accept_partial_block(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_PATH:
            handlebars_compiler_accept_path(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_PROGRAM:
            handlebars_compiler_accept_program(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_SEXPR:
            handlebars_compiler_accept_sexpr(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_STRING:
            handlebars_compiler_accept_string(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            handlebars_compiler_accept_raw_block(compiler, node);
			break;
        case HANDLEBARS_AST_NODE_UNDEFINED:
			handlebars_compiler_accept_undefined(compiler, node);
			break;

        // Should never get here
        default:
            assert(0);
            break;
    }

    // Pop source node stack
    compiler->sns->i--;
}

struct handlebars_program * handlebars_compiler_compile_ex(
    struct handlebars_compiler * compiler,
    struct handlebars_ast_node * node
) {
    handlebars_compiler_compile(compiler, node);
    return compiler->program;
}

void handlebars_compiler_compile(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * node
) {
    struct handlebars_error * e = HBSCTX(compiler)->e;
    jmp_buf * prev = e->jmp;
    jmp_buf buf;

    // Save jump buffer
    if( !prev ) {
        if( handlebars_setjmp_ex(compiler, &buf) ) {
            goto done;
        }
    }

    // Allocate stacks
    if (!compiler->bps) {
        compiler->bps = alloca(sizeof(struct handlebars_block_param_stack));
        compiler->bps->i = 0;
    }
    if (!compiler->sns) {
        compiler->sns = alloca(sizeof(struct handlebars_source_node_stack));
        compiler->sns->i = 0;
    }

    // Compile
    compiler->program->flags = compiler->flags;
    handlebars_compiler_accept(compiler, node);

    // Reset stacks
    compiler->bps = NULL;
    compiler->sns = NULL;

done:
    e->jmp = prev;
}

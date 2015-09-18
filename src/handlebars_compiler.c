
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_ast_helpers.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"



#define __MK(type) handlebars_opcode_type_ ## type
#define __PUSH(opcode) handlebars_compiler_opcode(compiler, opcode)

#define __OPB(type, arg) __PUSH(handlebars_opcode_ctor_boolean(compiler, __MK(type), arg))
#define __OPL(type, arg) __PUSH(handlebars_opcode_ctor_long(compiler, __MK(type), arg))
#define __OPN(type) __PUSH(handlebars_opcode_ctor(compiler, __MK(type)))
#define __OPS(type, arg) __PUSH(handlebars_opcode_ctor_string(compiler, __MK(type), arg))

#define __OPLS(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_long_string(compiler, __MK(type), arg1, arg2))
#define __OPS2(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_string2(compiler, __MK(type), arg1, arg2))
#define __OPSL(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_string_long(compiler, __MK(type), arg1, arg2))

#define __S1(x) #x
#define __S2(x) __S1(x)
#define __MEMCHECK(cond) \
    do { \
        if( unlikely(!cond) ) { \
            compiler->errnum = handlebars_compiler_error_nomem; \
            compiler->error = "Out of memory [" __S2(__FILE__) ":" __S2(__LINE__) "]"; \
            return; \
        } \
    } while(0)



static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node);
static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr);
static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid);
static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr);
static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid);
static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node);
static inline short handlebars_compiler_block_param_index(
        struct handlebars_compiler * compiler, const char * name, 
        int * depth, int * param);



/*static*/ const char * handlebars_builtins[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};



struct handlebars_compiler * handlebars_compiler_ctor(void * ctx)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_talloc_zero(ctx, struct handlebars_compiler);
    
    if( likely(compiler != NULL) ) {
        compiler->known_helpers = handlebars_builtins;
    }
    
    return compiler;
};

void handlebars_compiler_dtor(struct handlebars_compiler * compiler)
{
    assert(compiler != NULL);

    handlebars_talloc_free(compiler);
};

int handlebars_compiler_get_flags(struct handlebars_compiler * compiler)
{
    assert(compiler != NULL);

    return compiler->flags;
}

void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, int flags)
{
    assert(compiler != NULL);

    // Only allow option flags to be changed
    flags = flags & handlebars_compiler_flag_all;
    compiler->flags = compiler->flags & ~handlebars_compiler_flag_all;
    compiler->flags = compiler->flags | flags;
    
    // Update shortcuts
    compiler->string_params = 1 && (compiler->flags & handlebars_compiler_flag_string_params);
    compiler->track_ids = 1 && (compiler->flags & handlebars_compiler_flag_track_ids);
    compiler->use_depths = 1 && (compiler->flags & handlebars_compiler_flag_use_depths);
    compiler->no_escape = 1 && (compiler->flags & handlebars_compiler_flag_no_escape);
    compiler->known_helpers_only = 1 && (compiler->flags & handlebars_compiler_flag_known_helpers_only);
    compiler->prevent_indent = 1 && (compiler->flags & handlebars_compiler_flag_prevent_indent);
    compiler->use_data = 1 && (compiler->flags & handlebars_compiler_flag_use_data);
    compiler->explicit_partial_context = 1 && (compiler->flags & handlebars_compiler_flag_explicit_partial_context);
}



// Utilities

static inline void handlebars_compiler_add_depth(
        struct handlebars_compiler * compiler, int depth)
{
    assert(compiler != NULL);
    
    if( depth > 0 ) {
        compiler->result_flags |= handlebars_compiler_flag_use_depths;
    }
}

static inline short handlebars_compiler_is_known_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * path)
{
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    const char * helper_name;
    const char ** ptr;
    
    assert(compiler != NULL);

    if( NULL == path ||
        path->type != HANDLEBARS_AST_NODE_PATH ||
        NULL == (parts = path->node.path.parts) || 
        NULL == parts->first || 
        NULL == (path_segment = parts->first->data) ||
        NULL == (helper_name = path_segment->node.path_segment.part) ) {
        return 0;
    }
    
    for( ptr = compiler->known_helpers ; *ptr ; ++ptr ) {
        if( strcmp(helper_name, *ptr) == 0 ) {
            return 1;
        }
    }
    
    return 0;
}

static inline enum handlebars_compiler_sexpr_type handlebars_compiler_classify_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    struct handlebars_ast_node * path;
    short is_simple, is_block_param, is_helper, is_eligible;
    const char * part;
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
        } else if( compiler->known_helpers_only ) {
            is_eligible = 0;
        }
    }
    
    if( is_helper ) {
        return handlebars_compiler_sexpr_type_helper;
    } else if( is_eligible ) {
        return handlebars_compiler_sexpr_type_ambiguous;
    } else {
        return handlebars_compiler_sexpr_type_simple;
    }
}

static inline long handlebars_compiler_compile_program(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * program)
{
    struct handlebars_compiler * subcompiler;
    long guid;

    assert(compiler != NULL);
    assert(program != NULL);
    assert(program->type == HANDLEBARS_AST_NODE_PROGRAM ||
           program->type == HANDLEBARS_AST_NODE_CONTENT);
    
    subcompiler = handlebars_compiler_ctor(compiler);
    if( unlikely(subcompiler == NULL) ) {
        compiler->errnum = handlebars_compiler_error_nomem;
        return -1;
    }
    
    // copy compiler flags, bps, and options
    handlebars_compiler_set_flags(subcompiler, handlebars_compiler_get_flags(compiler));
    subcompiler->bps = compiler->bps;
    subcompiler->known_helpers = compiler->known_helpers;
    
    // compile
    handlebars_compiler_compile(subcompiler, program);
    guid = compiler->guid++;
    
    // copy compiler error
    if( (subcompiler->errnum != 0 && compiler->errnum == 0) ) {
        compiler->errnum = subcompiler->errnum;
        compiler->error = subcompiler->error;
    }

    // Don't propogate use_decorators
    compiler->result_flags |= (subcompiler->result_flags & ~handlebars_compiler_flag_use_decorators);
    
    // Realloc children array
    if( compiler->children_size <= compiler->children_length ) {
        compiler->children_size += 2;
        compiler->children = (struct handlebars_compiler **)
            handlebars_talloc_realloc(compiler, compiler->children, 
                    struct handlebars_compiler *, compiler->children_size);
        if( unlikely(compiler->children == NULL) ) {
            compiler->errnum = handlebars_compiler_error_nomem;
            return -1;
        }
    }
    
    // Append child
    compiler->children[compiler->children_length++] = subcompiler;
    
    return guid;
}

static inline void handlebars_compiler_opcode(
        struct handlebars_compiler * compiler, struct handlebars_opcode * opcode)
{
    assert(compiler != NULL);

    // Realloc opcode array
    if( compiler->opcodes_size <= compiler->opcodes_length ) {
        compiler->opcodes = handlebars_talloc_realloc(compiler, compiler->opcodes,
                    struct handlebars_opcode *, compiler->opcodes_size + 32);
        __MEMCHECK(compiler->opcodes);
        compiler->opcodes_size += 32;
    }
    
    // Get location from source node stack
    if( compiler->sns.i > 0 ) {
        opcode->loc = compiler->sns.s[compiler->sns.i - 1]->loc;
    }
    
    // Append opcode
    compiler->opcodes[compiler->opcodes_length++] = opcode;
}

static inline void handlebars_compiler_push_param(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * param)
{
    const char * strval;
    
    assert(compiler != NULL);
    assert(param != NULL);
    if( !param ) {
        return;
    }
    
    if( compiler->string_params ) {
        int depth = 0;
        struct handlebars_opcode * opcode;
        char * tmp;
        
        if( param->type == HANDLEBARS_AST_NODE_PATH ) {
            depth = param->node.path.depth;
        }
        
        if( depth ) {
            handlebars_compiler_add_depth(compiler, depth);
        }
        __OPL(get_context, depth);
        
        strval = handlebars_ast_node_get_string_mode_value(param);
        
        // sigh
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_string_param);
        __MEMCHECK(opcode);

        if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
            handlebars_operand_set_boolval(&opcode->op1, strcmp(strval, "true") == 0);
        } else {
            tmp = handlebars_talloc_strdup(compiler, strval);
            // @todo should be /^(\.\.?\/)/
            tmp = handlebars_ltrim(tmp, "./");
            for( char * tmp2 = tmp; *tmp2; tmp2++ ) {
                if( *tmp2 == '/' ) {
                    *tmp2 = '.';
                }
            }
            handlebars_operand_set_stringval(opcode, &opcode->op1, tmp);
        }
        handlebars_operand_set_stringval(opcode, &opcode->op2, handlebars_ast_node_readable_type(param->type));
        __PUSH(opcode);
        /*
        __OPS2(push_string_param, 
                handlebars_ast_node_get_string_mode_value(param), 
                handlebars_ast_node_readable_type(param->type));
        */
        
        if( param->type == HANDLEBARS_AST_NODE_SEXPR ) {
            handlebars_compiler_accept/*_sexpr*/(compiler, param);
        }
    } else {
        if( compiler->track_ids ) {
            const char * part;
            struct handlebars_opcode * opcode;
            int block_param_depth = -1;
            int block_param_num = -1;
            char tmp[32];
            const char * block_param_arr[3];
            const char ** parts_arr;
            char * block_param_parts;
            
            opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_id);
            __MEMCHECK(opcode);
            
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
                
                handlebars_operand_set_stringval(opcode, &opcode->op1, "BlockParam");
                handlebars_operand_set_arrayval(opcode, &opcode->op2, block_param_arr);
                parts_arr = handlebars_ast_node_get_id_parts(opcode, param);
                __MEMCHECK(parts_arr);
                if( *parts_arr ) {
                    block_param_parts = handlebars_implode(".", parts_arr + 1);
                    __MEMCHECK(block_param_parts);
                    handlebars_operand_set_stringval(opcode, &opcode->op3, block_param_parts);
                    handlebars_talloc_free(block_param_parts);
                } else {
                    handlebars_operand_set_stringval(opcode, &opcode->op3, "");
                }
                handlebars_talloc_free(parts_arr);
                __PUSH(opcode);
            } else {
                strval = handlebars_ast_node_get_string_mode_value(param);
                if( strval == strstr(strval, "this") ) {
                	strval += 4 + (*(strval + 4) == '.' || *(strval + 4) == '$' ? 1 : 0 );
                }
                if( *strval == '.' && *(strval + 1) == 0 ) {
                    strval = "";
                } else if( *strval == '.' && *(strval + 1) == '/' ) {
                    strval += 2;
                }
                handlebars_operand_set_stringval(opcode, &opcode->op1, handlebars_ast_node_readable_type(param->type));
                // @todo need to remove leading .
                if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
                    handlebars_operand_set_boolval(&opcode->op2, strcmp(strval, "true") == 0);
                } else if( param->type == HANDLEBARS_AST_NODE_NUMBER ) {
                    long lval;
                    sscanf(strval, "%10ld", &lval);
                    handlebars_operand_set_longval(&opcode->op2, lval);
                } else if( param->type == HANDLEBARS_AST_NODE_STRING ) {
                    handlebars_operand_set_stringval(opcode, &opcode->op2, strval ? strval : "");
                } else {
                    handlebars_operand_set_stringval(opcode, &opcode->op2, strval);
                }
                __PUSH(opcode);
            }
        }
        handlebars_compiler_accept(compiler, param);
    }
}

static inline void handlebars_compiler_push_params(
        struct handlebars_compiler * compiler, struct handlebars_ast_list * params)
{
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
        struct handlebars_ast_node * sexpr, long programGuid, long inverseGuid, short omit_empty)
{
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * hash;

    assert(compiler != NULL);
    assert(sexpr != NULL);
    
    params = handlebars_ast_node_get_params(sexpr);
    hash = handlebars_ast_node_get_hash(sexpr);
    
    handlebars_compiler_push_params(compiler, params);
    
    programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
    inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
    
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
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
	struct handlebars_ast_node * path = handlebars_ast_node_get_path(node);
	struct handlebars_ast_node * part;
	struct handlebars_ast_list * parts;
	const char * val;
	switch( path->type ) {
		case HANDLEBARS_AST_NODE_UNDEFINED:
		case HANDLEBARS_AST_NODE_NUL:
		case HANDLEBARS_AST_NODE_BOOLEAN:
		case HANDLEBARS_AST_NODE_NUMBER:
		case HANDLEBARS_AST_NODE_STRING:
			val = handlebars_ast_node_get_string_mode_value(path);
			// Make parts
			part = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PATH_SEGMENT, node);
			part->node.path_segment.part = handlebars_talloc_strdup(part, val);
			__MEMCHECK(part);
			parts = handlebars_ast_list_ctor(node);
			__MEMCHECK(parts);
		    handlebars_ast_list_append(parts, part);
		    // Re-jigger node
		    memset(path, 0, sizeof(struct handlebars_ast_node));
			path->type = HANDLEBARS_AST_NODE_PATH;
			path->node.path.original = handlebars_talloc_strdup(path, val);
			__MEMCHECK(path->node.path.original);
			path->node.path.parts = talloc_steal(path, parts);
			break;
	}
}

static inline short handlebars_compiler_block_param_index(
        struct handlebars_compiler * compiler, const char * name, 
        int * depth, int * param)
{
    int i;
    int l = compiler->bps->i;
    char * block_param1;
    char * block_param2;
    *param = -1;
    
    if( !name ) { // @todo double-check
    	return 0;
    }

    for( i = 0; i < l; i++ ) {
        block_param1 = compiler->bps->s[l - i - 1].block_param1;
        block_param2 = compiler->bps->s[l - i - 1].block_param2;
        if( block_param1 && 0 == strcmp(block_param1, name) ) {
            *param = 0;
            *depth = i;
            return 1;
        } else if( block_param2 && 0 == strcmp(block_param2, name) ) {
            *param = 1;
            *depth = i;
            return 1;
        }
    }
    
    return 0;
}



// Acceptors

static inline void handlebars_compiler_accept_program(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    struct handlebars_ast_list * list = node->node.program.statements;
    char * block_param1 = node->node.program.block_param1;
    char * block_param2 = node->node.program.block_param2;
    size_t statement_count = 0;

    assert(compiler != NULL);
    
    if( compiler->bps->i > HANDLEBARS_COMPILER_STACK_SIZE ) {
        compiler->errnum = handlebars_compiler_error_block_param_stack_blown;
        compiler->error = handlebars_talloc_strdup(compiler, "Block param stack blown");
        return;
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
        compiler->result_flags |= handlebars_compiler_flag_is_simple;
    }
    if( block_param1 ) {
        compiler->block_params++;
    }
    if( block_param2 ) {
        compiler->block_params++;
    }
    // @todo fix
    // this.blockParams = program.blockParams ? program.blockParams.length : 0;
}

static inline void handlebars_compiler_accept_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * block)
{
    struct handlebars_ast_node * sexpr = block; // cough
    struct handlebars_ast_node * program = block->node.block.program;
    struct handlebars_ast_node * inverse = block->node.block.inverse;
    struct handlebars_ast_node * path = block->node.block.path;
    long programGuid = -1;
    long inverseGuid = -1;
    struct handlebars_ast_node * params;
    const char * original;

    assert(compiler != NULL);

    handlebars_compiler_transform_literal_to_path(compiler, block);

    if( program != NULL ) {
        programGuid = handlebars_compiler_compile_program(compiler, program);
    }
    
    if( block->node.block.is_decorator ) {
        // Decorator
    	original = handlebars_ast_node_get_string_mode_value(path);
    	params = handlebars_compiler_setup_full_mustache_params(
                    compiler, block, programGuid, inverseGuid, 0);
        compiler->result_flags |= handlebars_compiler_flag_use_decorators;
        __OPLS(register_decorator, handlebars_ast_list_count(params), original);
    } else {
		// Normal
		if( inverse != NULL ) {
			inverseGuid = handlebars_compiler_compile_program(compiler, inverse);
		}

		switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
			case handlebars_compiler_sexpr_type_helper:
				handlebars_compiler_accept_sexpr_helper(compiler, sexpr, programGuid, inverseGuid);
				break;
			case handlebars_compiler_sexpr_type_simple:
				handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
				programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
				inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
				__OPN(empty_hash);
				__OPS(block_value, path->node.path.original);
				break;
			case handlebars_compiler_sexpr_type_ambiguous:
				handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, programGuid, inverseGuid);
				programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
				inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
				__OPN(empty_hash);
				__OPN(ambiguous_block_value);
				break;
		}

		__OPN(append);
    }
}

static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * hash)
{
    struct handlebars_ast_list * params = hash->node.hash.pairs;
    int len = handlebars_ast_list_count(params);
    int i = 0;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    char ** keys;

    assert(compiler != NULL);
    
    keys = handlebars_talloc_array(compiler, char *, len);
    if( unlikely(keys == NULL) ) {
        compiler->errnum = handlebars_compiler_error_nomem;
        return;
    }
    
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
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node,
        struct handlebars_ast_node * name, struct handlebars_ast_node * params,
        struct handlebars_ast_node * program, char * indent)
{
    int count = 0;
    short is_dynamic = 0;
    long programGuid = -1;

    assert(compiler != NULL);
    assert(node != NULL);
    assert(name != NULL);

    compiler->result_flags |= handlebars_compiler_flag_use_partial;

    count = (params ? handlebars_ast_list_count(params) : 0);
    
    if( count > 1 ) {
    	compiler->errnum = handlebars_compiler_error_unsupported_partial_args;
    	return;
    } else if( !params || !handlebars_ast_list_count(params) ) {
    	if( compiler->flags & handlebars_compiler_flag_explicit_partial_context ) {
    		__OPS(push_literal, "undefined");
    	} else {
			struct handlebars_ast_node * tmp;
			if( !params ) {
				params = handlebars_ast_list_ctor(node);
				__MEMCHECK(params);
				if( node->type == HANDLEBARS_AST_NODE_PARTIAL ) {
					node->node.partial.params = params;
				} else if( node->type == HANDLEBARS_AST_NODE_PARTIAL_BLOCK ) {
					node->node.partial_block.params = params;
				}
			}
			__MEMCHECK(params);
			tmp = handlebars_ast_node_ctor(HANDLEBARS_AST_NODE_PATH, params);
			__MEMCHECK(tmp);
			handlebars_ast_list_append(params, tmp);
    	}
    }
    
    if( name->type == HANDLEBARS_AST_NODE_SEXPR ) {
    	is_dynamic = 1;
    }

    if( is_dynamic ) {
    	handlebars_compiler_accept(compiler, name);
    }

    if( program != NULL ) {
        programGuid = handlebars_compiler_compile_program(compiler, program);
    }

    handlebars_compiler_setup_full_mustache_params(compiler, node, programGuid, -1, 1);

    if( compiler->prevent_indent && indent && *indent ) {
        __OPS(append_content, indent);
        indent = "";
    }

    do {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_partial);
        __MEMCHECK(opcode);
        handlebars_operand_set_boolval(&opcode->op1, is_dynamic);
        if( !is_dynamic ) {
            const char * strval = handlebars_ast_node_get_string_mode_value(name);
            long lv = 0;
        	double fv;
            if( name->type == HANDLEBARS_AST_NODE_NUMBER ) {
                fv = strtod(strval, NULL);
                lv = strtol(strval, NULL, 10);
                if( !fv || !lv || fv != (double) lv ) {
                    lv = 0;
                }
            }
            if( lv ) {
                handlebars_operand_set_longval(&opcode->op2, lv);
            } else {
                handlebars_operand_set_stringval(opcode, &opcode->op2, strval);
            }
        }
        handlebars_operand_set_stringval(opcode, &opcode->op3, indent ? indent : "");
        __PUSH(opcode);
    } while(0);

    __OPN(append);
}

static inline void handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * partial)
{
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    char * indent;

    assert(compiler != NULL);
    assert(partial != NULL);
    assert(partial->type == HANDLEBARS_AST_NODE_PARTIAL);

    name = partial->node.partial.name;
    params = partial->node.partial.params;
    indent = partial->node.partial.indent;

    _handlebars_compiler_accept_partial(compiler, partial, name, params, NULL, indent);
}

static inline void handlebars_compiler_accept_partial_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * partial_block)
{
    struct handlebars_ast_node * name;
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * program;
    char * indent;

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
        struct handlebars_compiler * compiler, struct handlebars_ast_node * content)
{
    assert(compiler != NULL);
    assert(content != NULL);
    assert(content->type == HANDLEBARS_AST_NODE_CONTENT);
    
    if( likely(/* content && */ content->node.content.value && *content->node.content.value) ) {
        __OPS(append_content, content->node.content.value);
    }
}

static inline void handlebars_compiler_accept_mustache(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * mustache)
{
    assert(compiler != NULL);
    assert(mustache != NULL);
    assert(mustache->type == HANDLEBARS_AST_NODE_MUSTACHE);
    
    
    if( mustache->node.mustache.is_decorator ) {
        // Decorator
        struct handlebars_ast_node * path = mustache->node.mustache.path;
        struct handlebars_ast_node * params;
        const char * original;
    	original = handlebars_ast_node_get_string_mode_value(path);
    	params = handlebars_compiler_setup_full_mustache_params(
                    compiler, mustache, -1, -1, 0);
        compiler->result_flags |= handlebars_compiler_flag_use_decorators;
        __OPLS(register_decorator, handlebars_ast_list_count(params), original);
    } else {
    	// Normal
        handlebars_compiler_accept_sexpr(compiler, mustache);

        if( !mustache->node.mustache.unescaped && !compiler->no_escape ) {
            __OPN(append_escaped);
        } else {
            __OPN(append);
        }
    }
}

static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    struct handlebars_ast_node * path;
    const char * name;
    int is_block = (programGuid >= 0 || inverseGuid >= 0);

    assert(compiler != NULL);
    assert(sexpr != NULL);
    
    path = handlebars_ast_node_get_path(sexpr);
    
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    name = handlebars_ast_node_get_id_part(path);
    
    __OPL(get_context, path->node.path.depth);
    programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
    inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);

	path->node.path.strict = 1;
    handlebars_compiler_accept/*_id*/(compiler, path);
    
    do {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_ambiguous);
        __MEMCHECK(opcode);
        handlebars_operand_set_stringval(opcode, &opcode->op1, name);
        handlebars_operand_set_boolval(&opcode->op2, is_block);
        __PUSH(opcode);
        //__OPSL(invoke_ambiguous, name, is_block);
    } while(0);
}

static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    struct handlebars_ast_node * path;

    assert(compiler != NULL);
    assert(sexpr != NULL);
    
    path = handlebars_ast_node_get_path(sexpr);
    
    if( path ) {
    	path->node.path.strict = 1;
        handlebars_compiler_accept/*_data*/(compiler, path);
    }
    
    __OPN(resolve_possible_lambda);
}

static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    struct handlebars_ast_node * path;
    struct handlebars_ast_list * params;
    const char * name;

    assert(compiler != NULL);
    assert(sexpr != NULL);
    
    path = handlebars_ast_node_get_path(sexpr);
    
    params = handlebars_compiler_setup_full_mustache_params(
                compiler, sexpr, programGuid, inverseGuid, 0);
    
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    name = handlebars_ast_node_get_id_part(path);
    
    if( handlebars_compiler_is_known_helper(compiler, path) ) {
        __OPLS(invoke_known_helper, handlebars_ast_list_count(params), name);
    } else if( compiler->known_helpers_only ) {
        compiler->errnum = handlebars_compiler_error_unknown_helper;
        compiler->error = handlebars_talloc_asprintf(compiler, 
                "You specified knownHelpersOnly, but used the unknown helper %s", name);
        __MEMCHECK(compiler->error);
    } else {
        struct handlebars_opcode * opcode;
        int is_simple = handlebars_ast_helper_simple_id(path);
        
        path->node.path.falsy = 1;
    	path->node.path.strict = 1;
        handlebars_compiler_accept/*_id*/(compiler, path);
        
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_helper);
        __MEMCHECK(opcode);
        handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
        //handlebars_operand_set_stringval(compiler, &opcode->op2, name);
        handlebars_operand_set_stringval(compiler, &opcode->op2, path->node.path.original);
        handlebars_operand_set_boolval(&opcode->op3, is_simple);
        handlebars_compiler_opcode(compiler, opcode);
    }
}

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    assert(compiler != NULL);
    assert(sexpr != NULL);

    handlebars_compiler_transform_literal_to_path(compiler, sexpr);

    switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
        case handlebars_compiler_sexpr_type_helper:
            handlebars_compiler_accept_sexpr_helper(compiler, sexpr, -1, -1);
            break;
        case handlebars_compiler_sexpr_type_simple:
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
            break;
        case handlebars_compiler_sexpr_type_ambiguous:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, -1, -1);
            break;
        default:
        	assert(0);
    }
}

static inline void handlebars_compiler_accept_path(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * path)
{
    const char * name;
    short is_scoped = 0;
    struct handlebars_opcode * opcode;
    char ** parts_arr;
    int block_param_depth = -1;
    int block_param_num = -1;

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
        
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_block_param);
        __MEMCHECK(opcode);
        handlebars_operand_set_arrayval(opcode, &opcode->op1, block_param_arr);
        parts_arr = handlebars_ast_node_get_id_parts(opcode, path);
        __MEMCHECK(parts_arr);
        opcode->op2.type = handlebars_operand_type_array;
        opcode->op2.data.arrayval = parts_arr;
        __PUSH(opcode);
    } else if( name == NULL ) {
        __OPN(push_context);
    } else if( path->node.path.data ) {
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_data);
        __MEMCHECK(opcode);
        parts_arr = handlebars_ast_node_get_id_parts(opcode, path);
        __MEMCHECK(parts_arr);

        handlebars_operand_set_longval(&opcode->op1, path->node.path.depth);
        opcode->op2.type = handlebars_operand_type_array;
        opcode->op2.data.arrayval = parts_arr;
        if( path->node.path.strict ) {
        	handlebars_operand_set_boolval(&opcode->op3, path->node.path.strict);
        }

        __PUSH(opcode);
    } else {
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_on_context);
        __MEMCHECK(opcode);
        parts_arr = handlebars_ast_node_get_id_parts(opcode, path);
        __MEMCHECK(parts_arr);
        opcode->op1.type = handlebars_operand_type_array;
        opcode->op1.data.arrayval = parts_arr;
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
        struct handlebars_compiler * compiler, struct handlebars_ast_node * string)
{
    assert(string != NULL);
    assert(string->type == HANDLEBARS_AST_NODE_STRING);
    
    __OPS(push_string, string->node.string.value);
}

static inline void handlebars_compiler_accept_number(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * number)
{
	double fv;
	long lv;
	char * val = number->node.number.value;

    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_NUMBER);

    if( 0 == strcmp(val, "0") ) {
    	__OPL(push_literal, 0);
        return;
    }
    
    // Convert to float and long
    fv = strtod(val, NULL);
    lv = strtol(val, NULL, 10);

    if( fv && lv && fv == (double) lv ) {
    	// It's a long - @todo do we need to do float too?
    	__OPL(push_literal, lv);
    } else {
        __OPS(push_literal, number->node.number.value);
    }
}

static inline void handlebars_compiler_accept_boolean(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * boolean)
{
    struct handlebars_opcode * opcode;

    assert(boolean != NULL);
    assert(boolean->type == HANDLEBARS_AST_NODE_BOOLEAN);

    opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_literal);
    __MEMCHECK(opcode);
    handlebars_operand_set_boolval(&opcode->op1, strcmp(boolean->node.boolean.value, "false") != 0);
    __PUSH(opcode);
}


static inline void handlebars_compiler_accept_nul(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * number)
{
    struct handlebars_opcode * opcode;

    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_NUL);

    __OPS(push_literal, "null");
}

static inline void handlebars_compiler_accept_undefined(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * number)
{
    struct handlebars_opcode * opcode;

    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_UNDEFINED);

    __OPS(push_literal, "undefined");
}

static inline void handlebars_compiler_accept_comment(
        HANDLEBARS_ATTR_UNUSED struct handlebars_compiler * compiler,
        HANDLEBARS_ATTR_UNUSED struct handlebars_ast_node * comment)
{
    assert(comment != NULL);
    assert(comment->type == HANDLEBARS_AST_NODE_COMMENT);
}

static inline void handlebars_compiler_accept_raw_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * raw_block)
{
    struct handlebars_ast_node * sexpr = raw_block; // cough
    struct handlebars_ast_node * program = raw_block->node.raw_block.program;
    struct handlebars_ast_node * inverse = raw_block->node.raw_block.inverse;
    struct handlebars_ast_node * path = raw_block->node.raw_block.path;
    long programGuid = -1;
    long inverseGuid = -1;

    assert(compiler != NULL);
    
    handlebars_compiler_transform_literal_to_path(compiler, raw_block);

    if( program != NULL ) {
        programGuid = handlebars_compiler_compile_program(compiler, program);
    }
    
    if( inverse != NULL ) {
        inverseGuid = handlebars_compiler_compile_program(compiler, inverse);
    }
    
    switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
        case handlebars_compiler_sexpr_type_helper:
            handlebars_compiler_accept_sexpr_helper(compiler, sexpr, programGuid, inverseGuid);
            break;
        case handlebars_compiler_sexpr_type_simple:
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
            programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
            inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
            __OPN(empty_hash);
            __OPS(block_value, path->node.path.original);
            break;
        case handlebars_compiler_sexpr_type_ambiguous:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, programGuid, inverseGuid);
            programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
            inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
            __OPN(empty_hash);
            __OPN(ambiguous_block_value);
            break;
    }
    
    __OPN(append);
}

static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    if( !node || compiler->errnum ) {
        return;
    }
    
    // Add node to source node stack
    if( compiler->sns.i > HANDLEBARS_COMPILER_STACK_SIZE ) {
        compiler->errnum = handlebars_compiler_error_source_node_stack_blown;
        compiler->error = handlebars_talloc_strdup(compiler, "Source node stack blown");
        return;
    }
    compiler->sns.s[compiler->sns.i] = node;
    compiler->sns.i++;
    
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
    compiler->sns.i--;
}

void handlebars_compiler_compile(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    if( NULL == compiler->bps ) {
        compiler->bps = handlebars_talloc_zero(compiler, struct handlebars_block_param_stack);
    }
    handlebars_compiler_accept(compiler, node);
}

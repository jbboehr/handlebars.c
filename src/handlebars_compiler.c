
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"



#define __MK(type) handlebars_opcode_type_ ## type
#define __PUSH(opcode) handlebars_compiler_opcode(compiler, opcode)

#define __OPL(type, arg) __PUSH(handlebars_opcode_ctor_long(compiler, __MK(type), arg))
#define __OPN(type) __PUSH(handlebars_opcode_ctor(compiler, __MK(type)))
#define __OPS(type, arg) __PUSH(handlebars_opcode_ctor_string(compiler, __MK(type), arg))



static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node);
static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr);
static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid);
static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid);
static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid);



static const char * handlebars_builtins[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};



struct handlebars_compiler * handlebars_compiler_ctor(void)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_talloc_zero(NULL, struct handlebars_compiler);
    
    if( compiler ) {
        compiler->known_helpers = handlebars_builtins;
    }
    
    return compiler;
};

void handlebars_compiler_dtor(struct handlebars_compiler * compiler)
{
    handlebars_talloc_free(compiler);
};

int handlebars_compiler_get_flags(struct handlebars_compiler * compiler)
{
    return compiler->flags;
}

void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, int flags)
{
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
}



// Utilities

static inline void handlebars_compiler_add_depth(
        struct handlebars_compiler * compiler, int depth)
{
    compiler->depths |= (1 << depth);
}

static inline short handlebars_compiler_is_known_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * id)
{
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;
    const char * helper_name;
    const char ** ptr;
    if( !id || !(parts = id->node.id.parts) || !parts->first || 
            !(path_segment = parts->first->data) ||
            !(helper_name = path_segment->node.path_segment.part) ) {
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
    short is_helper = sexpr->node.sexpr.is_helper;
    short is_eligible = sexpr->node.sexpr.eligible_helper;
    
    if( is_eligible && !is_helper ) {
        if( handlebars_compiler_is_known_helper(compiler, sexpr->node.sexpr.id) ) {
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
    return -1; // @todo
}

static inline void handlebars_compiler_opcode(
        struct handlebars_compiler * compiler, struct handlebars_opcode * opcode)
{
    // Realloc opcode array
    if( compiler->opcodes_size <= compiler->opcodes_length ) {
        compiler->opcodes_size += 32;
        compiler->opcodes = talloc_array(compiler, struct handlebars_opcode *, 
                    compiler->opcodes_size);
    }
    
    // Append opcode
    compiler->opcodes[compiler->opcodes_length++] = opcode;
}

/*
static inline void _handlebars_compiler_push_param(struct handlebars_compiler * compiler)
{
    ;
}
*/

static inline void handlebars_compiler_setup_full_mustache_params(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr, long program, long inverse)
{
    
}



// Acceptors

static inline void handlebars_compiler_accept_program(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    struct handlebars_ast_list * list = node->node.program.statements;
    
    handlebars_ast_list_foreach(list, item, tmp) {
        handlebars_compiler_accept(compiler, item->data);
    }
    
    // @todo sort depths?
}

static inline void handlebars_compiler_accept_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * block)
{
    struct handlebars_ast_node * mustache = block->node.block.mustache;
    struct handlebars_ast_node * program = block->node.block.program;
    struct handlebars_ast_node * inverse = block->node.block.inverse;
    struct handlebars_ast_node * sexpr = mustache->node.mustache.sexpr;
    struct handlebars_ast_node * id;
    long programGuid = -1;
    long inverseGuid = -1;
    
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
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr, programGuid, inverseGuid);
            __OPL(push_program, programGuid);
            __OPL(push_program, inverseGuid);
            __OPN(empty_hash);
            id = sexpr->node.sexpr.id;
            __OPS(block_value, id ? id->node.id.original : "");
            break;
        default:
        case handlebars_compiler_sexpr_type_ambiguous:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, programGuid, inverseGuid);
            __OPL(push_program, programGuid);
            __OPL(push_program, inverseGuid);
            __OPN(empty_hash);
            __OPN(ambiguous_block_value);
            break;
    }
    
    __OPN(append);
}

static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_content(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * content)
{
    if( content && content->node.content.string ) {
        __OPS(append_content, content->node.content.string);
    }
}

static inline void handlebars_compiler_accept_mustache(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * mustache)
{
    handlebars_compiler_accept_sexpr(compiler, mustache->node.mustache.sexpr);
    
    if( mustache->node.mustache.escaped && !compiler->no_escape ) {
        __OPN(append_escaped);
    } else {
        __OPN(append);
    }
}

static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    switch( handlebars_compiler_classify_sexpr(compiler, sexpr) ) {
        case handlebars_compiler_sexpr_type_helper:
            handlebars_compiler_accept_sexpr_helper(compiler, sexpr, -1, -1);
            break;
        case handlebars_compiler_sexpr_type_simple:
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr, -1, -1);
            break;
        default:
        case handlebars_compiler_sexpr_type_ambiguous:
            handlebars_compiler_accept_sexpr_ambiguous(compiler, sexpr, -1, -1);
            break;
    }
}

static inline void handlebars_compiler_accept_id(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_data(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    // @todo
    ;
}

static inline void handlebars_compiler_accept_string(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * string)
{
    __OPS(push_string, string->node.string.string);
}

static inline void handlebars_compiler_accept_number(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * number)
{
    __OPS(push_literal, number->node.number.string);
}

static inline void handlebars_compiler_accept_boolean(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * boolean)
{
    __OPS(push_literal, boolean->node.boolean.string);
}

static inline void handlebars_compiler_accept_comment(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    ;
}

/*
static inline void handlebars_compiler_accept_raw_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    ;
}

static inline void handlebars_compiler_accept_hash_segment(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    ;
}

static inline void handlebars_compiler_accept_partial_name(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    ;
}

static inline void handlebars_compiler_accept_path_segment(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    ;
}
*/


static void handlebars_compiler_accept(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_PROGRAM:
            return handlebars_compiler_accept_program(compiler, node);
        case HANDLEBARS_AST_NODE_MUSTACHE:
            return handlebars_compiler_accept_mustache(compiler, node);
        case HANDLEBARS_AST_NODE_SEXPR:
            return handlebars_compiler_accept_sexpr(compiler, node);
        case HANDLEBARS_AST_NODE_PARTIAL:
            return handlebars_compiler_accept_partial(compiler, node);
        case HANDLEBARS_AST_NODE_BLOCK:
            return handlebars_compiler_accept_block(compiler, node);
        case HANDLEBARS_AST_NODE_CONTENT:
            return handlebars_compiler_accept_content(compiler, node);
        case HANDLEBARS_AST_NODE_HASH:
            return handlebars_compiler_accept_hash(compiler, node);
        case HANDLEBARS_AST_NODE_ID:
            return handlebars_compiler_accept_id(compiler, node);
        case HANDLEBARS_AST_NODE_DATA:
            return handlebars_compiler_accept_data(compiler, node);
        case HANDLEBARS_AST_NODE_STRING:
            return handlebars_compiler_accept_string(compiler, node);
        case HANDLEBARS_AST_NODE_NUMBER:
            return handlebars_compiler_accept_number(compiler, node);
        case HANDLEBARS_AST_NODE_BOOLEAN:
            return handlebars_compiler_accept_boolean(compiler, node);
        case HANDLEBARS_AST_NODE_COMMENT:
            return handlebars_compiler_accept_comment(compiler, node);
        
        case HANDLEBARS_AST_NODE_NIL:
        default:
            // error?
            break;
        
        /*
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return handlebars_compiler_accept_raw_block(compiler, node);
        case HANDLEBARS_AST_NODE_HASH_SEGMENT:
            return handlebars_compiler_accept_hash_segment(compiler, node);
        case HANDLEBARS_AST_NODE_PARTIAL_NAME:
            return handlebars_compiler_accept_partial_name(compiler, node);
        case HANDLEBARS_AST_NODE_PATH_SEGMENT:
            return handlebars_compiler_accept_path_segment(compiler, node);
        */
    }
}

void handlebars_compiler_compile(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    handlebars_compiler_accept(compiler, node);
}

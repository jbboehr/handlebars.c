
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_private.h"




#define __MK(type) handlebars_opcode_type_ ## type
#define __PUSH(opcode) handlebars_compiler_opcode(compiler, opcode)

#define __OPL(type, arg) __PUSH(handlebars_opcode_ctor_long(compiler, __MK(type), arg))
#define __OPN(type) __PUSH(handlebars_opcode_ctor(compiler, __MK(type)))
#define __OPS(type, arg) __PUSH(handlebars_opcode_ctor_string(compiler, __MK(type), arg))

#define __OPLS(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_long_string(compiler, __MK(type), arg1, arg2))
#define __OPS2(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_string2(compiler, __MK(type), arg1, arg2))
#define __OPSL(type, arg1, arg2) __PUSH(handlebars_opcode_ctor_string_long(compiler, __MK(type), arg1, arg2))



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



/*static*/ const char * handlebars_builtins[] = {
    "helperMissing", "blockHelperMissing", "each", "if",
    "unless", "with", "log", "lookup", NULL
};



struct handlebars_compiler * handlebars_compiler_ctor(void * ctx)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_talloc_zero(ctx, struct handlebars_compiler);
    
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
    
    if( NULL == id ||
        id->type != HANDLEBARS_AST_NODE_ID ||
        NULL == (parts = id->node.id.parts) || 
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
    short is_helper = sexpr->node.sexpr.is_helper;
    short is_eligible = sexpr->node.sexpr.eligible_helper;
    struct handlebars_ast_node * id = sexpr->node.sexpr.id;
    
    //assert(id != NULL);
    //assert(id->type == HANDLEBARS_AST_NODE_ID);
    //assert(id == NULL || id->type == HANDLEBARS_AST_NODE_ID);
    
    if( is_eligible && !is_helper ) {
        if( handlebars_compiler_is_known_helper(compiler, id) ) {
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
    unsigned long depths;
    int depthi;
    
    assert(program != NULL);
    assert(program->type == HANDLEBARS_AST_NODE_PROGRAM ||
           program->type == HANDLEBARS_AST_NODE_CONTENT);
    
    subcompiler = handlebars_compiler_ctor(compiler);
    if( subcompiler == NULL ) {
        compiler->errnum = handlebars_compiler_error_nomem;
        return -1;
    }
    
    // copy compiler flags
    handlebars_compiler_set_flags(subcompiler, handlebars_compiler_get_flags(compiler));
    
    // compile
    handlebars_compiler_compile(subcompiler, program);
    guid = compiler->guid++;
    
    compiler->use_partial |= subcompiler->use_partial;
    
    // Realloc children array
    if( compiler->children_size <= compiler->children_length ) {
        compiler->children_size += 2;
        compiler->children = (struct handlebars_compiler **)
            handlebars_talloc_realloc(compiler, compiler->children, 
                    struct handlebars_compiler *, compiler->children_size);
    }
    
    // Append child
    compiler->children[compiler->children_length++] = subcompiler;
    
    // Add depths
    depths = subcompiler->depths;
    depthi = 1;
    while( depths > 0 ) {
        if( (depths & 1) && depthi >= 2 ) {
            handlebars_compiler_add_depth(compiler, depthi - 1);
        }
        depthi++;
        depths = depths >> 1;
    }
    
    return guid;
}

static inline void handlebars_compiler_opcode(
        struct handlebars_compiler * compiler, struct handlebars_opcode * opcode)
{
    // Realloc opcode array
    if( compiler->opcodes_size <= compiler->opcodes_length ) {
        compiler->opcodes_size += 32;
        compiler->opcodes = handlebars_talloc_realloc(compiler, compiler->opcodes,
                    struct handlebars_opcode *, compiler->opcodes_size);
    }
    
    // Append opcode
    compiler->opcodes[compiler->opcodes_length++] = opcode;
}

static inline void handlebars_compiler_push_param(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * param)
{
    assert(param != NULL);
    if( !param ) {
        return;
    }
    
    if( compiler->string_params ) {
        int depth = 0;
        struct handlebars_opcode * opcode;
        
        if( param->type == HANDLEBARS_AST_NODE_ID ) {
            depth = param->node.id.depth;
        }
        
        if( depth ) {
            handlebars_compiler_add_depth(compiler, depth);
        }
        __OPL(get_context, depth);
        
        // sigh
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_string_param);
        if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
            handlebars_operand_set_boolval(&opcode->op1, strcmp(handlebars_ast_node_get_string_mode_value(param), "true") == 0);
        } else {
            handlebars_operand_set_stringval(opcode, &opcode->op1, handlebars_ast_node_get_string_mode_value(param));
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
            const char * tmp;
            struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_id);
            handlebars_operand_set_stringval(opcode, &opcode->op1, handlebars_ast_node_readable_type(param->type));
            tmp = handlebars_ast_node_get_id_name(param);
            if( !tmp ) {
                tmp = handlebars_ast_node_get_string_mode_value(param);
            }
            if( param->type == HANDLEBARS_AST_NODE_BOOLEAN ) {
                handlebars_operand_set_boolval(&opcode->op2, strcmp(tmp, "true") == 0);
            } else if( param->type == HANDLEBARS_AST_NODE_NUMBER ) {
                long tmp2;
                sscanf(tmp, "%10ld", &tmp2);
                handlebars_operand_set_longval(&opcode->op2, tmp2);
            } else if( param->type == HANDLEBARS_AST_NODE_STRING ) {
                handlebars_operand_set_stringval(opcode, &opcode->op2, tmp ? tmp : "");
            } else {
                handlebars_operand_set_stringval(opcode, &opcode->op2, tmp);
            }
            __PUSH(opcode);
            /*
            __OPS2(push_id, 
                    , 
                    tmp);
            */
        }
        handlebars_compiler_accept(compiler, param);
    }
}

static inline void handlebars_compiler_push_params(
        struct handlebars_compiler * compiler, struct handlebars_ast_list * params)
{
    if( params != NULL ) {
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;
        
        handlebars_ast_list_foreach(params, item, tmp) {
            handlebars_compiler_push_param(compiler, item->data);
        }
    }
}

static inline struct handlebars_ast_list * handlebars_compiler_setup_full_mustache_params(
        struct handlebars_compiler * compiler,
        struct handlebars_ast_node * sexpr, long programGuid, long inverseGuid)
{
    struct handlebars_ast_list * params = sexpr->node.sexpr.params;
    handlebars_compiler_push_params(compiler, params);
    
    programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
    inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
    
    // @todo make sure empty
    if( sexpr->node.sexpr.hash ) {
        handlebars_compiler_accept/*_hash*/(compiler, sexpr->node.sexpr.hash);
    } else {
        __OPN(empty_hash);
    }
    
    return params;
}



// Acceptors

static inline void handlebars_compiler_accept_program(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    struct handlebars_ast_list * list = node->node.program.statements;
    
    if( list != NULL ) {
        struct handlebars_ast_list_item * item;
        struct handlebars_ast_list_item * tmp;
        
        handlebars_ast_list_foreach(list, item, tmp) {
            handlebars_compiler_accept(compiler, item->data);
        }
    }
    
    // @todo sort depths?
}


static inline void handlebars_compiler_accept_block_internal(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * mustache,
        struct handlebars_ast_node * program, struct handlebars_ast_node * inverse)
{
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
            handlebars_compiler_accept_sexpr_simple(compiler, sexpr);
            programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
            inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
            __OPN(empty_hash);
            id = sexpr->node.sexpr.id;
            assert(!id || id->type == HANDLEBARS_AST_NODE_ID);
            __OPS(block_value, id ? id->node.id.original : "");
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

static inline void handlebars_compiler_accept_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * block)
{
    struct handlebars_ast_node * mustache = block->node.block.mustache;
    struct handlebars_ast_node * program = block->node.block.program;
    struct handlebars_ast_node * inverse = block->node.block.inverse;
    handlebars_compiler_accept_block_internal(compiler, mustache, program, inverse);
}

static inline void handlebars_compiler_accept_hash(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * hash)
{
    struct handlebars_ast_list * params = hash->node.hash.segments;
    int len = handlebars_ast_list_count(params);
    int i = 0;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    char ** keys;
    
    keys = handlebars_talloc_array(compiler, char *, len);
    if( !keys ) {
        compiler->errnum = handlebars_compiler_error_nomem;
        return;
    }
    
    __OPN(push_hash);
    
    handlebars_ast_list_foreach(params, item, tmp) {
        struct handlebars_ast_node * hash_segment = item->data;
        
        assert(hash_segment != NULL);
        assert(hash_segment->type == HANDLEBARS_AST_NODE_HASH_SEGMENT);
        
        if( /* hash_segment && */ hash_segment->type == HANDLEBARS_AST_NODE_HASH_SEGMENT ) {
            handlebars_compiler_push_param(compiler, hash_segment->node.hash_segment.value);
            keys[i++] = hash_segment->node.hash_segment.key;
        }
    }
    
    while( i-- ) {
        __OPS(assign_to_hash, keys[i]);
    }
    
    __OPN(pop_hash);
    
    handlebars_talloc_free(keys);
}

static inline void handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * partial)
{
    struct handlebars_ast_node * partial_name;
    struct handlebars_ast_node * id;
    const char * name = NULL;
    
    assert(partial != NULL);
    assert(partial->type == HANDLEBARS_AST_NODE_PARTIAL);
    
    partial_name = partial->node.partial.partial_name;
    compiler->use_partial = 1;
    
    assert(partial_name != NULL);
    assert(partial_name->type == HANDLEBARS_AST_NODE_PARTIAL_NAME);
    
    id = partial_name->node.partial_name.name;
    
    assert(id != NULL);
    
    if( id->type == HANDLEBARS_AST_NODE_ID ) {
        name = id->node.id.original;
    } else {
        // @todo might not be right
        name = handlebars_ast_node_get_string_mode_value(id);
    }
    
    if( partial->node.partial.hash ) {
        handlebars_compiler_accept(compiler, partial->node.partial.hash);
    } else {
        __OPS(push, "undefined");
    }
    
    if( partial->node.partial.context ) {
        handlebars_compiler_accept(compiler, partial->node.partial.context);
    } else {
        __OPL(get_context, 0);
        __OPN(push_context);
    }
    
    __OPS2(invoke_partial, name, partial->node.partial.indent ? partial->node.partial.indent : "");
    __OPN(append);
}

static inline void handlebars_compiler_accept_content(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * content)
{
    assert(content != NULL);
    assert(content->type == HANDLEBARS_AST_NODE_CONTENT);
    
    if( /* content && */ content->node.content.string && *content->node.content.string ) {
        __OPS(append_content, content->node.content.string);
    }
}

static inline void handlebars_compiler_accept_mustache(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * mustache)
{
    assert(mustache != NULL);
    assert(mustache->type == HANDLEBARS_AST_NODE_MUSTACHE);
    
    handlebars_compiler_accept/*_sexpr*/(compiler, mustache->node.mustache.sexpr);
    
    if( !mustache->node.mustache.unescaped && !compiler->no_escape ) {
        __OPN(append_escaped);
    } else {
        __OPN(append);
    }
}

static inline void handlebars_compiler_accept_sexpr_ambiguous(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    struct handlebars_ast_node * id;
    const char * name;
    int is_block = (programGuid >= 0 || inverseGuid >= 0);
    
    assert(sexpr != NULL);
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    id = sexpr->node.sexpr.id;
    
    assert(id != NULL);
    assert(id->type == HANDLEBARS_AST_NODE_ID); // @todo might not be ID
    
    name = handlebars_ast_node_get_id_part(id);
    
    __OPL(get_context, id->node.id.depth);
    programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
    inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
    
    handlebars_compiler_accept/*_id*/(compiler, id);
    
    do {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_ambiguous);
        handlebars_operand_set_stringval(opcode, &opcode->op1, name);
        handlebars_operand_set_boolval(&opcode->op2, is_block);
        __PUSH(opcode);
        //__OPSL(invoke_ambiguous, name, is_block);
    } while(0);
}

static inline void handlebars_compiler_accept_sexpr_simple(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    struct handlebars_ast_node * id;
    
    assert(sexpr != NULL);
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    id = sexpr->node.sexpr.id;
    
    assert(id != NULL);
    assert(id->type == HANDLEBARS_AST_NODE_ID ||
           id->type == HANDLEBARS_AST_NODE_DATA);
    
    if( id->type == HANDLEBARS_AST_NODE_DATA ) {
        handlebars_compiler_accept/*_data*/(compiler, id);
    } else if( id->type == HANDLEBARS_AST_NODE_ID ) {
        if( handlebars_ast_list_count(id->node.id.parts) ) {
            handlebars_compiler_accept/*_id*/(compiler, id);
        } else {
            handlebars_compiler_add_depth(compiler, id->node.id.depth);
            __OPL(get_context, id->node.id.depth);
            __OPN(push_context);
        }
    }
    
    __OPN(resolve_possible_lambda);
}

static inline void handlebars_compiler_accept_sexpr_helper(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr,
        long programGuid, long inverseGuid)
{
    struct handlebars_ast_list * params;
    struct handlebars_ast_node * id;
    const char * name;
    
    assert(sexpr != NULL);
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    params = handlebars_compiler_setup_full_mustache_params(
                compiler, sexpr, programGuid, inverseGuid);
    id = sexpr->node.sexpr.id;
    
    assert(id != NULL);
    assert(id->type == HANDLEBARS_AST_NODE_ID);
    
    name = handlebars_ast_node_get_id_part(id);
    
    if( handlebars_compiler_is_known_helper(compiler, id) ) {
        __OPLS(invoke_known_helper, handlebars_ast_list_count(params), name);
    } else if( compiler->known_helpers_only ) {
        compiler->errnum = handlebars_compiler_error_unknown_helper;
        compiler->error = handlebars_talloc_asprintf(compiler, 
                "You specified knownHelpersOnly, but used the unknown helper %s", name);
    } else {
        struct handlebars_opcode * opcode;
        short is_simple = (short) id->node.id.is_simple;
        
        id->node.id.is_falsy = 1;
        handlebars_compiler_accept/*_id*/(compiler, id);
        
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_helper);
        handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
        //handlebars_operand_set_stringval(compiler, &opcode->op2, name);
        handlebars_operand_set_stringval(compiler, &opcode->op2, id->node.id.original);
        handlebars_operand_set_boolval(&opcode->op3, is_simple);
        handlebars_compiler_opcode(compiler, opcode);
    }
}

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
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
    }
}

static inline void handlebars_compiler_accept_id(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * id)
{
    const char * name;
    
    assert(id != NULL);
    assert(id->type == HANDLEBARS_AST_NODE_ID);
    
    handlebars_compiler_add_depth(compiler, id->node.id.depth);
    __OPL(get_context, id->node.id.depth);
    
    name = handlebars_ast_node_get_id_part(id);
    
    if( name == NULL ) {
        __OPN(push_context);
    } else {
        struct handlebars_opcode * opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_on_context);
        char ** parts_arr = handlebars_ast_node_get_id_parts(opcode, id);
        opcode->op1.type = handlebars_operand_type_array;
        opcode->op1.data.arrayval = parts_arr;
        if( id->node.id.is_falsy ) {
            handlebars_operand_set_boolval(&opcode->op2, id->node.id.is_falsy);
        }
        if( id->node.id.is_scoped ) {
            handlebars_operand_set_boolval(&opcode->op3, id->node.id.is_scoped);
        }
        __PUSH(opcode);
    }
}

static inline void handlebars_compiler_accept_data(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * data)
{
    struct handlebars_opcode * opcode;
    struct handlebars_ast_node * id = data->node.data.id;
    char ** parts_arr;
    
    assert(id != NULL);
    
    compiler->use_data = 1;
    
    opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_data);
    parts_arr = handlebars_ast_node_get_id_parts(opcode, id);
    
    handlebars_operand_set_longval(&opcode->op1, id->node.id.depth);
    opcode->op2.type = handlebars_operand_type_array;
    opcode->op2.data.arrayval = parts_arr;
    
    __PUSH(opcode);
}

static inline void handlebars_compiler_accept_string(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * string)
{
    assert(string != NULL);
    assert(string->type == HANDLEBARS_AST_NODE_STRING);
    
    __OPS(push_string, string->node.string.string);
}

static inline void handlebars_compiler_accept_number(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * number)
{
    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_NUMBER);
    
    __OPS(push_literal, number->node.number.string);
}

static inline void handlebars_compiler_accept_boolean(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * boolean)
{
    assert(boolean != NULL);
    assert(boolean->type == HANDLEBARS_AST_NODE_BOOLEAN);
    
    __OPS(push_literal, boolean->node.boolean.string);
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
    struct handlebars_ast_node * mustache = raw_block->node.raw_block.mustache;
    struct handlebars_ast_node * program = raw_block->node.raw_block.program;
    handlebars_compiler_accept_block_internal(compiler, mustache, program, NULL);
}

/*
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
    if( !node ) {
        return;
    }
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
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return handlebars_compiler_accept_raw_block(compiler, node);
        
        // Should never get here
        default:
            assert(0);
            break;
    }
}

void handlebars_compiler_compile(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node)
{
    handlebars_compiler_accept(compiler, node);
}


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

#define __MEMCHECK(ptr) \
    do { \
        if( unlikely(ptr == NULL) ) { \
            compiler->errnum = handlebars_compiler_error_nomem; \
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
}



// Utilities

static inline void handlebars_compiler_add_depth(
        struct handlebars_compiler * compiler, int depth)
{
    assert(compiler != NULL);
    
    if( depth > 0 ) {
        compiler->depths |= (1 << depth - 1);
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
    //short is_helper = sexpr->node.sexpr.is_helper;
    //short is_eligible = sexpr->node.sexpr.eligible_helper;
    struct handlebars_ast_node * path = sexpr->node.sexpr.path;

    assert(compiler != NULL);
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    /*if( is_eligible && !is_helper ) {
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
    */
}

static inline long handlebars_compiler_compile_program(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * program)
{
    struct handlebars_compiler * subcompiler;
    long guid;
    unsigned long depths;
    int depthi;

    assert(compiler != NULL);
    assert(program != NULL);
    assert(program->type == HANDLEBARS_AST_NODE_PROGRAM ||
           program->type == HANDLEBARS_AST_NODE_CONTENT);
    
    subcompiler = handlebars_compiler_ctor(compiler);
    if( unlikely(subcompiler == NULL) ) {
        compiler->errnum = handlebars_compiler_error_nomem;
        return -1;
    }
    
    // copy compiler flags
    handlebars_compiler_set_flags(subcompiler, handlebars_compiler_get_flags(compiler));
    
    // compile
    handlebars_compiler_compile(subcompiler, program);
    guid = compiler->guid++;
    
    // copy
    if( subcompiler->errnum != 0 && compiler->errnum == 0 ) {
        compiler->errnum = subcompiler->errnum;
    }

    compiler->use_partial |= subcompiler->use_partial;
    
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
    assert(compiler != NULL);

    // Realloc opcode array
    if( compiler->opcodes_size <= compiler->opcodes_length ) {
        compiler->opcodes = handlebars_talloc_realloc(compiler, compiler->opcodes,
                    struct handlebars_opcode *, compiler->opcodes_size + 32);
        __MEMCHECK(compiler->opcodes);
        compiler->opcodes_size += 32;
    }
    
    // Append opcode
    compiler->opcodes[compiler->opcodes_length++] = opcode;
}

static inline void handlebars_compiler_push_param(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * param)
{
    assert(compiler != NULL);
    assert(param != NULL);
    if( !param ) {
        return;
    }
    
    if( compiler->string_params ) {
        int depth = 0;
        struct handlebars_opcode * opcode;
        
        if( param->type == HANDLEBARS_AST_NODE_PATH ) {
            depth = param->node.path.depth;
        }
        
        if( depth ) {
            handlebars_compiler_add_depth(compiler, depth);
        }
        __OPL(get_context, depth);
        
        // sigh
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_push_string_param);
        __MEMCHECK(opcode);

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
            __MEMCHECK(opcode);
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
        struct handlebars_ast_node * sexpr, long programGuid, long inverseGuid)
{
    struct handlebars_ast_list * params = sexpr->node.sexpr.params;

    assert(compiler != NULL);

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

    assert(compiler != NULL);
    
    if( likely(list != NULL) ) {
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
    // @todo this is broken
    /*
    struct handlebars_ast_node * sexpr = mustache->node.mustache.sexpr;
    struct handlebars_ast_node * id;
    long programGuid = -1;
    long inverseGuid = -1;

    assert(compiler != NULL);
    
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
            assert(!id || id->type == HANDLEBARS_AST_NODE_PATH);
            __OPS(block_value, id ? id->node.path.original : "");
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
    */
}

static inline void handlebars_compiler_accept_block(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * block)
{
    // @todo this is broken
    /*
    struct handlebars_ast_node * mustache = block->node.block.mustache;
    struct handlebars_ast_node * program = block->node.block.program;
    struct handlebars_ast_node * inverse = block->node.block.inverse;

    assert(compiler != NULL);

    handlebars_compiler_accept_block_internal(compiler, mustache, program, inverse);
    */
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

static inline void handlebars_compiler_accept_partial(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * partial)
{
    /*
    struct handlebars_ast_node * name;
    struct handlebars_ast_node * path;
    const char * name = NULL;

    assert(compiler != NULL);
    assert(partial != NULL);
    assert(partial->type == HANDLEBARS_AST_NODE_PARTIAL);
    
    name = partial->node.partial.name;
    compiler->use_partial = 1;
    
    // @todo this is broken
    //assert(partial_name != NULL);
    //assert(partial_name->type == HANDLEBARS_AST_NODE_PARTIAL_NAME);
    //id = partial_name->node.partial_name.name;
    //assert(id != NULL);
    
    //if( id->type == HANDLEBARS_AST_NODE_PATH ) {
    //    name = id->node.path.original;
    //} else {
        // @todo might not be right
        name = handlebars_ast_node_get_string_mode_value(id);
    //}
    
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
    */
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
    
    // @todo this is broken
    //handlebars_compiler_accept/*_sexpr*/(compiler, mustache->node.mustache.sexpr);
    
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
    struct handlebars_ast_node * path;
    const char * name;
    int is_block = (programGuid >= 0 || inverseGuid >= 0);

    assert(compiler != NULL);
    assert(sexpr != NULL);
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    path = sexpr->node.sexpr.path;
    
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    name = handlebars_ast_node_get_id_part(path);
    
    __OPL(get_context, path->node.path.depth);
    programGuid == -1 ? __OPN(push_program) : __OPL(push_program, programGuid);
    inverseGuid == -1 ? __OPN(push_program) : __OPL(push_program, inverseGuid);
    
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
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    path = sexpr->node.sexpr.path;
    
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    if( path->node.path.data ) {
        // @todo probably broken
        handlebars_compiler_accept/*_data*/(compiler, path);
    } else {
        if( handlebars_ast_list_count(path->node.path.parts) ) {
            handlebars_compiler_accept/*_id*/(compiler, path);
        } else {
            handlebars_compiler_add_depth(compiler, path->node.path.depth);
            __OPL(get_context, path->node.path.depth);
            __OPN(push_context);
        }
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
    assert(sexpr->type == HANDLEBARS_AST_NODE_SEXPR);
    
    params = handlebars_compiler_setup_full_mustache_params(
                compiler, sexpr, programGuid, inverseGuid);
    path = sexpr->node.sexpr.path;
    
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
        
        handlebars_compiler_accept/*_id*/(compiler, path);
        
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_invoke_helper);
        __MEMCHECK(opcode);
        handlebars_operand_set_longval(&opcode->op1, handlebars_ast_list_count(params));
        //handlebars_operand_set_stringval(compiler, &opcode->op2, name);
        handlebars_operand_set_stringval(compiler, &opcode->op2, path->node.path.original);
        handlebars_operand_set_boolval(&opcode->op3, 0);
        handlebars_compiler_opcode(compiler, opcode);
    }
}

static inline void handlebars_compiler_accept_sexpr(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * sexpr)
{
    assert(compiler != NULL);
    assert(sexpr != NULL);

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

static inline void handlebars_compiler_accept_path(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * path)
{
    const char * name;

    assert(compiler != NULL);
    assert(path != NULL);
    assert(path->type == HANDLEBARS_AST_NODE_PATH);
    
    handlebars_compiler_add_depth(compiler, path->node.path.depth);
    __OPL(get_context, path->node.path.depth);
    
    name = handlebars_ast_node_get_id_part(path);
    
    if( name == NULL ) {
        __OPN(push_context);
    } else {
        struct handlebars_opcode * opcode;
        char ** parts_arr;
        opcode = handlebars_opcode_ctor(compiler, handlebars_opcode_type_lookup_on_context);
        __MEMCHECK(opcode);
        parts_arr = handlebars_ast_node_get_id_parts(opcode, path);
        __MEMCHECK(parts_arr);
        opcode->op1.type = handlebars_operand_type_array;
        opcode->op1.data.arrayval = parts_arr;
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
    assert(number != NULL);
    assert(number->type == HANDLEBARS_AST_NODE_NUMBER);
    
    __OPS(push_literal, number->node.number.value);
}

static inline void handlebars_compiler_accept_boolean(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * boolean)
{
    assert(boolean != NULL);
    assert(boolean->type == HANDLEBARS_AST_NODE_BOOLEAN);
    
    __OPS(push_literal, boolean->node.boolean.value);
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
    // @todo this is broken
    /*
    struct handlebars_ast_node * mustache = raw_block->node.raw_block.mustache;
    struct handlebars_ast_node * program = raw_block->node.raw_block.program;
    handlebars_compiler_accept_block_internal(compiler, mustache, program, NULL);
    */
}

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
        case HANDLEBARS_AST_NODE_PATH:
            return handlebars_compiler_accept_path(compiler, node);
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

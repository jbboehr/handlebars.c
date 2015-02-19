
#include "handlebars_compiler.h"
#include "handlebars_memory.h"


struct handlebars_compiler * handlebars_compiler_ctor(void)
{
    struct handlebars_compiler * compiler;
    
    compiler = handlebars_talloc_zero(NULL, struct handlebars_compiler);
    
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
}

const char * handlebars_opcode_readable_type(enum handlebars_opcode_type type)
{
#define _RTYPE_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_CASE(type, name) \
    case _RTYPE_MK(type): return _RTYPE_STR(name); break

  switch( type ) {
        _RTYPE_CASE(nil, nil);
        _RTYPE_CASE(ambiguous_block_value, ambiguousBlockValue);
        _RTYPE_CASE(append, append);
        _RTYPE_CASE(append_escaped, appendEscaped);
        _RTYPE_CASE(empty_hash, emptyHash);
        _RTYPE_CASE(pop_hash, popHash);
        _RTYPE_CASE(push_context, pushContext);
        _RTYPE_CASE(push_hash, pushHash);
        _RTYPE_CASE(resolve_possible_lambda, resolvePossibleLambda);
        
        _RTYPE_CASE(get_context, getContext);
        _RTYPE_CASE(push_program, pushProgram);
        
        _RTYPE_CASE(append_content, appendContent);
        _RTYPE_CASE(assign_to_hash, assignToHash);
        _RTYPE_CASE(block_value, blockValue);
        _RTYPE_CASE(push, push);
        _RTYPE_CASE(push_literal, pushLiteral);
        _RTYPE_CASE(push_string, pushString);
        
        _RTYPE_CASE(invoke_partial, invokePartial);
        _RTYPE_CASE(push_id, pushId);
        _RTYPE_CASE(push_string_param, pushStringParam);
        
        _RTYPE_CASE(invoke_ambiguous, invokeAmbiguous);
        
        _RTYPE_CASE(invoke_known_helper, invokeKnownHelper);
        
        _RTYPE_CASE(invoke_helper, invokeHelper);
        
        _RTYPE_CASE(lookup_on_context, lookupOnContext);
        
        _RTYPE_CASE(lookup_data, lookupData);
        
        _RTYPE_CASE(invalid, invalid);
    }
    
    return "invalid";
}

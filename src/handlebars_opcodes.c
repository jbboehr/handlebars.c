
#include "handlebars_opcodes.h"

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

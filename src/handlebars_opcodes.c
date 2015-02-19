
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"

struct handlebars_opcode * handlebars_opcode_ctor(
        void * ctx, enum handlebars_opcode_type type)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
    }
    return opcode;
}

struct handlebars_opcode * handlebars_opcode_ctor_long(
        void * ctx, enum handlebars_opcode_type type, long arg)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
        handlebars_operand_set_longval(&opcode->op1, arg);
    }
    return opcode;
}

struct handlebars_opcode * handlebars_opcode_ctor_string(
        void * ctx, enum handlebars_opcode_type type, const char * arg)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
        handlebars_operand_set_stringval(ctx, &opcode->op1, arg);
    }
    return opcode;
}

void handlebars_operand_set_boolval(struct handlebars_operand * operand, short arg)
{
    operand->type = handlebars_operand_type_boolean;
    operand->data.boolval = arg;
}

void handlebars_operand_set_longval(struct handlebars_operand * operand, long arg)
{
    operand->type = handlebars_operand_type_long;
    operand->data.longval = arg;
}

void handlebars_operand_set_stringval(void * ctx, struct handlebars_operand * operand, const char * arg)
{
    operand->type = handlebars_operand_type_string;
    operand->data.stringval = handlebars_talloc_strdup(ctx, arg);
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

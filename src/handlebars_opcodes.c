
#include <string.h>

#include "handlebars.h"
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

struct handlebars_opcode * handlebars_opcode_ctor_long_string(
        void * ctx, enum handlebars_opcode_type type, long arg1, const char * arg2)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
        handlebars_operand_set_longval(&opcode->op1, arg1);
        handlebars_operand_set_stringval(ctx, &opcode->op2, arg2);
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

struct handlebars_opcode * handlebars_opcode_ctor_string2(
        void * ctx, enum handlebars_opcode_type type, const char * arg1, const char * arg2)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
        handlebars_operand_set_stringval(ctx, &opcode->op1, arg1);
        handlebars_operand_set_stringval(ctx, &opcode->op2, arg2);
    }
    return opcode;
}

struct handlebars_opcode * handlebars_opcode_ctor_string_long(
        void * ctx, enum handlebars_opcode_type type, const char * arg1, long arg2)
{
    struct handlebars_opcode * opcode = handlebars_talloc_zero(ctx, struct handlebars_opcode);
    if( opcode ) {
        opcode->type = type;
        handlebars_operand_set_stringval(ctx, &opcode->op1, arg1);
        handlebars_operand_set_longval(&opcode->op2, arg2);
    }
    return opcode;
}

void handlebars_operand_set_null(struct handlebars_operand * operand)
{
    operand->type = handlebars_operand_type_null;
    memset(&operand->data, 0, sizeof(union handlebars_operand_internals));
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

int handlebars_operand_set_stringval(void * ctx, struct handlebars_operand * operand, const char * arg)
{
    operand->type = handlebars_operand_type_string;
    operand->data.stringval = handlebars_talloc_strdup(ctx, arg);
    
    if( operand->data.stringval == NULL ) {
        operand->type = handlebars_operand_type_null;
        return HANDLEBARS_NOMEM;
    }
    
    return HANDLEBARS_SUCCESS;
}

int handlebars_operand_set_arrayval(void * ctx, struct handlebars_operand * operand, const char ** arg)
{
    char ** arr;
    char ** arrptr;
    const char ** ptr;
    size_t num = 0;
    
    // Get number of items
    for( ptr = arg; *ptr; ++ptr, ++num );
    
    // Allocate array
    arrptr = arr = handlebars_talloc_array(ctx, char *, num + 1);
    if( !arr ) {
        goto error;
    }
    
    // Copy each item
    for( ptr = arg; *ptr; ++ptr ) {
        char * tmp = handlebars_talloc_strdup(arr, *ptr);
        *arrptr++ = tmp;
    }
    *arrptr++ = NULL;
    
    // Assign to operand
    operand->type = handlebars_operand_type_array;
    operand->data.arrayval = arr;
    
    return HANDLEBARS_SUCCESS;
    
error:
    if( arr ) {
        handlebars_talloc_free(arr);
    }
    return HANDLEBARS_NOMEM;
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

enum handlebars_opcode_type handlebars_opcode_reverse_readable_type(const char * type)
{
#define _RTYPE_REV_STR(x) #x
#define _RTYPE_MK(type) handlebars_opcode_type_ ## type
#define _RTYPE_REV_CMP(str, str2) \
    if( strcmp(type, _RTYPE_REV_STR(str2)) == 0 ) { \
        return _RTYPE_MK(str); \
    }
    int l = strlen(type);
    
    switch( type[0] ) {
        case 'a':
            _RTYPE_REV_CMP(ambiguous_block_value, ambiguousBlockValue);
            _RTYPE_REV_CMP(append, append);
            _RTYPE_REV_CMP(append_escaped, appendEscaped);
            _RTYPE_REV_CMP(append_content, appendContent);
            _RTYPE_REV_CMP(assign_to_hash, assignToHash);
            break;
        case 'b':
            _RTYPE_REV_CMP(block_value, blockValue);
            break;
        case 'e':
            _RTYPE_REV_CMP(empty_hash, emptyHash);
            break;
        case 'g':
            _RTYPE_REV_CMP(get_context, getContext);
            break;
        case 'i':
            _RTYPE_REV_CMP(invoke_partial, invokePartial);
            _RTYPE_REV_CMP(invoke_ambiguous, invokeAmbiguous);
            _RTYPE_REV_CMP(invoke_known_helper, invokeKnownHelper);
            _RTYPE_REV_CMP(invoke_helper, invokeHelper);
            _RTYPE_REV_CMP(invalid, invalid);
        case 'l':
            _RTYPE_REV_CMP(lookup_on_context, lookupOnContext);
            _RTYPE_REV_CMP(lookup_data, lookupData);
            break;
        case 'n':
            _RTYPE_REV_CMP(nil, nil);
            break;
        case 'p':
            _RTYPE_REV_CMP(pop_hash, popHash);
            _RTYPE_REV_CMP(push_context, pushContext);
            _RTYPE_REV_CMP(push_hash, pushHash);
            _RTYPE_REV_CMP(push_program, pushProgram);
            _RTYPE_REV_CMP(push, push);
            _RTYPE_REV_CMP(push_literal, pushLiteral);
            _RTYPE_REV_CMP(push_string, pushString);
            _RTYPE_REV_CMP(push_id, pushId);
            _RTYPE_REV_CMP(push_string_param, pushStringParam);
            break;
        case 'r':
            _RTYPE_REV_CMP(resolve_possible_lambda, resolvePossibleLambda);
            break;
    }
    
    // Unknown :(
    return -1;
}

short handlebars_opcode_num_operands(enum handlebars_opcode_type type)
{
    switch( type ) {
        default:
        case handlebars_opcode_type_invalid:
        
        case handlebars_opcode_type_nil:
        case handlebars_opcode_type_ambiguous_block_value:
        case handlebars_opcode_type_append:
        case handlebars_opcode_type_append_escaped:
        case handlebars_opcode_type_empty_hash:
        case handlebars_opcode_type_pop_hash:
        case handlebars_opcode_type_push_context:
        case handlebars_opcode_type_push_hash:
        case handlebars_opcode_type_resolve_possible_lambda:
            return 0;
        
        case handlebars_opcode_type_get_context:
        case handlebars_opcode_type_push_program:
        
        case handlebars_opcode_type_append_content:
        case handlebars_opcode_type_assign_to_hash:
        case handlebars_opcode_type_block_value:
        case handlebars_opcode_type_push:
        case handlebars_opcode_type_push_literal:
        case handlebars_opcode_type_push_string:
            return 1;
        
        case handlebars_opcode_type_invoke_partial:
        case handlebars_opcode_type_push_id:
        case handlebars_opcode_type_push_string_param:
        
        case handlebars_opcode_type_invoke_ambiguous:
        
        case handlebars_opcode_type_invoke_known_helper:
        
        case handlebars_opcode_type_lookup_data:
            return 2;
            
        case handlebars_opcode_type_invoke_helper:
        
        case handlebars_opcode_type_lookup_on_context:
            return 3;
    }
}
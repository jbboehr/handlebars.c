
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"

char * handlebars_operand_print_append(char * str, struct handlebars_operand * operand)
{
    switch( operand->type ) {
        case handlebars_operand_type_null:
            str = handlebars_talloc_strdup_append(str, "[NULL]");
            break;
        case handlebars_operand_type_boolean:
            str = talloc_asprintf_append(str, "[BOOLEAN:%d]", operand->data.boolval);
            break;
        case handlebars_operand_type_long:
            str = talloc_asprintf_append(str, "[LONG:%ld]", operand->data.longval);
            break;
        case handlebars_operand_type_string:
            str = talloc_asprintf_append(str, "[STRING:%s]", operand->data.stringval);
            break;
        case handlebars_operand_type_array:
            str = handlebars_talloc_strdup_append(str, "[ARRAY:]");
            //str = talloc_asprintf_append(str, "[ARRAY:]");
            break;
    }
    return str;
}

char * handlebars_opcode_print(void * ctx, struct handlebars_opcode * opcode)
{
    char * str;
    const char * name;
    
    name = handlebars_opcode_readable_type(opcode->type);
    str = handlebars_talloc_strdup(ctx, name);
    str = handlebars_operand_print_append(str, &opcode->op1);
    str = handlebars_operand_print_append(str, &opcode->op2);
    str = handlebars_operand_print_append(str, &opcode->op3);
    
    return str;
}

char * handlebars_opcode_array_print(void * ctx, struct handlebars_opcode ** opcodes, size_t count)
{
    char * str = NULL;
    char * tmp;
    size_t i;
    
    for( i = 0; i < count; i++, opcodes++ ) {
        tmp = handlebars_opcode_print(ctx, *opcodes);
        if( tmp ) {
            if( str ) {
                str = talloc_asprintf_append(str, " %s", tmp);
                handlebars_talloc_free(tmp);
            } else {
                str = tmp;
            }
        }
    }
    
    return str;
}


#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"

static inline char * handlebars_operand_print_append(struct handlebars_operand * operand, char ** buf)
{
    switch( operand->type ) {
        case handlebars_operand_type_null:
            *buf = handlebars_talloc_strdup_append(*buf, "[NULL]");
            break;
        case handlebars_operand_type_boolean:
            *buf = talloc_asprintf_append(*buf, "[BOOL:%d]", operand->data.boolval);
            break;
        case handlebars_operand_type_long:
            *buf = talloc_asprintf_append(*buf, "[LONG:%d]", operand->data.longval);
            break;
        case handlebars_operand_type_string:
            *buf = talloc_asprintf_append(*buf, "[STRING:%d]", operand->data.stringval);
            break;
        case handlebars_operand_type_array:
            *buf = handlebars_talloc_strdup_append(*buf, "[ARRAY:]");
            //*buf = talloc_asprintf_append(*buf, "[ARRAY:]");
            break;
    }
    return *buf;
}

char * handlebars_opcode_print(void * ctx, struct handlebars_opcode * opcode)
{
    char * str;
    const char * name;
    
    name = handlebars_opcode_readable_type(opcode->type);
    str = handlebars_talloc_strdup(ctx, name);
    str = handlebars_operand_print_append(&opcode->op1, &str);
    str = handlebars_operand_print_append(&opcode->op2, &str);
    str = handlebars_operand_print_append(&opcode->op3, &str);
    
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

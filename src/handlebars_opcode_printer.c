
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <handlebars_string.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"



struct handlebars_opcode_printer * handlebars_opcode_printer_ctor(struct handlebars_context * context)
{
    struct handlebars_opcode_printer * printer = MC(handlebars_talloc_zero(context, struct handlebars_opcode_printer));
    printer->output = MC(handlebars_talloc_strdup(printer, ""));
    return printer;
}

void handlebars_opcode_printer_dtor(struct handlebars_opcode_printer * printer)
{
    assert(printer != NULL);

    handlebars_talloc_free(printer);
}

char * handlebars_operand_print_append(struct handlebars_context * context, char * str, struct handlebars_operand * operand)
{
    assert(str != NULL);
    assert(operand != NULL);

    switch( operand->type ) {
        case handlebars_operand_type_null:
            str = handlebars_talloc_strdup_append(str, "[NULL]");
            break;
        case handlebars_operand_type_boolean:
            str = handlebars_talloc_asprintf_append(str, "[BOOLEAN:%d]", operand->data.boolval);
            break;
        case handlebars_operand_type_long:
            str = handlebars_talloc_asprintf_append(str, "[LONG:%ld]", operand->data.longval);
            break;
        case handlebars_operand_type_string: {
            struct handlebars_string * tmp = handlebars_string_addcslashes(context, operand->data.string, HBS_STRL("\r\n\t"));
            str = handlebars_talloc_asprintf_append(str, "[STRING:%s]", tmp->val);
            handlebars_talloc_free(tmp);
            break;
        }
        case handlebars_operand_type_array: {
            struct handlebars_string ** tmp = operand->data.array;
            str = handlebars_talloc_strdup_append(str, "[ARRAY:");
            for( ; *tmp; ++tmp ) {
                if( tmp != operand->data.array ) {
                    str = handlebars_talloc_strdup_append(str, ",");
                }
                str = handlebars_talloc_strdup_append(str, (*tmp)->val);
            }
            str = handlebars_talloc_strdup_append(str, "]");
            break;
        }
    }
    return str;
}

char * handlebars_opcode_print_append(struct handlebars_context * context, char * str, struct handlebars_opcode * opcode, int flags)
{
    const char * name = handlebars_opcode_readable_type(opcode->type);
    short num = handlebars_opcode_num_operands(opcode->type);
    
    str = handlebars_talloc_strdup_append(str, name);
    
    // @todo add option to override this
    if( num >= 1 ) {
        str = handlebars_operand_print_append(context, str, &opcode->op1);
    } else {
        assert(opcode->op1.type == handlebars_operand_type_null);
    }
    if( num >= 2 ) {
        str = handlebars_operand_print_append(context, str, &opcode->op2);
    } else {
        assert(opcode->op2.type == handlebars_operand_type_null);
    }
    if( num >= 3 ) {
        str = handlebars_operand_print_append(context, str, &opcode->op3);
    } else {
        assert(opcode->op3.type == handlebars_operand_type_null);
    }
    if( num >= 4 ) {
        str = handlebars_operand_print_append(context, str, &opcode->op4);
    } else {
        assert(opcode->op4.type == handlebars_operand_type_null);
    }
    
    // Add location
    if( flags & handlebars_opcode_printer_flag_locations ) {
        str = handlebars_talloc_asprintf_append(str, " [%d:%d-%d:%d]", 
                opcode->loc.first_line, opcode->loc.first_column,
                opcode->loc.last_line, opcode->loc.last_column);
    }
    
    return str;
}

char * handlebars_opcode_print(struct handlebars_context * context, struct handlebars_opcode * opcode)
{
    char * str = MC(handlebars_talloc_strdup(context, ""));
    return MC(handlebars_opcode_print_append(context, str, opcode, 0));
}

#undef CONTEXT
#define CONTEXT printer->ctx

static void handlebars_opcode_printer_array_print(struct handlebars_opcode_printer * printer)
{
    size_t indent = printer->indent < 8 ? printer->indent * 2 : 16;
    char indentbuf[17];
    struct handlebars_opcode ** opcodes = printer->opcodes;
    size_t count = printer->opcodes_length;
    size_t i;
    
    memset(&indentbuf, ' ', indent);
    indentbuf[indent] = 0;
    
    for( i = 0; i < count; i++, opcodes++ ) {
        printer->output = MC(handlebars_talloc_strdup_append(printer->output, indentbuf));
        printer->output = MC(handlebars_opcode_print_append(CONTEXT, printer->output, *opcodes, printer->flags));
        printer->output = MC(handlebars_talloc_strdup_append(printer->output, "\n"));
    }
    
    printer->output = MC(handlebars_talloc_asprintf_append(printer->output, "%s-----\n", indentbuf));
}

void handlebars_opcode_printer_print(struct handlebars_opcode_printer * printer, struct handlebars_compiler * compiler)
{
    size_t i;
    char indentbuf[17];
    size_t indent;
    struct handlebars_compiler * child;

    printer->ctx = HBSCTX(compiler);
    
    // Make indent
    indent = printer->indent < 8 ? printer->indent * 2 : 16;
    memset(&indentbuf, ' ', indent);
    indentbuf[indent] = 0;
    
    // Print opcodes
    printer->opcodes = compiler->opcodes;
    printer->opcodes_length = compiler->opcodes_length;
    handlebars_opcode_printer_array_print(printer);
    
    printer->indent++;
    
    // Print decorators
    for( i = 0; i < compiler->decorators_length; i++ ) {
        printer->output = handlebars_talloc_asprintf_append(printer->output, "%sDECORATOR\n", indentbuf);
        child = *(compiler->decorators + i);
        handlebars_opcode_printer_print(printer, child);
    }
    
    // Print children
    for( i = 0; i < compiler->children_length; i++ ) {
        child = *(compiler->children + i);
        handlebars_opcode_printer_print(printer, child);
    }
    
    printer->indent--;
}

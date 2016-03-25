
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



struct handlebars_opcode_printer {
    struct handlebars_context * ctx;
    struct handlebars_opcode ** opcodes;
    size_t opcodes_length;
    size_t indent;
    int flags;
    struct handlebars_string * output;
};



static struct handlebars_opcode_printer * handlebars_opcode_printer_ctor(struct handlebars_context * context)
{
    struct handlebars_opcode_printer * printer = MC(handlebars_talloc_zero(context, struct handlebars_opcode_printer));
    printer->output = handlebars_string_init(context, 128);
    return printer;
}

static inline struct handlebars_string * append_indent(struct handlebars_context * context, struct handlebars_string * string, size_t num)
{
    num *= 2;
    char tmp[256];
    memset(tmp, ' ', num);
    tmp[num] = 0;
    return handlebars_string_append(context, string, tmp, num);
}

struct handlebars_string * handlebars_operand_print_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    struct handlebars_operand * operand
) {
    struct handlebars_string * tmp;
    struct handlebars_string ** arr;

    assert(string != NULL);
    assert(operand != NULL);

    switch( operand->type ) {
        case handlebars_operand_type_null:
            string = handlebars_string_append(context, string, HBS_STRL("[NULL]"));
            break;
        case handlebars_operand_type_boolean:
            string = handlebars_string_asprintf_append(context, string, "[BOOLEAN:%d]", operand->data.boolval);
            break;
        case handlebars_operand_type_long:
            string = handlebars_string_asprintf_append(context, string, "[LONG:%ld]", operand->data.longval);
            break;
        case handlebars_operand_type_string:
            tmp = handlebars_string_addcslashes(context, operand->data.string, HBS_STRL("\r\n\t"));
            string = handlebars_string_asprintf_append(context, string, "[STRING:%.*s]", (int) tmp->len, tmp->val);
            handlebars_talloc_free(tmp);
            break;
        case handlebars_operand_type_array: {
            arr = operand->data.array;
            string = handlebars_string_append(context, string, HBS_STRL("[ARRAY:"));
            for( ; *arr; ++arr ) {
                if( arr != operand->data.array ) {
                    string = handlebars_string_append(context, string, HBS_STRL(","));
                }
                string = handlebars_string_append(context, string, HBS_STR_STRL((*arr)));
            }
            string = handlebars_string_append(context, string, HBS_STRL("]"));
            break;
        }
    }
    return string;
}

struct handlebars_string * handlebars_operand_print(
    struct handlebars_context * context,
    struct handlebars_operand * operand
) {
    struct handlebars_string * string = handlebars_string_init(context, 64);
    return handlebars_operand_print_append(context, string, operand);
}

struct handlebars_string * handlebars_opcode_print_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    struct handlebars_opcode * opcode,
    int flags
) {
    const char * name = handlebars_opcode_readable_type(opcode->type);
    short num = handlebars_opcode_num_operands(opcode->type);

    string = handlebars_string_append(context, string, name, strlen(name));

    if( num >= 1 ) {
        string = handlebars_operand_print_append(context, string, &opcode->op1);
    } else {
        assert(opcode->op1.type == handlebars_operand_type_null);
    }
    if( num >= 2 ) {
        string = handlebars_operand_print_append(context, string, &opcode->op2);
    } else {
        assert(opcode->op2.type == handlebars_operand_type_null);
    }
    if( num >= 3 ) {
        string = handlebars_operand_print_append(context, string, &opcode->op3);
    } else {
        assert(opcode->op3.type == handlebars_operand_type_null);
    }
    if( num >= 4 ) {
        string = handlebars_operand_print_append(context, string, &opcode->op4);
    } else {
        assert(opcode->op4.type == handlebars_operand_type_null);
    }
    
    // Add location
    if( flags & handlebars_opcode_printer_flag_locations ) {
        string = handlebars_string_asprintf_append(context, string, " [%d:%d-%d:%d]",
                opcode->loc.first_line, opcode->loc.first_column,
                opcode->loc.last_line, opcode->loc.last_column);
    }
    
    return string;
}

struct handlebars_string * handlebars_opcode_print(
    struct handlebars_context * context,
    struct handlebars_opcode * opcode,
    int flags
) {
    struct handlebars_string * string = handlebars_string_init(context, 64);
    return handlebars_opcode_print_append(context, string, opcode, flags);
}

#undef CONTEXT
#define CONTEXT printer->ctx

static void handlebars_opcode_printer_array_print(struct handlebars_opcode_printer * printer)
{
    struct handlebars_opcode ** opcodes = printer->opcodes;
    size_t count = printer->opcodes_length;
    size_t i;
    
    for( i = 0; i < count; i++, opcodes++ ) {
        printer->output = append_indent(CONTEXT, printer->output, printer->indent);
        printer->output = handlebars_opcode_print_append(CONTEXT, printer->output, *opcodes, printer->flags);
        printer->output = handlebars_string_append(CONTEXT, printer->output, HBS_STRL("\n"));
    }

    printer->output = append_indent(CONTEXT, printer->output, printer->indent);
    printer->output = handlebars_string_append(CONTEXT, printer->output, HBS_STRL("-----\n"));
}

static void handlebars_opcode_printer_print(struct handlebars_opcode_printer * printer, struct handlebars_compiler * compiler)
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
        printer->output = handlebars_talloc_asprintf_append(printer->output, "%.*sDECORATOR\n", (int) indent, indentbuf);
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

struct handlebars_string * handlebars_compiler_print(struct handlebars_compiler * compiler, int flags)
{
    struct handlebars_opcode_printer * printer = handlebars_opcode_printer_ctor(HBSCTX(compiler));
    struct handlebars_string * ret;

    handlebars_opcode_printer_print(printer, compiler);
    ret = handlebars_string_ctor(HBSCTX(compiler), printer->output, strlen(printer->output));
    handlebars_talloc_free(printer);

    return ret;
}
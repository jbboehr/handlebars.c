/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



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
    struct handlebars_operand_string * arr;
    size_t i;

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
            tmp = handlebars_string_addcslashes(context, operand->data.string.string, HBS_STRL("\r\n\t"));
            string = handlebars_string_asprintf_append(context, string, "[STRING:%.*s]", (int) hbs_str_len(tmp), hbs_str_val(tmp));
            handlebars_talloc_free(tmp);
            break;
        case handlebars_operand_type_array: {
            arr = operand->data.array.array;
            string = handlebars_string_append(context, string, HBS_STRL("[ARRAY:"));
            for( i = 0 ; i < operand->data.array.count; ++i ) {
                if( i > 0 ) {
                    string = handlebars_string_append(context, string, HBS_STRL(","));
                }
                string = handlebars_string_append(context, string, HBS_STR_STRL((arr + i)->string));
            }
            string = handlebars_string_append(context, string, HBS_STRL("]"));
            break;
        }
        default: assert(0); break; // LCOV_EXCL_LINE
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
        // hack for invoke_ambiguous
        if (opcode->type == handlebars_opcode_type_invoke_ambiguous && opcode->op3.type == handlebars_operand_type_null) {
            // ignore
        } else {
            string = handlebars_operand_print_append(context, string, &opcode->op3);
        }
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

static void handlebars_opcode_printer_print(struct handlebars_opcode_printer * printer, struct handlebars_program * program)
{
    size_t i;
    struct handlebars_program * child;

    // Print opcodes
    printer->opcodes = program->opcodes;
    printer->opcodes_length = program->opcodes_length;
    handlebars_opcode_printer_array_print(printer);

    printer->indent++;

    // Print decorators
    for( i = 0; i < program->decorators_length; i++ ) {
        printer->output = append_indent(CONTEXT, printer->output, (size_t) printer->indent);
        printer->output = handlebars_string_append(CONTEXT, printer->output, HBS_STRL("DECORATOR\n"));
        child = *(program->decorators + i);
        handlebars_opcode_printer_print(printer, child);
    }

    // Print children
    for( i = 0; i < program->children_length; i++ ) {
        child = *(program->children + i);
        handlebars_opcode_printer_print(printer, child);
    }

    printer->indent--;
}

struct handlebars_string * handlebars_program_print(
    struct handlebars_context * context,
    struct handlebars_program * program,
    int flags
) {
    struct handlebars_opcode_printer * printer = handlebars_opcode_printer_ctor(context);
    printer->ctx = context;
    printer->flags = flags;
    handlebars_opcode_printer_print(printer, program);
    return printer->output;
}


/**
 * @file
 * @brief Functions for printing opcodes into a human-readable string
 */

#ifndef HANDLEBARS_OPCODE_PRINTER_H
#define HANDLEBARS_OPCODE_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_opcode;
struct handlebars_operand;
struct handlebars_compiler;

enum handlebars_opcode_printer_flag {
    handlebars_opcode_printer_flag_none = 0,
    handlebars_opcode_printer_flag_no_newlines,
    handlebars_opcode_printer_flag_dump_all_operands
};

struct handlebars_opcode_printer {
    struct handlebars_opcode ** opcodes;
    size_t opcodes_length;
    int indent;
    int flags;
    char * output;
};

struct handlebars_opcode_printer * handlebars_opcode_printer_ctor(void * ctx);

void handlebars_opcode_printer_dtor(struct handlebars_opcode_printer * printer);

char * handlebars_operand_print_append(char * str, struct handlebars_operand * operand);

char * handlebars_opcode_print_append(char * str, struct handlebars_opcode * opcode);

char * handlebars_opcode_print(void * ctx, struct handlebars_opcode * opcode);

char * handlebars_opcode_array_print(void * ctx, struct handlebars_opcode ** opcodes, size_t count);

void handlebars_opcode_printer_print(struct handlebars_opcode_printer * printer, struct handlebars_compiler * compiler);

#ifdef	__cplusplus
}
#endif

#endif
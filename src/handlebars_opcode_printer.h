
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

char * handlebars_opcode_print(void * ctx, struct handlebars_opcode * opcode);

char * handlebars_opcode_array_print(void * ctx, struct handlebars_opcode ** opcodes, size_t count);

#ifdef	__cplusplus
}
#endif

#endif

/**
 * @file
 * @brief Functions for printing opcodes into a human-readable string
 */

#ifndef HANDLEBARS_OPCODE_PRINTER_H
#define HANDLEBARS_OPCODE_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_opcode;
struct handlebars_operand;
struct handlebars_compiler;

/**
 * @brief Flags to control opcode printer behaviour
 */
enum handlebars_opcode_printer_flag {
    /**
     * @brief Default printer behaviour
     */
    handlebars_opcode_printer_flag_none = 0,

    /**
     * @brief Join with spaces instead of newlines
     */
    handlebars_opcode_printer_flag_no_newlines,

    /**
     * @brief Dump all operands (not implemented)
     */
    handlebars_opcode_printer_flag_dump_all_operands
};

/**
 * @brief Opcode printer context
 */
struct handlebars_opcode_printer {
    struct handlebars_opcode ** opcodes;
    size_t opcodes_length;
    int indent;
    int flags;
    char * output;
};

/**
 * @brief Construct an opcode printer context
 *
 * @param[in] ctx The talloc parent context
 */
struct handlebars_opcode_printer * handlebars_opcode_printer_ctor(void * ctx);

/**
 * @brief Destruct an opcode printer context
 *
 * @param[in] printer The printer context to destruct
 */
void handlebars_opcode_printer_dtor(struct handlebars_opcode_printer * printer);

/**
 * @brief Print an operand and append it to the specified buffer
 *
 * @param[in] str The string to which to append
 * @param[in] operand The operand to print
 * @return The original pointer, or a new pointer if reallocated
 */
char * handlebars_operand_print_append(char * str, struct handlebars_operand * operand);

/**
 * @brief Print an opcode and append it to the specified buffer
 *
 * @param[in] str The string to which to append
 * @param[in] opcode The opcode to print
 * @return The original pointer, or a new pointer if reallocated
 */
char * handlebars_opcode_print_append(char * str, struct handlebars_opcode * opcode);

/**
 * @brief Print an opcode and return the string
 *
 * @param[in] opcode The opcode to print
 * @return The printed opcode
 */
char * handlebars_opcode_print(void * ctx, struct handlebars_opcode * opcode);

/**
 * @brief Print a compiler context
 *
 * @param[in] printer The printer context
 * @param[in] compiler The compiler context
 * @return void
 */
void handlebars_opcode_printer_print(struct handlebars_opcode_printer * printer, struct handlebars_compiler * compiler);

#ifdef	__cplusplus
}
#endif

#endif

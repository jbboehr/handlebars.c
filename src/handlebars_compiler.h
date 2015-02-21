
/**
 * @file
 * @brief Compiler
 */

#ifndef HANDLEBARS_COMPILER_H
#define HANDLEBARS_COMPILER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stddef.h>


struct handlebars_ast_node;


enum handlebars_compiler_flag {
    handlebars_compiler_flag_none = 0,
    
    // Option flags
    handlebars_compiler_flag_use_depths = (1 << 0),
    handlebars_compiler_flag_string_params = (1 << 1),
    handlebars_compiler_flag_track_ids = (1 << 2),
    handlebars_compiler_flag_no_escape = (1 << 3),
    handlebars_compiler_flag_known_helpers_only = (1 << 4),
    
    // Result flags
    handlebars_compiler_flag_use_partial = (1 << 8),
    handlebars_compiler_flag_is_simple = (1 << 9),
    
    // Composite option flags
    handlebars_compiler_flag_compat = (1 << 0),
    handlebars_compiler_flag_all = ((1 << 5) - 1)
};

enum handlebars_compiler_sexpr_type {
    handlebars_compiler_sexpr_type_ambiguous = 0,
    handlebars_compiler_sexpr_type_helper = 1,
    handlebars_compiler_sexpr_type_simple = 2
};

struct handlebars_compiler {
    
    struct handlebars_opcode ** opcodes;
    size_t opcodes_length;
    size_t opcodes_size;
    
    // @todo children
    
    unsigned long depths;
    
    const char ** known_helpers;
    
    // Symbol counter
    long guid;
    
    // Flags
    int flags;
    
    // Option flags
    short string_params;
    short track_ids;
    short use_depths;
    short no_escape;
    short known_helpers_only;
    
    // Result flags
    short is_simple;
    short use_partial;
};

void handlebars_compiler_compile(
        struct handlebars_compiler * compiler, struct handlebars_ast_node * node);

/**
 * @brief Construct a compiler context object.
 * 
 * @param[in] ctx The memory context
 * @return the compiler context pointer
 */
struct handlebars_compiler * handlebars_compiler_ctor(void * ctx);

/**
 * @brief Free a compiler context and it's resouces.
 * 
 * @param[in] compiler
 * @return void
 */
void handlebars_compiler_dtor(struct handlebars_compiler * compiler);

/**
 * @brief Get the compiler flags.
 * 
 * @param[in] compiler
 * @return the compiler flags
 */
int handlebars_compiler_get_flags(struct handlebars_compiler * compiler);

/**
 * @brief Set the compiler flags, with handlebars_compiler_flag_all as a 
 *        mask.
 * 
 * @param[in] compiler
 * @param[in] flags
 * @return void
 */
void handlebars_compiler_set_flags(struct handlebars_compiler * compiler, int flags);

#ifdef	__cplusplus
}
#endif

#endif

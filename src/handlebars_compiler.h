
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


enum handlebars_compiler_flag {
    handlebars_compiler_flag_none = 0,
    
    // Option flags
    handlebars_compiler_flag_use_depths = 1,
    handlebars_compiler_flag_string_params = 2,
    handlebars_compiler_flag_track_ids = 4,
    
    // Result flags
    handlebars_compiler_flag_use_partial = 16,
    handlebars_compiler_flag_is_simple = 32,
    
    // Composite option flags
    handlebars_compiler_flag_compat = 1,
    handlebars_compiler_flag_all = 7
};

struct handlebars_compiler {
    
    // @todo opcodes children depths
    
    // Symbol counter
    size_t guid;
    
    // Flags
    int flags;
    
    // Option flags
    short string_params;
    short track_ids;
    short use_depths;
    
    // Result flags
    short is_simple;
    short use_partial;
};

/**
 * @brief Construct a compiler context object.
 * 
 * @return the compiler context pointer
 */
struct handlebars_compiler * handlebars_compiler_ctor(void);

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

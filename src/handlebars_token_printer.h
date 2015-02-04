
#ifndef HANDLEBARS_TOKEN_PRINTER_H
#define HANDLEBARS_TOKEN_PRINTER_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Declarations
 */
struct handlebars_token;
struct handlebars_token_list;

/**
 * @brief Flags to control print behaviour 
 */
enum handlebars_token_printer_flags {
    /**
     * @brief Default print behaviour flag
     */
    handlebars_token_printer_flag_none = 0,
    
    /**
     * @brief Use newlines between tokens instead of spaces
     */
    handlebars_token_printer_flag_newlines = 1
};

/**
 * @brief Print a token into a human-readable string
 * 
 * @param[in] token The token to print
 * @param[in] flags The print flags
 * @return The printed token
 */
char * handlebars_token_print(struct handlebars_token * token, int flags);

/**
 * @brief Print a token list into a human-readable string 
 * 
 * @param[in] list The token list to print
 * @param[in] flags The print flags
 * @return The printed tokens
 */
char * handlebars_token_list_print(struct handlebars_token_list * list, int flags);

#ifdef	__cplusplus
}
#endif

#endif

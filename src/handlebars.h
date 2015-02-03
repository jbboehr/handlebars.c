
#ifndef HANDLEBARS_H
#define HANDLEBARS_H

#ifdef	__cplusplus
extern "C" {
#endif

// Pre-declarations
struct handlebars_context;
struct handlebars_token_list;

// Annoying lex missing prototypes
int handlebars_yy_get_column(void * yyscanner);
void handlebars_yy_set_column(int column_no, void * yyscanner);

// Macros
#define handlebars_stringify(x) #x

// Error types
#define HANDLEBARS_SUCCESS 0
#define HANDLEBARS_ERROR 1
#define HANDLEBARS_NOMEM 2
#define HANDLEBARS_NULLARG 3
#define HANDLEBARS_PARSEERR 4

/**
 * @brief Get the library version as an integer
 * 
 * @return the version
 */
int handlebars_version(void);

/**
 * @brief Get the library version as a string
 * 
 * @return the version
 */
const char * handlebars_version_string(void);

/**
 * @brief Convenience function for lexing to a token list
 * 
 * @param[in] ctx
 * @return the token list
 */
struct handlebars_token_list * handlebars_lex(struct handlebars_context * ctx);

#ifdef	__cplusplus
}
#endif

#endif


/**
 * @file
 * @brief Utilities
 */

#ifndef HANDLEBARS_UTILS_H
#define HANDLEBARS_UTILS_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Declarations
 */
struct handlebars_context;
struct YYLTYPE;
union YYSTYPE;

/**
 * @brief Adds slashes to as string for a list of specified characters. Returns a  
 *        newly allocated string, or NULL on failure.
 * 
 * @param[in] str The string to which to add slashes
 * @param[in] str_length The length of the input string
 * @param[in] what A list of characters to escape
 * @param[in] what_length The length of the character list
 * @return The string with escaped characters
 */
char * handlebars_addcslashes(const char * str, size_t str_length, const char * what, size_t what_length);

/**
 * @brief Trims a set of characters off the right end of string. Trims in
 *        place by setting a null terminator.
 * 
 * @param[in] string the string to trim
 * @param[in] what the set of characters to trim
 * @return the original pointer, with null moved to trim input characters
 */
char * handlebars_rtrim(char * string, const char * what);

/**
 * @brief Strips slashes from a string. Changes are done in-place. length accepts
 *        NULL, uses strlen for length.
 * 
 * @param[in] str The input string
 * @param[in] length The length of the input string
 * @return The original pointer, transformed
 */
void handlebars_stripcslashes(char * str, size_t * length);

/**
 * @brief Handle an error in the parser. Prints message to stderr
 * 
 * @param[in] lloc The parser location info
 * @param[in] context The context object
 * @param[in] err The error message
 * @return void
 */
void handlebars_yy_error(struct YYLTYPE * lloc, struct handlebars_context * context, const char * err);

/**
 * @brief Handle a fatal error in the parser. Prints message to stderr and exits with code 2.
 * 
 * @param[in] msg An error message
 * @param[in] yyscanner The scanner object
 * @return void
 */
void handlebars_yy_fatal_error(const char * msg, void * yyscanner);

/**
 * @brief Reads input for the lexer. Reads input from the tmpl field of the context object
 * 
 * @param[out] buffer The buffer to store input into
 * @param[out] numBytesRead The number of bytes read
 * @param[in] maxBytesToRead The maximum number of bytes to read
 * @param[in] context The handlebars context
 * @return void
 */
void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context);

/**
 * @brief Print a parser value
 * 
 * @param[in] file The file handle to which to write
 * @param[in] type The parser node type
 * @param[in] value The parser node value
 * @return void
 */
void handlebars_yy_print(FILE *file, int type, union YYSTYPE value);

/**
 * @brief Custom alloc for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 * 
 * @param[in] bytes The number of bytes to allocate
 * @param[in] yyscanner The scanner context. NULL if allocating the yyscanner itself.
 * @return A pointer to the newly allocated memory, or NULL on failure
 */
void * handlebars_yy_alloc(size_t bytes, void * yyscanner);

/**
 * @brief Custom realloc for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 * 
 * @param[in] ptr The pointer to reallocate
 * @param[in] bytes The desired new size
 * @param[in] yyscanner The scanner context
 * @return The original pointer, or a new pointer, or NULL on failure 
 */
void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner);

/**
 * @brief Custom free for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 * 
 * @param[in] ptr The pointer to free
 * @param[in] yyscanner The scanner context
 * @return void
 */
void handlebars_yy_free(void * ptr, void * yyscanner);

#ifdef	__cplusplus
}
#endif

#endif

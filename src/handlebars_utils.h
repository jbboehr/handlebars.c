
#ifndef HANDLEBARS_UTILS_H
#define HANDLEBARS_UTILS_H

#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Pre-declarations
 */
struct handlebars_context;
struct YYLTYPE;
union YYSTYPE;

/**
 * Adds slashes to as string for a list of specified characters. Returns a  
 * newly allocated string, or NULL on failure.
 */
char * handlebars_addcslashes(const char * str, size_t str_length, const char * what, size_t what_length);

/**
 * @brief Trims a set of characters off the right end of string
 * 
 * @param[in] string the string to trim
 * @param[in] what the set of characters to trim
 * @return the original pointer
 */
char * handlebars_rtrim(char * string, const char * what);

/**
 * Strips slashes from a string. Changes are done in-place. length accepts
 * NULL, uses strlen for length.
 */
void handlebars_stripcslashes(char * str, size_t * length);

void handlebars_yy_error(struct YYLTYPE * lloc, struct handlebars_context * context, const char * err);
void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context);
void handlebars_yy_fatal_error(const char * msg, void * yyscanner);
void handlebars_yy_print(FILE *file, int type, union YYSTYPE value);

// Allocators for a reentrant scanner (flex)
// We use the scanner pointer as a talloc context
struct handlebars_context * _handlebars_context_tmp;
void * handlebars_yy_alloc(size_t bytes, void * yyscanner);
void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner);
void handlebars_yy_free(void * ptr, void * yyscanner);

#ifdef	__cplusplus
}
#endif

#endif

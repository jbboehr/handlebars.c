
#ifndef HANDLEBARS_UTILS_H
#define HANDLEBARS_UTILS_H

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
 * Strips slashes from a string. Changes are done in-place. length accepts
 * NULL, uses strlen for length.
 */
void handlebars_stripcslashes(char * str, size_t * length);

void handlebars_yy_error(struct YYLTYPE * lloc, struct handlebars_context * context, const char * err);
void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context);
void handlebars_yy_fatal_error(const char * msg, void * yyscanner);
void handlebars_yy_print(FILE *file, int type, union YYSTYPE value);
     
#ifdef	__cplusplus
}
#endif

#endif

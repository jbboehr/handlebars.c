
#ifndef HANDLEBARS_H
#define HANDLEBARS_H

#ifdef	__cplusplus
extern "C" {
#endif

// Pre-declarations
struct handlebars_context;
struct handlebars_token_list;

// Macros
#define handlebars_stringify(x) #x

// Error types
#define HANDLEBARS_SUCCESS 0
#define HANDLEBARS_ERROR 1
#define HANDLEBARS_NOMEM 2
#define HANDLEBARS_NULLARG 3
#define HANDLEBARS_PARSEERR 4

// Version functions
int handlebars_version(void);
const char * handlebars_version_string(void);

// Annoying lex missing prototypes
int handlebars_yy_get_column(void * yyscanner);
void handlebars_yy_set_column(int column_no, void * yyscanner);

// Convenience function
struct handlebars_token_list * handlebars_lex(struct handlebars_context * ctx);

#ifdef	__cplusplus
}
#endif

#endif

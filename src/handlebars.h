
#ifndef HANDLEBARS_H
#define HANDLEBARS_H

#ifdef	__cplusplus
extern "C" {
#endif

// Error types
#define HANDLEBARS_SUCCESS 0
#define HANDLEBARS_ERROR 1
#define HANDLEBARS_NOMEM 2
#define HANDLEBARS_NULLARG 3

// Annoying lex missing prototypes
int handlebars_yy_get_column(void * yyscanner);
void handlebars_yy_set_column(int column_no, void * yyscanner);

#define handlebars_stringify(x) #x

#ifdef	__cplusplus
}
#endif

#endif

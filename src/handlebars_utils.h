
#ifndef HANDLEBARS_UTILS_H
#define HANDLEBARS_UTILS_H

/**
 * Pre-declarations
 */
struct handlebars_context;
struct YYLTYPE;

void handlebars_yy_error(struct YYLTYPE * lloc, struct handlebars_context * context, const char * err);
void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context);
void handlebars_yy_fatal_error(const char * msg, void * yyscanner);

#endif

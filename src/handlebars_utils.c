
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"

const char * handlebars_token_readable_type(int type)
{
  switch( type ) {
    case END: return "END";
    case CONTENT: return "CONTENT";
    case WHITESPACE: return "WHITESPACE";
    default: return "UNKNOWN";
  }
}

void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context)
{
  int numBytesToRead = maxBytesToRead;
  int bytesRemaining = strlen(context->tmpl) - context->tmplReadOffset;
  int i;
  if ( numBytesToRead > bytesRemaining ) { numBytesToRead = bytesRemaining; }
  for ( i = 0; i < numBytesToRead; i++ ) {
      buffer[i] = context->tmpl[context->tmplReadOffset+i];
  }
  *numBytesRead = numBytesToRead;
  context->tmplReadOffset += numBytesToRead;
}

void handlebars_yy_error(struct YYLTYPE * lloc, struct handlebars_context * context, const char * err)
{
#if defined(YYDEBUG) && YYDEBUG
  fprintf(stderr, "%d : %s\n", lloc->first_line, err);
#endif
  context->error = strdup(err);
  context->errloc = malloc(sizeof(YYLTYPE));
  memcpy(context->errloc, lloc, sizeof(YYLTYPE));
}

void handlebars_yy_fatal_error(const char * msg, void * yyscanner)
{
  fprintf(stderr, "%s\n", msg);
  exit(2);
}


#include <memory.h>
#include <stdlib.h>
#include "handlebars.h"
#include "handlebars_context.h"

extern int handlebars_yy_lex_init(void ** scanner);
extern void handlebars_yy_set_extra(handlebars_context * user_defined, void * yyscanner );
extern int handlebars_yy_lex_destroy(void * yyscanner);

int handlebars_context_ctor(handlebars_context * context)
{
  int ret;

  if( NULL == context ) {
    return 1;
  }

  memset(context, 0, sizeof(handlebars_context));

  ret = handlebars_yy_lex_init(&context->scanner);
  if( ret == 0 ) {
	  handlebars_yy_set_extra(context, context->scanner);
  }
  return ret;
}

int handlebars_context_dtor(handlebars_context * context)
{
  int ret;

  if( NULL == context ) {
    return 1;
  }

  ret = handlebars_yy_lex_destroy(context->scanner);
  context->scanner = NULL;
  return ret;
}

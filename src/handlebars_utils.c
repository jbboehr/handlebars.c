
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"

char * handlebars_addcslashes(const char * str, size_t str_length, const char * what, size_t what_length)
{
	char flags[256];
	char * source;
	char * target;
	char * end;
	char c;
	size_t i;
	char * new_str;
	size_t new_str_length;

	// Make char mask
	memset(flags, 0, sizeof(flags));
	for( i = 0; i < what_length; i++ ) {
		flags[(unsigned char) what[i]] = 1;
	}

	new_str = talloc_zero_size(NULL, sizeof(char) * str_length * 4 + 1);

	source = (char *) str;
	target = new_str;
	end = source + str_length;
	for( ; source < end ; source++ ) {
		c = *source;
		if( flags[(unsigned char) c] ) {
			if ((unsigned char) c < 32 || (unsigned char) c > 126) {
				*target++ = '\\';
				switch (c) {
					case '\n': *target++ = 'n'; break;
					case '\t': *target++ = 't'; break;
					case '\r': *target++ = 'r'; break;
					case '\a': *target++ = 'a'; break;
					case '\v': *target++ = 'v'; break;
					case '\b': *target++ = 'b'; break;
					case '\f': *target++ = 'f'; break;
					default: target += sprintf(target, "%03o", (unsigned char) c);
				}
				continue;
			}
			*target++ = '\\';

		}
		*target++ = c;
	}
	*target = 0;
	new_str_length = target - new_str;
	if( new_str_length < str_length * 4 + 1 ) {
		char * tmp = talloc_strndup(NULL, new_str, new_str_length + 1);
		talloc_free(new_str);
		new_str = tmp;
	}
	return new_str;
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

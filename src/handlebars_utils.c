
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
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

	new_str = handlebars_talloc_zero_size(NULL, sizeof(char) * str_length * 4 + 1);

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
		char * tmp = handlebars_talloc_strndup(NULL, new_str, new_str_length + 1);
		handlebars_talloc_free(new_str);
		new_str = tmp;
	}
	return new_str;
}

void handlebars_stripcslashes(char * str, size_t * length)
{
	char *source, *target, *end;
	size_t nlen = (length == NULL ? strlen(str) : *length);
	size_t i;
	char numtmp[4];
	
	source = str;
	end = source + nlen;
	target = str;
	for ( ; source < end; source++ ) {
		if (*source == '\\' && source + 1 < end) {
			source++;
			switch (*source) {
				case 'n':  *target++='\n'; nlen--; break;
				case 'r':  *target++='\r'; nlen--; break;
				case 'a':  *target++='\a'; nlen--; break;
				case 't':  *target++='\t'; nlen--; break;
				case 'v':  *target++='\v'; nlen--; break;
				case 'b':  *target++='\b'; nlen--; break;
				case 'f':  *target++='\f'; nlen--; break;
				case '\\': *target++='\\'; nlen--; break;
				case 'x':
					if (source+1 < end && isxdigit((int)(*(source+1)))) {
						numtmp[0] = *++source;
						if (source+1 < end && isxdigit((int)(*(source+1)))) {
							numtmp[1] = *++source;
							numtmp[2] = '\0';
							nlen-=3;
						} else {
							numtmp[1] = '\0';
							nlen-=2;
						}
						*target++=(char)strtol(numtmp, NULL, 16);
						break;
					}
					/* break is left intentionally */
				default:
					i=0;
					while (source < end && *source >= '0' && *source <= '7' && i<3) {
						numtmp[i++] = *source++;
					}
					if (i) {
						numtmp[i]='\0';
						*target++=(char)strtol(numtmp, NULL, 8);
						nlen-=i;
						source--;
					} else {
						*target++=*source;
						nlen--;
					}
			}
		} else {
			*target++=*source;
		}
	}

	if (nlen != 0) {
		*target='\0';
	}
	
	if( length != NULL ) {
		*length = nlen;
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
	context->errnum = HANDLEBARS_PARSEERR;
  context->error = handlebars_talloc_strdup(context, err);
  context->errloc = handlebars_talloc_zero(context, YYLTYPE);
  memcpy(context->errloc, lloc, sizeof(YYLTYPE));
}

void handlebars_yy_fatal_error(const char * msg, void * yyscanner)
{
  fprintf(stderr, "%s\n", msg);
  exit(2);
}

void handlebars_yy_print(FILE *file, int type, YYSTYPE value)
{
	fprintf(stderr, "%d : \n", type);
}

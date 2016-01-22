
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

char * handlebars_addcslashes_ex(const char * str, size_t str_length, const char * what, size_t what_length)
{
    char flags[256];
    char * source;
    char * target;
    char * end;
    size_t i;
    char * new_str;
    size_t new_str_length;

    assert(str != NULL);
    
    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    new_str = handlebars_talloc_zero_size(NULL, sizeof(char) * str_length * 4 + 1);
    if( unlikely(new_str == NULL) ) {
        return NULL;
    }

    source = (char *) str;
    target = new_str;
    end = source + str_length;
    for( ; source < end ; source++ ) {
        char c = *source;
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

char * handlebars_htmlspecialchars(const char * str)
{
    const char * flags[256];
    size_t newsize = 0;
    const char * p;
    char * newstr;
    char * r;
    size_t tmp;

    // Build map
    memset(flags, 0, sizeof(flags));
    flags['&'] = "&amp;";
    flags['"'] = "&quot;";
    flags['\''] = "&#039;";
    flags['<'] = "&lt;";
    flags['>'] = "&gt;";

    // Estimate new size
    for( p = str; *p != NULL; p++ ) {
        if( flags[*p] != NULL ) {
            newsize += strlen(flags[*p]);
        } else {
            newsize++;
        }
    }

    // Alloc new string
    r = newstr = handlebars_talloc_array(NULL, char, newsize + 1);
    memset(newstr, 0, newsize + 1);

    // Copy
    for( p = str; *p != NULL; p++ ) {
        if( flags[*p] != NULL ) {
            tmp = strlen(flags[*p]);
            memcpy(r, flags[*p], tmp);
            r += tmp;
        } else {
            *r = *p;
            r++;
        }
    }

    r = '\0';

    return newstr;
}

char * handlebars_implode(const char * sep, const char ** arr)
{
    const char ** ptr = arr;
    char * val;
    
    if( *ptr ) {
        val = handlebars_talloc_strdup(NULL, *ptr++);
    } else {
        return handlebars_talloc_strdup(NULL, "");
    }
    while( *ptr++ ) {
        val = handlebars_talloc_asprintf_append(val, "%s%s", sep, *ptr);
    }
    return val;
}

char * handlebars_ltrim_ex(char * string, size_t * length, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * ptr;
    size_t len = length ? *length : strlen(string);

    assert(string != NULL);
    
    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    ptr = string;
    while( *ptr && flags[(unsigned char) *ptr] ) {
        ++ptr;
        --len;
    }

    if( ptr > string ) {
        memmove(string, ptr, len + 1);
    }
    
    if( length ) {
        *length = len;
    }

    return string;
}

char * handlebars_rtrim_ex(char * string, size_t * length, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * original;
    size_t len = length ? *length : strlen(string);
    
    assert(string != NULL);
    
    if( unlikely(len <= 0) ) {
        return string;
    }
    
    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }
    
    original = string + len;
    while(original > string && flags[(unsigned char) *--original]) {
        --len;
        *original = '\0';
    }
    
    if( length ) {
        *length = len;
    }
    
    return string;
}

char * handlebars_stripcslashes_ex(char * str, size_t * length)
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
                /* no break */
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
        *target = '\0';
    }
    
    if( length != NULL ) {
        *length = nlen;
    }
    
    return str;
}

char * handlebars_str_reduce(char * string, const char * substr, const char * replacement) {
    char * tok;
	char * orig = string;
	int replacement_len = strlen(replacement);
	int substr_len = strlen(substr);
	int string_len = strlen(string);
	
	assert(replacement_len <= substr_len);
	assert(substr_len > 0);
	
	if( replacement_len > substr_len ) {
	    return NULL;
	} else if( substr_len <= 0 || string_len <= 0 ) {
	    return string;
	}
	
	tok = string;
	int counter = 0;
	while( (tok = strstr(tok, substr)) ) {
	    assert(++counter < 1000);
	//while( (tok = strstr(string, substr)) ) {
		memcpy(tok, replacement, replacement_len);
		memcpy(tok + replacement_len, tok + substr_len, string_len - substr_len - (tok - string));
		memset(string + string_len - substr_len + replacement_len, 0, 1);
	}
	
	return orig;
}


void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_context * context)
{
    int numBytesToRead = maxBytesToRead;
    int bytesRemaining = strlen(context->tmpl) - context->tmplReadOffset;
    int i;
    if( numBytesToRead > bytesRemaining ) {
        numBytesToRead = bytesRemaining;
    }
    for( i = 0; i < numBytesToRead; i++ ) {
        buffer[i] = context->tmpl[context->tmplReadOffset+i];
    }
    *numBytesRead = numBytesToRead;
    context->tmplReadOffset += numBytesToRead;
}

void handlebars_yy_error(struct handlebars_locinfo * lloc, struct handlebars_context * context, const char * err)
{
    assert(context != NULL);

#if defined(YYDEBUG) && YYDEBUG
    fprintf(stderr, "%d : %s\n", lloc->first_line, err);
#endif

    context->errnum = HANDLEBARS_PARSEERR;
    context->error = handlebars_talloc_strdup(context, err);
    context->errloc = handlebars_talloc_zero(context, handlebars_locinfo);
    if( likely(context->errloc != NULL) ) {
        memcpy(context->errloc, lloc, sizeof(handlebars_locinfo));
    }
}

void handlebars_yy_fatal_error(const char * msg, HANDLEBARS_ATTR_UNUSED void * yyscanner)
{
    // Exit
    fprintf(stderr, "%s\n", msg);
    handlebars_exit(2);
}

void handlebars_yy_print(FILE *file, int type, HANDLEBARS_ATTR_UNUSED YYSTYPE value)
{
    fprintf(file, "%d : \n", type);
}


// Allocators for a reentrant scanner (flex)

void * handlebars_yy_alloc(size_t bytes, void * yyscanner)
{
    // Note: it looks like the yyscanner is allocated before we can pass in
    // a handlebars context...
    // Also look into the performance hit for doing this
    struct handlebars_context * ctx = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_context_init_current);
    return (void *) _handlebars_yy_alloc(ctx, bytes, "handlebars_yy_alloc");
}

void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner)
{
    // Going to skip wrappers for now
    struct handlebars_context * ctx = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_context_init_current);
    return (void *) _handlebars_yy_realloc(ctx, ptr, sizeof(char), bytes, "handlebars_yy_realloc");
}

void handlebars_yy_free(void * ptr, HANDLEBARS_ATTR_UNUSED void * yyscanner)
{
    // Going to skip wrappers for now
    _handlebars_yy_free(ptr, "handlebars_yy_free");
}

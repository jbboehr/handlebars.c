
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
#include "handlebars_context.h"
#include "handlebars_memory.h"
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
    if( new_str == NULL ) {
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
    
    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }
    
    original = string + len;
    while(original >= string && flags[(unsigned char) *--original]) {
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
    
    return str;
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
    return (void *) handlebars_talloc_size(ctx, bytes);
}

void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner)
{
    struct handlebars_context * ctx = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_context_init_current);
    return (void *) handlebars_talloc_realloc(ctx, ptr, char, bytes);
}

void handlebars_yy_free(void * ptr, HANDLEBARS_ATTR_UNUSED void * yyscanner)
{
    handlebars_talloc_free(ptr);
}

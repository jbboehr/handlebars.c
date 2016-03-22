
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
#include "handlebars_memory.h"
#include "handlebars_private.h"

#include "handlebars_ast.h"
#include "handlebars_string.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"



extern struct handlebars_parser * _handlebars_parser_init_current;

struct htmlspecialchars_pair {
    const char * str;
    size_t len;
};

static const struct htmlspecialchars_pair htmlspecialchars[256] = {
    ['&']  = {HBS_STRL("&amp;")},
    ['"']  = {HBS_STRL("&quot;")},
    ['\''] = {HBS_STRL("&#x27;")}, //"&#039;"
    ['<']  = {HBS_STRL("&lt;")},
    ['>']  = {HBS_STRL("&gt;")},
    ['`']  = {HBS_STRL("&#x60;")},
};

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

char * handlebars_htmlspecialchars_append_buffer(char * buf, const char * str, size_t len)
{
    size_t orig_size;
    size_t new_len = len;
    char * out;
    const char * p;
    const struct htmlspecialchars_pair * pair;

    if( len <= 0 ) {
        return buf;
    }

    // Calculate new size
    for( p = str + len - 1; p >= str; p-- ) {
        pair = &htmlspecialchars[(unsigned char) *p];
        if( pair->len ) {
            new_len += pair->len - 1; // @todo check if sizeof works
        }
    }

    // If new size is equal to len, nothing to escape, just append
    if( new_len == len ) {
        return handlebars_talloc_strndup_append_buffer(buf, str, len);
    }

    // Realloc original buffer
    orig_size = talloc_array_length(buf);
    buf = talloc_realloc_size(NULL, buf, orig_size + new_len);

    // Copy
    out = buf + orig_size - 1;
    for( p = str; *p; p++ ) {
        pair = &htmlspecialchars[(unsigned char) *p];
        if( pair->len ) {
            memcpy(out, pair->str, pair->len);
            out += pair->len;
        } else {
            *out++ = *p;
        }
    }

    *out = '\0';

    return buf;
}

char * handlebars_htmlspecialchars(const char * str)
{
    char * buf = handlebars_talloc_strdup(NULL, "");
    return handlebars_htmlspecialchars_append_buffer(buf, str, strlen(str));
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

char * handlebars_indent(void * ctx, const char * str, const char * indent)
{
    size_t len, i;
    char * out = handlebars_talloc_strdup(ctx, "");
    bool endsInLine;
    size_t indent_len = strlen(indent);

    if( !str ) {
        return out;
    }

    out = handlebars_talloc_strndup_append_buffer(out, indent, indent_len);
    len = strlen(str);
    endsInLine = (str[len - 1] == '\n');
    if( endsInLine ) {
        len--;
    }

    if( !len ) {
        return out;
    }

    for( i = 0; i < len; i++ ) {
        if( str[i] == '\n' ) {
            out = handlebars_talloc_strndup_append_buffer(out, HBS_STRL("\n"));
            out = handlebars_talloc_strndup_append_buffer(out, indent, indent_len);
        } else {
            char tmp[2];
            tmp[0] = str[i];
            tmp[1] = 0;
            out = handlebars_talloc_strndup_append_buffer(out, tmp, 1);
        }
    }

    if( endsInLine ) {
        out = handlebars_talloc_strndup_append_buffer(out, HBS_STRL("\n"));
    }

    return out;
}

struct handlebars_string * handlebars_ltrim(struct handlebars_string * string, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * ptr;

    assert(string != NULL);

    if( unlikely(string == NULL || string->len <= 0) ) {
        return string;
    }

    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }

    ptr = string->val;
    while( *ptr && flags[(unsigned char) *ptr] ) {
        ++ptr;
        --string->len;
    }

    if( ptr > string->val ) {
        memmove(string->val, ptr, string->len + 1);
    }

    return string;
}

struct handlebars_string * handlebars_rtrim(struct handlebars_string * string, const char * what, size_t what_length)
{
    size_t i;
    char flags[256];
    char * original;
    
    assert(string != NULL);
    
    if( unlikely(string == NULL || string->len <= 0) ) {
        return string;
    }
    
    // Make char mask
    memset(flags, 0, sizeof(flags));
    for( i = 0; i < what_length; i++ ) {
        flags[(unsigned char) what[i]] = 1;
    }
    
    original = string->val + string->len;
    while( original > string->val && flags[(unsigned char) *--original] ) {
        --string->len;
        *original = '\0';
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
#ifndef NDEBUG
    int counter = 0;
#endif
	
	assert(replacement_len <= substr_len);
	assert(substr_len > 0);
	
	if( replacement_len > substr_len ) {
	    return NULL;
	} else if( substr_len <= 0 || string_len <= 0 ) {
	    return string;
	}
	
	tok = string;
	while( (tok = strstr(tok, substr)) ) {
	    assert(++counter < 1000);
	//while( (tok = strstr(string, substr)) ) {
		memcpy(tok, replacement, replacement_len);
		memcpy(tok + replacement_len, tok + substr_len, string_len - substr_len - (tok - string));
		memset(string + string_len - substr_len + replacement_len, 0, 1);
	}
	
	return orig;
}

#undef CONTEXT
#define CONTEXT HBSCTX(parser)

void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_parser * parser)
{
    struct handlebars_string * tmpl = parser->tmpl;
    const char * val = (const char *) tmpl->val;

    int numBytesToRead = maxBytesToRead;
    int bytesRemaining = tmpl->len - parser->tmplReadOffset;
    int i;
    if( numBytesToRead > bytesRemaining ) {
        numBytesToRead = bytesRemaining;
    }
    for( i = 0; i < numBytesToRead; i++ ) {
        buffer[i] = val[parser->tmplReadOffset+i];
    }
    *numBytesRead = numBytesToRead;
    parser->tmplReadOffset += numBytesToRead;
}

void handlebars_yy_error(struct handlebars_locinfo * lloc, struct handlebars_parser * parser, const char * err)
{
    assert(parser != NULL);

#if defined(YYDEBUG) && YYDEBUG
    fprintf(stderr, "%d : %s\n", lloc->first_line, err);
#endif

    handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, lloc, err);
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
    struct handlebars_parser * parser = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_parser_init_current);
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_counter_incr();
#endif
    return _handlebars_yy_alloc(parser, bytes);
}

void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner)
{
    // Going to skip wrappers for now
    struct handlebars_parser * parser = (yyscanner ? handlebars_yy_get_extra(yyscanner) : _handlebars_parser_init_current);
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_counter_incr();
#endif
    return _handlebars_yy_realloc(parser, ptr, sizeof(char) * bytes);
}

void handlebars_yy_free(void * ptr, HANDLEBARS_ATTR_UNUSED void * yyscanner)
{
    // Going to skip wrappers for now
    _handlebars_yy_free(ptr);
}

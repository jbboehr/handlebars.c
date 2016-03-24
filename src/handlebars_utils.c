
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


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

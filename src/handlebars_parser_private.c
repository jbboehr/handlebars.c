/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

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
#include "handlebars_memory.h"
#include "handlebars_parser.h"
#include "handlebars_private.h"
#include "handlebars_string.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wredundant-decls"
#include "handlebars_parser_private.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#pragma GCC diagnostic pop

#undef CONTEXT
#define CONTEXT HBSCTX(parser)

#ifndef TLS
#define TLS
#endif



HBS_TEST_PUBLIC TLS struct handlebars_parser * handlebars_parser_init_current;

void handlebars_yy_input(char * buffer, int *numBytesRead, int maxBytesToRead, struct handlebars_parser * parser)
{
    struct handlebars_string * tmpl = parser->tmpl;
    const char * val = (const char *) hbs_str_val(tmpl);

    int numBytesToRead = maxBytesToRead;
    int bytesRemaining = hbs_str_len(tmpl) - parser->tmplReadOffset;
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

#if YYDEBUG
    fprintf(stderr, "%d : %s\n", lloc->first_line, err);
#endif

    handlebars_throw_ex(CONTEXT, HANDLEBARS_PARSEERR, lloc, "%s", err);
}

void handlebars_yy_fatal_error(const char * msg, struct handlebars_parser * parser)
{
    assert(parser != NULL);

#if YYDEBUG
    fprintf(stderr, "%s\n", msg);
#endif

    handlebars_throw(CONTEXT, HANDLEBARS_PARSEERR, "%s", msg);
}

void handlebars_yy_print(FILE *file, int type, HBS_ATTR_UNUSED YYSTYPE value)
{
    fprintf(file, "%d : \n", type);
}


// Allocators for a reentrant scanner (flex)

void * handlebars_yy_alloc(size_t bytes, void * yyscanner)
{
    // Note: it looks like the yyscanner is allocated before we can pass in
    // a handlebars context...
    struct handlebars_parser * parser = (yyscanner ? handlebars_yy_get_extra(yyscanner) : handlebars_parser_init_current);
    assert(parser != NULL);
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_counter_incr();
#endif
    void * retval = handlebars_talloc_size(parser, bytes);
    if (unlikely(retval == NULL)) {
        handlebars_throw(HBSCTX(parser), HANDLEBARS_NOMEM, "Out of memory");
    }
    return retval;
}

void * handlebars_yy_realloc(void * ptr, size_t bytes, void * yyscanner)
{
    // Going to skip wrappers for now
    struct handlebars_parser * parser = (yyscanner ? handlebars_yy_get_extra(yyscanner) : handlebars_parser_init_current);
    assert(parser != NULL);
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_counter_incr();
#endif
    void * retval = handlebars_talloc_realloc_size(parser, ptr, sizeof(char) * bytes);
    if (unlikely(retval == NULL)) {
        handlebars_throw(HBSCTX(parser), HANDLEBARS_NOMEM, "Out of memory");
    }
    return retval;
}

void handlebars_yy_free(void * ptr, HBS_ATTR_UNUSED void * yyscanner)
{
    // Going to skip wrappers for now
    handlebars_talloc_free(ptr);
}

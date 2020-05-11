/**
 * Copyright (C) 2016 John Boehr
 *
 * This file is part of handlebars.c.
 *
 * handlebars.c is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * handlebars.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with handlebars.c.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



#if defined(YYDEBUG)
#define append(str, len) fprintf(stderr, "Delimiter preprocessor: appending: \"%.*s\"\n", len, str); \
    new_tmpl = handlebars_string_append(ctx, new_tmpl, str, len)
#define move_forward(x) \
    if( x > i ) { \
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failed to advanced scanner by %ld", x); \
    } \
    fprintf(stderr, "Delimiter preprocessor: moving forward %ld characters, new position: \"%c\"\n", x, *(p + x)); \
    p += x; \
    i -= x
#else
#define append(str, len) new_tmpl = handlebars_string_append(ctx, new_tmpl, str, len)
#define move_forward(x) \
    if( x > i ) { \
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failed to advanced scanner by %lu", (unsigned long) x); \
    } \
    p += x; \
    i -= x
#endif

static const char placeholder[] = "{{! delimiter placeholder }}";

struct handlebars_string * handlebars_preprocess_delimiters(
    struct handlebars_context * ctx,
    struct handlebars_string * tmpl,
    struct handlebars_string * open,
    struct handlebars_string * close
) {
    register long i = hbs_str_len(tmpl);
    register const char *p = hbs_str_val(tmpl);

    struct handlebars_string * new_open = NULL;
    struct handlebars_string * new_close = NULL;
    struct handlebars_string * new_tmpl = handlebars_string_init(ctx, hbs_str_len(tmpl));
    int state = 0;
    const char *po = NULL;
    const char *pc = NULL;
    const char *pce = NULL;
    int starts_with_bracket = 0;

    // Initialize/duplicate open/close
    if( open == NULL ) {
        open = handlebars_string_ctor(ctx, HBS_STRL("{{"));
    } else {
        open = handlebars_string_copy_ctor(ctx, open);
        starts_with_bracket = hbs_str_val(open)[0] == '{';
    }
    if( close == NULL ) {
        close = handlebars_string_ctor(ctx, HBS_STRL("}}"));
    } else {
        close = handlebars_string_copy_ctor(ctx, close);
    }

    for( ; i > 0; i--, p++ ) {
        switch( state ) {
            default: // Default
            case 0: state0:
                // If current character is a slash, skip one character
                if( *p == '\\' ) {
                    append(p, 1);
                    move_forward(1);
                    append(p, 1);
                    continue;
                }

                // Remaining size needs to be at least:
                // hbs_str_len(open) + hbs_str_len(close) + 2 (for equals) + 1 (minimum delimiter size) + 1 (space in the middle)
                if( i >= hbs_str_len(open) + hbs_str_len(close) + 4 && strncmp(p, hbs_str_val(open), hbs_str_len(open)) == 0 && *(p + hbs_str_len(open)) == '=' ) {
                    // We are going into a delimiter switch
                    state = 1; goto state1;
                }

                // Remaining size needs to be at least:
                // hbs_str_len(open) + hbs_str_len(close) + 1
                if( i >= hbs_str_len(open) + hbs_str_len(close) + 1 && strncmp(p, hbs_str_val(open), hbs_str_len(open)) == 0 ) {
                    // We are going into a regular tag
                    append("{{", 2);
                    move_forward(hbs_str_len(open));
                    state = 2; goto state2;
                }

                // Escape the bracket if our current custom delims aren't brackets
                if( i >= 2 && *p == '{' && *(p + 1) == '{' && !starts_with_bracket ) {
                    append("\\", 1);
                }

                // This is an escape
                append(p, 1);
                break;
            case 1: state1: // In delimiter switch
                // Scan past open tag and equals
                move_forward(hbs_str_len(open) + 1);

                // Scan past any whitespace
                for( ; i > 0; i--, p++ ) {
                    if( *p != ' ' ) {
                        break;
                    }
                }

                // Mark beginning of open tag
                po = p;

                // Look for a space
                for( ; i > 0; i--, p++ ) {
                    if( *p == ' ' ) {
                        break;
                    }
                }

                // Not found
                if( i == 0 ) {
                    handlebars_throw(ctx, HANDLEBARS_ERROR, "Delimiter change must contain a space");
                }

                // Save new open tag
                new_open = handlebars_string_ctor(ctx, po, p - po);

                // Scan past any whitespace
                for( ; i > 0; i--, p++ ) {
                    if (*p != ' ') {
                        break;
                    }
                }
                pc = p;

                // Look for another equals
                for( ; i > 0; i--, p++ ) {
                    if( *p == '=' ) {
                        break;
                    }
                }

                // Not found
                if( i == 0 ) {
                    handlebars_throw(ctx, HANDLEBARS_ERROR, "Delimiter change must contain two equals");
                }

                // Scan backwards while whitespace
                pce = p - 1;
                while( *pce == ' ' ) {
                    pce--;
                }

                // Save new close tag
                new_close = handlebars_string_ctor(ctx, pc, pce - pc + 1);

                // Skip over equals
                move_forward(1);

                // The next sequence must be the closing delimiter, or error
                if( i < hbs_str_len(close) || strncmp(p, hbs_str_val(close), hbs_str_len(close)) ) {
                    handlebars_throw(ctx, HANDLEBARS_ERROR, "Delimiter change must end with an equals");
                }

                // Skip over close tag
                move_forward(hbs_str_len(close));

                // Swap
                handlebars_talloc_free(open);
                handlebars_talloc_free(close);
                open = new_open;
                close = new_close;
                starts_with_bracket = hbs_str_val(open)[0] == '{';

#if defined(YYDEBUG)
                fprintf(stderr, "Delimiter preprocessor: New delimiters: \"%.*s\", \"%.*s\"\n", (int) hbs_str_len(open), hbs_str_val(open), (int) hbs_str_len(close), hbs_str_val(close));
#endif

                // Append a comment - tricks whitespace rules into working
                append(placeholder, (int) sizeof(placeholder) - 1);

                // Goto new state
                if( i > 0 ) {
                    state = 0;
                    goto state0;
                }
            case 2: state2: // In regular tag
                if( i >= hbs_str_len(close) && strncmp(p, hbs_str_val(close), hbs_str_len(close)) == 0 ) {
                    // Ending
                    append("}}", 2);
                    move_forward(hbs_str_len(close));
                    if( i > 0 ) {
                        state = 0;
                        goto state0;
                    }
                } else {
                    append(p, 1);
                }
                break;
        }
    }

    // Free open/close
    handlebars_talloc_free(open);
    handlebars_talloc_free(close);

    HANDLEBARS_MEMCHECK(new_tmpl, ctx);
    return new_tmpl;
}

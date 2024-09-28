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
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_delimiters.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_string.h"



#if defined(YYDEBUG)
#define append(str, len) fprintf(stderr, "Delimiter preprocessor: appending: \"%.*s\"\n", (int) len, str); \
    new_tmpl = handlebars_string_append(ctx, new_tmpl, str, len)
#define move_forward(x) \
    if( (ssize_t) x > (ssize_t) i ) { \
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failed to advanced scanner by %zd", (ssize_t) x); \
    } \
    fprintf(stderr, "Delimiter preprocessor: moving forward %zd characters, new position: \"%c\"\n", (ssize_t) x, *(p + x)); \
    p += x; \
    i -= x
#else
#define append(str, len) new_tmpl = handlebars_string_append(ctx, new_tmpl, str, len)
#define move_forward(x) \
    if( (ssize_t) x > (ssize_t) i ) { \
        handlebars_throw(ctx, HANDLEBARS_ERROR, "Failed to advanced scanner by %zd", (ssize_t) x); \
    } \
    p += x; \
    i -= x
#endif

struct handlebars_string * handlebars_preprocess_delimiters(
    struct handlebars_context * ctx,
    struct handlebars_string * tmpl,
    struct handlebars_string * open,
    struct handlebars_string * close
) {
    register ssize_t i = hbs_str_len(tmpl);
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
                if( (size_t) i >= hbs_str_len(open) + hbs_str_len(close) + 4 && strncmp(p, hbs_str_val(open), hbs_str_len(open)) == 0 && *(p + hbs_str_len(open)) == '=' ) {
                    // We are going into a delimiter switch
                    state = 1; goto state1;
                }

                // Remaining size needs to be at least:
                // hbs_str_len(open) + hbs_str_len(close) + 1
                if( (size_t) i >= hbs_str_len(open) + hbs_str_len(close) + 1 && strncmp(p, hbs_str_val(open), hbs_str_len(open)) == 0 ) {
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
                if( i <= 0 ) {
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
                if( i <= 0 ) {
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
                assert(i >= 0);
                if( (size_t) i < hbs_str_len(close) || strncmp(p, hbs_str_val(close), hbs_str_len(close)) ) {
                    handlebars_throw(ctx, HANDLEBARS_ERROR, "Delimiter change must end with an equals");
                }

                // Skip over close tag
                move_forward(hbs_str_len(close));

                // Swap
                handlebars_talloc_free(open);
                handlebars_talloc_free(close);
                open = new_open;
                close = new_close;
                new_open = new_close = NULL;
                starts_with_bracket = hbs_str_val(open)[0] == '{';

#if defined(YYDEBUG)
                fprintf(stderr, "Delimiter preprocessor: New delimiters: \"%.*s\", \"%.*s\"\n", (int) hbs_str_len(open), hbs_str_val(open), (int) hbs_str_len(close), hbs_str_val(close));
#endif

                // Append a special mustache tag
                do {
                    struct handlebars_string *open_escaped = handlebars_string_addcslashes(ctx, open, HBS_STRL("\""));
                    struct handlebars_string *close_escaped = handlebars_string_addcslashes(ctx, close, HBS_STRL("\""));
                    new_tmpl = handlebars_string_append(ctx, new_tmpl, HBS_STRL("{{hbsc_set_delimiters \""));
                    new_tmpl = handlebars_string_append_str(ctx, new_tmpl, open_escaped);
                    new_tmpl = handlebars_string_append(ctx, new_tmpl, HBS_STRL("\" \""));
                    new_tmpl = handlebars_string_append_str(ctx, new_tmpl, close_escaped);
                    new_tmpl = handlebars_string_append(ctx, new_tmpl, HBS_STRL("\"}}"));
                    handlebars_string_delref(open_escaped);
                    handlebars_string_delref(close_escaped);
                } while (0);

                // Goto new state
                if( i > 0 ) {
                    state = 0;
                    goto state0;
                }
                break; // FEAR
            case 2: state2: // In regular tag
                assert(i >= 0);
                if( (size_t) i >= hbs_str_len(close) && strncmp(p, hbs_str_val(close), hbs_str_len(close)) == 0 ) {
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

#if defined(YYDEBUG)
    fprintf(stderr, "Delimiter preprocessor: Processed template: %.*s\n", (int) hbs_str_len(new_tmpl), hbs_str_val(new_tmpl));
#endif

    // Free open/close
    handlebars_talloc_free(open);
    handlebars_talloc_free(close);
    handlebars_string_delref(tmpl);

    HANDLEBARS_MEMCHECK(new_tmpl, ctx);
    return new_tmpl;
}

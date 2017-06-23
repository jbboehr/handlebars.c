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

struct delimiter_scanner {

};

struct handlebars_string * handlebars_preprocess_delimiters(
    struct handlebars_context * ctx,
    struct handlebars_string * tmpl,
    struct handlebars_string * open,
    struct handlebars_string * close
) {
    register size_t i = tmpl->len;
    register const char *p = tmpl->val;
    register const char *end = tmpl->val + tmpl->len;
    register short is_delim = 0;

    for( ; i > 0; i--, p++ ) {
        // Remaining size needs to be at least:
        // open->len + close->len + 2 (for equals) + 1 (minimum delimiter size) + 1 (space in the middle)
        if( i >= open->len + close->len + 4 && strncmp(p, open->val, open->len) == 0 ) {
            // Found open

            // Scan past open tag
            p += open->len;
            i -= open->len;

            // If the next character is an equals
            if( *p == '=' ) {
                // Look for a space
                for( ; i > 0; i--, p++ ) {
                    if( *p == ' ' ) {
                        // @todo
                        break;
                    }
                }

                // Look for another equals
                for( ; i > 0; i--, p++ ) {
                    if( *p == '=' ) {
                        // @todo
                        break;
                    }
                }

                // The next sequence must be the closing delimiter, or error
                if( i < close->len || strncmp(p, close->val, close->len) ) {
                    // @todo error
                }
            } else {
                // Look for close
                for( ; i > 0; i--, p++ ) {
                    // Remaining size needs to be at least close->len
                    if( i < close->len ) {
                        break;
                    } else if( strncmp(p, close->val, close->len) == 0 ) {
                        // Scan past close
                        p += close->len;
                        i -= close->len;
                        break;
                    }

                }
            }

        }
    }
}

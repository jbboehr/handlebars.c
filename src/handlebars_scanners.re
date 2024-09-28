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
#include <stdlib.h>

#include "handlebars_scanners.h"

#define YYCTYPE unsigned char

bool handlebars_scanner_next_whitespace(const char * s, bool def)
{
    const YYCTYPE * YYCURSOR = (const unsigned char *) s;

    assert(s != NULL);

    for (;;) {
        /*!re2c
            re2c:yyfill:enable = 0;
            re2c:indent:string = '    ';

            "\x00"       { break; }
            [ \v\t\r]    { continue; }
            "\n"         { return 1; }
            .            { return 0; }
        */
    }

    return def;
}

bool handlebars_scanner_prev_whitespace(const char * s, bool def)
{
    const YYCTYPE * YYCURSOR = (const unsigned char *) s;
    short match = def;

    assert(s != NULL);

    for (;;) {
        /*!re2c
            re2c:yyfill:enable = 0;
            re2c:indent:string = '    ';

            "\x00"       { break; }
            [ \v\t\r]    { continue; }
            "\n"         { match = 1; continue; }
            .            { match = 0; continue; }
        */
    }

    return match;
}

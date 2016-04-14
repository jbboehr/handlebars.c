
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

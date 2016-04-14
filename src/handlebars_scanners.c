/* Generated by re2c 0.16 */
#line 1 "handlebars_scanners.re"

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
        
#line 24 "handlebars_scanners.c"
{
    YYCTYPE yych;
    yych = *YYCURSOR;
    if (yych <= ' ') {
        if (yych <= '\n') {
            if (yych <= 0x00) goto yy3;
            if (yych <= 0x08) goto yy5;
            if (yych <= '\t') goto yy7;
            goto yy9;
        } else {
            if (yych <= '\f') {
                if (yych <= '\v') goto yy7;
                goto yy5;
            } else {
                if (yych <= '\r') goto yy7;
                if (yych <= 0x1F) goto yy5;
                goto yy7;
            }
        }
    } else {
        if (yych <= 0xE0) {
            if (yych <= 0x7F) goto yy5;
            if (yych <= 0xC1) goto yy2;
            if (yych <= 0xDF) goto yy11;
            goto yy12;
        } else {
            if (yych <= 0xF0) {
                if (yych <= 0xEF) goto yy13;
                goto yy14;
            } else {
                if (yych <= 0xF3) goto yy15;
                if (yych <= 0xF4) goto yy16;
            }
        }
    }
yy2:
yy3:
    ++YYCURSOR;
#line 24 "handlebars_scanners.re"
    { break; }
#line 65 "handlebars_scanners.c"
yy5:
    ++YYCURSOR;
#line 27 "handlebars_scanners.re"
    { return 0; }
#line 70 "handlebars_scanners.c"
yy7:
    ++YYCURSOR;
#line 25 "handlebars_scanners.re"
    { continue; }
#line 75 "handlebars_scanners.c"
yy9:
    ++YYCURSOR;
#line 26 "handlebars_scanners.re"
    { return 1; }
#line 80 "handlebars_scanners.c"
yy11:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy2;
    if (yych <= 0xBF) goto yy5;
    goto yy2;
yy12:
    yych = *++YYCURSOR;
    if (yych <= 0x9F) goto yy2;
    if (yych <= 0xBF) goto yy11;
    goto yy2;
yy13:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy2;
    if (yych <= 0xBF) goto yy11;
    goto yy2;
yy14:
    yych = *++YYCURSOR;
    if (yych <= 0x8F) goto yy2;
    if (yych <= 0xBF) goto yy13;
    goto yy2;
yy15:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy2;
    if (yych <= 0xBF) goto yy13;
    goto yy2;
yy16:
    ++YYCURSOR;
    if ((yych = *YYCURSOR) <= 0x7F) goto yy2;
    if (yych <= 0x8F) goto yy13;
    goto yy2;
}
#line 28 "handlebars_scanners.re"

    }
    
    return def;
}

bool handlebars_scanner_prev_whitespace(const char * s, bool def)
{
    const YYCTYPE * YYCURSOR = (const unsigned char *) s;
    short match = def;
    
    assert(s != NULL);
    
    for (;;) {
        
#line 128 "handlebars_scanners.c"
{
    YYCTYPE yych;
    yych = *YYCURSOR;
    if (yych <= ' ') {
        if (yych <= '\n') {
            if (yych <= 0x00) goto yy20;
            if (yych <= 0x08) goto yy22;
            if (yych <= '\t') goto yy24;
            goto yy26;
        } else {
            if (yych <= '\f') {
                if (yych <= '\v') goto yy24;
                goto yy22;
            } else {
                if (yych <= '\r') goto yy24;
                if (yych <= 0x1F) goto yy22;
                goto yy24;
            }
        }
    } else {
        if (yych <= 0xE0) {
            if (yych <= 0x7F) goto yy22;
            if (yych <= 0xC1) goto yy19;
            if (yych <= 0xDF) goto yy28;
            goto yy29;
        } else {
            if (yych <= 0xF0) {
                if (yych <= 0xEF) goto yy30;
                goto yy31;
            } else {
                if (yych <= 0xF3) goto yy32;
                if (yych <= 0xF4) goto yy33;
            }
        }
    }
yy19:
yy20:
    ++YYCURSOR;
#line 46 "handlebars_scanners.re"
    { break; }
#line 169 "handlebars_scanners.c"
yy22:
    ++YYCURSOR;
#line 49 "handlebars_scanners.re"
    { match = 0; continue; }
#line 174 "handlebars_scanners.c"
yy24:
    ++YYCURSOR;
#line 47 "handlebars_scanners.re"
    { continue; }
#line 179 "handlebars_scanners.c"
yy26:
    ++YYCURSOR;
#line 48 "handlebars_scanners.re"
    { match = 1; continue; }
#line 184 "handlebars_scanners.c"
yy28:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy19;
    if (yych <= 0xBF) goto yy22;
    goto yy19;
yy29:
    yych = *++YYCURSOR;
    if (yych <= 0x9F) goto yy19;
    if (yych <= 0xBF) goto yy28;
    goto yy19;
yy30:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy19;
    if (yych <= 0xBF) goto yy28;
    goto yy19;
yy31:
    yych = *++YYCURSOR;
    if (yych <= 0x8F) goto yy19;
    if (yych <= 0xBF) goto yy30;
    goto yy19;
yy32:
    yych = *++YYCURSOR;
    if (yych <= 0x7F) goto yy19;
    if (yych <= 0xBF) goto yy30;
    goto yy19;
yy33:
    ++YYCURSOR;
    if ((yych = *YYCURSOR) <= 0x7F) goto yy19;
    if (yych <= 0x8F) goto yy30;
    goto yy19;
}
#line 50 "handlebars_scanners.re"

    }
    
    return match;
}

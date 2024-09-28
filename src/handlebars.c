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
#include "handlebars_ast.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"



int handlebars_version(void) {
    return HANDLEBARS_VERSION_PATCH
        + (HANDLEBARS_VERSION_MINOR * 100)
        + (HANDLEBARS_VERSION_MAJOR * 10000);
}

const char * handlebars_version_string(void) {
    return HANDLEBARS_VERSION_STRING;
}

const char * handlebars_spec_version_string(void)
{
    return HANDLEBARS_SPEC_VERSION_STRING;
}

const char * handlebars_mustache_spec_version_string(void)
{
    return MUSTACHE_SPEC_VERSION_STRING;
}

struct handlebars_context * handlebars_context_ctor_ex(void * ctx)
{
    struct handlebars_context * context = NULL;

    // Allocate struct as new top level talloc context
    context = handlebars_talloc_zero(ctx, struct handlebars_context);
    if( unlikely(context == NULL) ) {
        // Mainly doing this for consistency with lex init
        errno = ENOMEM;
        return NULL;
    }

    context->e = handlebars_talloc_zero(context, struct handlebars_error);
    if( unlikely(context->e == NULL) ) {
        handlebars_talloc_free(context);
        errno = ENOMEM;
        context = NULL;
    }

    return context;
}

struct handlebars_context * handlebars_context_ctor(void) {
    return handlebars_context_ctor_ex(NULL);
}

void handlebars_context_bind(struct handlebars_context * parent, struct handlebars_context * child)
{
    if( child->e ) {
        handlebars_throw(parent, HANDLEBARS_ERROR, "Error context already assigned");
    }
    child->e = parent->e;
}

void handlebars_context_dtor(struct handlebars_context * context)
{
    handlebars_talloc_free(context);
}

enum handlebars_error_type handlebars_error_num(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->num;
}

struct handlebars_locinfo handlebars_error_loc(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->loc;
}

const char * handlebars_error_msg(struct handlebars_context * context)
{
    assert(context->e != NULL);
    return context->e->msg;
}

char * handlebars_error_message(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[256];
    struct handlebars_error * e = context->e;

    assert(context != NULL);
    assert(context->e != NULL);

    if( e->msg == NULL ) {
        return NULL;
    }

    snprintf(errbuf, sizeof(errbuf), "%s on line %d, column %d",
             e->msg,
             e->loc.last_line,
             e->loc.last_column);

    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
        // this might be a bad idea...
        // fprintf(stderr, "%s\n", e->msg);
        // abort();
        return (char *) e->msg;
    }

    return errmsg;
}

char * handlebars_error_message_js(struct handlebars_context * context)
{
    char * errmsg;
    char errbuf[512];
    struct handlebars_error * e = context->e;

    assert(context != NULL);

    if( e->msg == NULL ) {
        return NULL;
    }

    // @todo check errno == HANDLEBARS_PARSEERR

    snprintf(errbuf, sizeof(errbuf), "Parse error on line %d, column %d : %s",
             e->loc.first_line,
             e->loc.first_column,
             e->msg);

    errmsg = handlebars_talloc_strdup(context, errbuf);
    if( unlikely(errmsg == NULL) ) {
        // this might be a bad idea...
        // fprintf(stderr, "%s\n", e->msg);
        // abort();
        return (char *) e->msg;
    }

    return errmsg;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

HBS_ATTR_PRINTF(4, 0)
static void _set_err(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, va_list ap)
{
    struct handlebars_error * e = context->e;
    e->num = num;

    e->msg = talloc_vasprintf(context, msg, ap);

    if( unlikely(e->msg == NULL) ) {
        e->num = HANDLEBARS_NOMEM;
        e->msg = HANDLEBARS_MEMCHECK_MSG;
    }
    if( loc ) {
        e->loc = *loc;
    } else {
        memset(&e->loc, 0, sizeof(e->loc));
    }
}

void handlebars_throw(struct handlebars_context * context, enum handlebars_error_type num, const char * msg, ...)
{
    struct handlebars_error * e = context->e;
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, NULL, msg, ap);
    va_end(ap);
#ifdef HANDLEBARS_ENABLE_DEBUG
    bool do_throw = NULL == getenv("HANDLEBARS_NOTHROW");
#else
    const bool do_throw = true;
#endif
    if( e->jmp && do_throw ) {
        longjmp(*e->jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", e->msg);
        abort();
    }
}

void handlebars_throw_ex(struct handlebars_context * context, enum handlebars_error_type num, struct handlebars_locinfo * loc, const char * msg, ...)
{
    struct handlebars_error * e = context->e;
    va_list ap;
    va_start(ap, msg);
    _set_err(context, num, loc, msg, ap);
    va_end(ap);
#ifdef HANDLEBARS_ENABLE_DEBUG
    bool do_throw = NULL == getenv("HANDLEBARS_NOTHROW");
#else
    const bool do_throw = true;
#endif
    if( e->jmp && do_throw ) {
        longjmp(*e->jmp, num);
    } else {
        fprintf(stderr, "Throw with invalid jmp_buf: %s\n", e->msg);
        abort();
    }
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

struct handlebars_context * handlebars_get_context(void * ctx, const char * loc)
{
    struct handlebars_context * r;

    r = talloc_get_type(ctx, struct handlebars_context);
    if( r ) {
        return r;
    }

    r = (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_parser);
    if( r ) {
        return r;
    }

    r = (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_compiler);
    if( r ) {
        return r;
    }

    r = (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_vm);
    if( r ) {
        return r;
    }

    r = (struct handlebars_context *) talloc_get_type(ctx, struct handlebars_cache);
    if( r ) {
        return r;
    }

    fprintf(stderr, "Not a context at %s\n", loc);
    abort();
}

const char* HANDLEBARS_MOTD = "Think not that I am come to send peace on earth: I came not to send peace, but a sword. Matthew 10:34";

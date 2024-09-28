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

#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



// {{{ Reference Counting

#ifndef HANDLEBARS_NO_REFCOUNT
static void user_rc_dtor(struct handlebars_rc * rc)
{
    struct handlebars_user * user = /*talloc_get_type_abort(*/hbs_container_of(rc, struct handlebars_user, rc)/*, struct handlebars_user)*/;
    user->handlers->dtor(user);
    handlebars_talloc_free(user);
}
#endif

#undef handlebars_user_addref
#undef handlebars_user_delref

void handlebars_user_addref(struct handlebars_user * user)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&user->rc);
#endif
}

void handlebars_user_delref(struct handlebars_user * user)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&user->rc, user_rc_dtor);
#endif
}

void handlebars_user_addref_ex(struct handlebars_user * user, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&user->rc);
        fprintf(stderr, "USR ADDREF %p (%zu -> %zu) %s %s\n", user, rc, rc + 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_user_addref(user);
#endif
}

void handlebars_user_delref_ex(struct handlebars_user * user, const char * expr, const char * loc)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    if (getenv("HANDLEBARS_RC_DEBUG")) { // LCOV_EXCL_START
        size_t rc = handlebars_rc_refcount(&user->rc);
        fprintf(stderr, "USR DELREF %p (%zu -> %zu) %s %s\n", user, rc, rc - 1, expr, loc);
    } // LCOV_EXCL_STOP
    handlebars_user_delref(user);
#endif
}

#ifdef HANDLEBARS_ENABLE_DEBUG
#define handlebars_user_addref(user) handlebars_user_addref_ex(user, #user, HBS_LOC)
#define handlebars_user_delref(user) handlebars_user_delref_ex(user, #user, HBS_LOC)
#endif

// }}} Reference Counting



void handlebars_user_init(struct handlebars_user * user, struct handlebars_context * ctx, const struct handlebars_value_handlers * handlers)
{
    user->ctx = ctx;
    user->handlers = handlers;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&user->rc);
#endif
}

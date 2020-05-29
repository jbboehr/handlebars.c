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

void handlebars_user_addref(struct handlebars_user * user)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&user->rc);
#endif
}

void handlebars_user_delref(struct handlebars_user * user)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&user->rc);
#endif
}

// }}} Reference Counting



void handlebars_user_init(struct handlebars_user * user, struct handlebars_context * ctx, const struct handlebars_value_handlers * handlers)
{
    user->ctx = ctx;
    user->handlers = handlers;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&user->rc, user_rc_dtor);
#endif
}

handlebars_count_func handlebars_value_handlers_get_count_fn(const struct handlebars_value_handlers * handlers) {
    return handlers->count;
}





struct handlebars_ptr {
    struct handlebars_user user;
    void * ptr;
};

static void hbs_ptr_dtor(struct handlebars_user * user)
{
    struct handlebars_ptr * uptr = (struct handlebars_ptr *) user;
    if( uptr && uptr->ptr ) {
        handlebars_talloc_free(uptr->ptr);
        uptr->ptr = NULL;
    }
}

static const struct handlebars_value_handlers ptr_handlers = {
    "ptr",
    NULL,
    &hbs_ptr_dtor,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

struct handlebars_ptr * handlebars_ptr_ctor(
    struct handlebars_context * ctx,
    void * ptr
) {
    struct handlebars_ptr * uptr = handlebars_talloc(ctx, struct handlebars_ptr);
    HANDLEBARS_MEMCHECK(uptr, ctx);
    handlebars_user_init((struct handlebars_user *) uptr, ctx, &ptr_handlers);
    // talloc_set_destructor(uptr, hbs_ptr_dtor);
    uptr->ptr = ptr;
    return uptr;
}

void * handlebars_ptr_get_ptr(struct handlebars_ptr * uptr)
{
    return uptr->ptr;
}

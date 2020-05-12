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



void handlebars_user_init(struct handlebars_user * user, const struct handlebars_value_handlers * handlers)
{
    user->handlers = handlers;
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&user->rc, user_rc_dtor);
#endif
}

handlebars_count_func handlebars_value_handlers_get_count_fn(const struct handlebars_value_handlers * handlers) {
    return handlers->count;
}

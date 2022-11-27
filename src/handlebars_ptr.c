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

#include <string.h>
#include <talloc.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_ptr.h"
#include "handlebars_value_handlers.h"

#ifndef HANDLEBARS_NO_REFCOUNT
#include "handlebars_rc.h"
#endif



struct handlebars_ptr {
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif
    const char * typ;
    void * uptr;
    bool nofree;
};

#ifndef HANDLEBARS_NO_REFCOUNT
static void ptr_rc_dtor(struct handlebars_rc * rc)
{
    struct handlebars_ptr * ptr = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_ptr, rc), struct handlebars_ptr);
    if (!ptr->nofree) {
        handlebars_talloc_free(ptr->uptr);
    }
    handlebars_talloc_free(ptr);
}
#endif

void handlebars_ptr_addref(struct handlebars_ptr * ptr)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&ptr->rc);
#endif
}

void handlebars_ptr_delref(struct handlebars_ptr * ptr)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&ptr->rc, ptr_rc_dtor);
#endif
}

struct handlebars_ptr * handlebars_ptr_ctor_ex(
    struct handlebars_context * ctx,
    const char * typ,
    void * uptr,
    bool nofree
) {
    struct handlebars_ptr * ptr = handlebars_talloc(ctx, struct handlebars_ptr);
    HANDLEBARS_MEMCHECK(ptr, ctx);
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&ptr->rc);
#endif
    ptr->typ = typ;
    ptr->nofree = nofree;
    ptr->uptr = uptr;
    return ptr;
}

void * handlebars_ptr_get_ptr_ex(struct handlebars_ptr * ptr, const char * typ)
{
    if (typ == ptr->typ || 0 == strcmp(typ, ptr->typ)) {
        return ptr->uptr;
    } else {
        fprintf(stderr, "Failed to retrieve ptr: %s != %s\n", typ, ptr->typ);
        abort();
    }
}

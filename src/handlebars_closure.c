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
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_value_private.h"

#include "handlebars_closure.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

struct handlebars_closure {
#ifndef HANDLEBARS_NO_REFCOUNT
    struct handlebars_rc rc;
#endif
    struct handlebars_vm * vm;
    handlebars_closure_func fn;
    int localc;
    struct handlebars_value localv[];
};

const size_t HANDLEBARS_CLOSURE_SIZE = sizeof(struct handlebars_closure);


// {{{ Reference Counting
#ifndef HANDLEBARS_NO_REFCOUNT
static void closure_rc_dtor(struct handlebars_rc * rc)
{
#ifdef HANDLEBARS_ENABLE_DEBUG
    if (getenv("HANDLEBARS_RC_DEBUG")) {
        fprintf(stderr, "CLO DTOR %p\n", hbs_container_of(rc, struct handlebars_closure, rc));
    }
#endif
    struct handlebars_closure * closure = talloc_get_type_abort(hbs_container_of(rc, struct handlebars_closure, rc), struct handlebars_closure);
    for (int i = 0; i < closure->localc; i++) {
        handlebars_value_dtor(&closure->localv[i]);
    }
    handlebars_talloc_free(closure);
}
#endif

void handlebars_closure_addref(struct handlebars_closure * closure)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_addref(&closure->rc);
#endif
}

void handlebars_closure_delref(struct handlebars_closure * closure)
{
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_delref(&closure->rc, closure_rc_dtor);
#endif
}
// }}} Reference Counting

struct handlebars_closure * handlebars_closure_ctor(
    struct handlebars_vm * vm,
    handlebars_closure_func fn,
    int localc,
    struct handlebars_value * localv
) {
    struct handlebars_closure * closure = handlebars_talloc_zero_size(vm, sizeof(struct handlebars_closure) + (sizeof(struct handlebars_value) * localc));
    talloc_set_type(closure, struct handlebars_closure);
#ifndef HANDLEBARS_NO_REFCOUNT
    handlebars_rc_init(&closure->rc);
#endif
    closure->vm = vm;
    closure->fn = fn;
    closure->localc = localc;
    for (int i = 0; i < localc; i++) {
        handlebars_value_value(&closure->localv[i], &localv[i]);
    }
    return closure;
}

struct handlebars_value * handlebars_closure_call(
    struct handlebars_closure * closure,
    HANDLEBARS_FUNCTION_ARGS
) {
    assert(rv != NULL);
    return closure->fn(closure->localc, closure->localv, HANDLEBARS_FUNCTION_ARGS_PASSTHRU);
}

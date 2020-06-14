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
    struct handlebars_module * module;
    long program;
    long partial_block_depth;
};


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
    struct handlebars_module * module,
    long program,
    long partial_block_depth
) {
    struct handlebars_closure * closure = handlebars_talloc_zero(vm, struct handlebars_closure);
    handlebars_rc_init(&closure->rc);
    closure->vm = vm;
    closure->module = module;
    closure->program = program;
    closure->partial_block_depth = partial_block_depth;
    return closure;
}

struct handlebars_value * handlebars_closure_call(
    struct handlebars_closure * closure,
    struct handlebars_value * input,
    struct handlebars_value * data,
    struct handlebars_value * block_params,
    struct handlebars_value * rv
) {
    struct handlebars_string * buffer = handlebars_vm_execute_ex(closure->vm, closure->module, input, closure->program, data, block_params);
    if (buffer) {
        handlebars_value_str(rv, buffer);
    }
    return rv;
}

long handlebars_closure_get_partial_block_depth(
    struct handlebars_closure * closure
) {
    return closure->partial_block_depth;
}

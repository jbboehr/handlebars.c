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

#ifndef HANDLEBARS_CLOSURE_H
#define HANDLEBARS_CLOSURE_H

#include "handlebars.h"
#include "handlebars_types.h"

HBS_EXTERN_C_START

struct handlebars_closure;
struct handlebars_context;
struct handlebars_module;

extern const size_t HANDLEBARS_CLOSURE_SIZE;

// {{{ Reference Counting
void handlebars_closure_addref(struct handlebars_closure * closure)
    HBS_ATTR_NONNULL_ALL;
void handlebars_closure_delref(struct handlebars_closure * closure)
    HBS_ATTR_NONNULL_ALL;
// }}} Reference Counting

struct handlebars_closure * handlebars_closure_ctor(
    struct handlebars_vm * vm,
    handlebars_closure_func fn,
    int localc,
    struct handlebars_value * localv
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_value * handlebars_closure_call(
    struct handlebars_closure * closure,
    HANDLEBARS_FUNCTION_ARGS
) HANDLEBARS_CLOSURE_ATTRS;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_COMPILER_H */

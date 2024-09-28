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

#ifndef HANDLEBARS_PTR_H
#define HANDLEBARS_PTR_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_ptr;

void handlebars_ptr_addref(struct handlebars_ptr * ptr)
    HBS_ATTR_NONNULL_ALL;

void handlebars_ptr_delref(struct handlebars_ptr * ptr)
    HBS_ATTR_NONNULL_ALL;

struct handlebars_ptr * handlebars_ptr_ctor_ex(
    struct handlebars_context * ctx,
    const char * typ,
    void * uptr,
    bool nofree
) HBS_ATTR_NONNULL_ALL;

#define handlebars_ptr_ctor(ctx, typ, uptr, nofree) handlebars_ptr_ctor_ex(ctx, HBS_S1(typ), uptr, nofree)

void * handlebars_ptr_get_ptr_ex(struct handlebars_ptr * ptr, const char * typ)
    HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

#define handlebars_ptr_get_ptr(ptr, typ) ((typ *) handlebars_ptr_get_ptr_ex(ptr, HBS_S1(typ)))

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_HANDLERS_H */

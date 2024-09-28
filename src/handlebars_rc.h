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

#ifndef HANDLEBARS_RC_H
#define HANDLEBARS_RC_H

#include "handlebars.h"

#include <stdint.h>

HBS_EXTERN_C_START

#define hbs_container_of(ptr, type, member) \
    ((type *) (void *) ((char *)(ptr) - offsetof(type, member)))

struct handlebars_rc;

typedef void (*handlebars_rc_dtor_func)(
    struct handlebars_rc *
);

struct handlebars_rc {
    uint8_t refcount;
};

#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

HBS_ATTR_NONNULL_ALL HBS_ATTR_ALWAYS_INLINE
inline void handlebars_rc_init(struct handlebars_rc * rc)
{
    rc->refcount = 0;
}

HBS_ATTR_NONNULL_ALL HBS_ATTR_ALWAYS_INLINE
inline void handlebars_rc_addref(struct handlebars_rc * rc)
{
    if (rc->refcount < UINT8_MAX) {
        rc->refcount++;
    }
}

HBS_ATTR_NONNULL_ALL HBS_ATTR_ALWAYS_INLINE
inline void handlebars_rc_delref(struct handlebars_rc * rc, handlebars_rc_dtor_func dtor)
{
    if (rc->refcount == UINT8_MAX) {
        // immortal
    } else if (rc->refcount <= 1) {
        rc->refcount = 0;
        dtor(rc);
    } else {
        rc->refcount--;
    }
}

HBS_ATTR_NONNULL_ALL HBS_ATTR_ALWAYS_INLINE
inline size_t handlebars_rc_refcount(struct handlebars_rc * rc)
{
    return rc->refcount;
}

HBS_EXTERN_C_END

#endif

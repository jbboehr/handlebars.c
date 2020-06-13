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

#include <stdio.h>

#include "handlebars.h"
#include "handlebars_rc.h"

extern inline void handlebars_rc_init(struct handlebars_rc * rc);
extern inline void handlebars_rc_addref(struct handlebars_rc * rc);
extern inline void handlebars_rc_delref(struct handlebars_rc * rc, handlebars_rc_dtor_func dtor);
extern inline size_t handlebars_rc_refcount(struct handlebars_rc * rc);

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

#ifndef HANDLEBARS_DELIMITERS_H
#define HANDLEBARS_DELIMITERS_H

#include "handlebars.h"

struct handlebars_string * handlebars_preprocess_delimiters(
    struct handlebars_context * ctx,
    struct handlebars_string * tmpl,
    struct handlebars_string * open,
    struct handlebars_string * close
) HBS_ATTR_NONNULL(1, 2) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

#endif /* HANDLEBARS_DELIMITERS_H */

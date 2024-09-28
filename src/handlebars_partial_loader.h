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

/**
 * @file
 * @brief Partial Loader
 */

#ifndef HANDLEBARS_PARTIAL_LOADER_H
#define HANDLEBARS_PARTIAL_LOADER_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_string;

struct handlebars_value * handlebars_value_partial_loader_init(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_PARTIAL_LOADER_H */

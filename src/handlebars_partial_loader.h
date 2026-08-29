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
struct handlebars_value;

struct handlebars_value * handlebars_value_partial_loader_init(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Initialize a filesystem-backed partial loader without allowing an
 *        internal `longjmp` to escape
 * @param[in] context The loader and allocation context
 * @param[in] base_path The directory containing partials
 * @param[in] extension The extension appended to partial names
 * @param[in,out] result The value to replace on success and preserve on failure
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_partial_loader_init_try(
    struct handlebars_context * context,
    struct handlebars_string * base_path,
    struct handlebars_string * extension,
    struct handlebars_value * result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Load a partial without allowing an internal `longjmp` to escape
 * @param[in] loader A value created by the partial-loader initializer
 * @param[in] name The partial name
 * @param[in,out] result The value to replace on success and preserve on failure
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_partial_loader_find_try(
    struct handlebars_value * loader,
    struct handlebars_string * name,
    struct handlebars_value * result
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

#ifdef HANDLEBARS_PARTIAL_LOADER_PRIVATE
HBS_LOCAL bool handlebars_value_is_partial_loader(
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL;
#endif

HBS_EXTERN_C_END

#endif /* HANDLEBARS_PARTIAL_LOADER_H */

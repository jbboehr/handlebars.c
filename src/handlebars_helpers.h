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

#ifndef HANDLEBARS_HELPERS_H
#define HANDLEBARS_HELPERS_H

#include "handlebars.h"
#include "handlebars_types.h"

HBS_EXTERN_C_START

struct handlebars_stack;
struct handlebars_value;
struct handlebars_vm;
struct handlebars_options;

extern const size_t HANDLEBARS_OPTIONS_SIZE;

void handlebars_options_deinit(
    struct handlebars_options * options
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get a NULL-terminated array of the names of all built-in helpers
 * @return The array
 */
const char ** handlebars_builtins_names(void)
    HBS_ATTR_RETURNS_NONNULL HBS_ATTR_PURE;

/**
 * @brief Get a NULL-terminated array of all built-in helper functions
 * @return The array
 */
handlebars_helper_func * handlebars_builtins(void)
    HBS_ATTR_RETURNS_NONNULL HBS_ATTR_PURE;

/**
 * @brief Get a built-in helper by name
 * @param[in] str
 * @param[in] len
 * @return The helper function pointer
 */
handlebars_helper_func handlebars_builtins_find(
    const char * str,
    unsigned int len
) HBS_ATTR_NONNULL_ALL HBS_ATTR_PURE;

struct handlebars_value * handlebars_builtin_block_helper_missing(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_each(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_helper_missing(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_lookup(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_log(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_if(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_unless(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_with(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;
struct handlebars_value * handlebars_builtin_hbsc_set_delimiters(HANDLEBARS_HELPER_ARGS) HANDLEBARS_HELPER_ATTRS;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_HELPERS_H */

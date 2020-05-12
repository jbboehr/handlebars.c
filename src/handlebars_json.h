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

#ifndef HANDLEBARS_JSON_H
#define HANDLEBARS_JSON_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_context;
struct handlebars_value;
struct json_object;

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void)
    HBS_ATTR_RETURNS_NONNULL HBS_ATTR_PURE;

/**
 * @brief Initialize a value from a JSON object
 * @param[in] ctx The handlebars context
 * @param[in] value The value to initialize
 * @param[in] json The JSON object
 * @return void
 */
void handlebars_value_init_json_object(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    struct json_object * json
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Initialize a value from a JSON string
 * @param[in] ctx The handlebars context
 * @param[in] value The value to initialize
 * @param[in] json The JSON string
 */
void handlebars_value_init_json_string(
    struct handlebars_context *ctx,
    struct handlebars_value * value,
    const char * json
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Construct a value from a JSON string
 * @param[in] ctx The handlebars context
 * @param[in] json The JSON string
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_json_string(
    struct handlebars_context *ctx,
    const char * json
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Construct a value from a JSON object
 * @param[in] ctx The handlebars context
 * @param[in] json The JSON object
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_json_object(
    struct handlebars_context * ctx,
    struct json_object * json
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_JSON_H */

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

#ifndef HANDLEBARS_YAML_H
#define HANDLEBARS_YAML_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct yaml_document_s;
struct yaml_node_s;

/**
 * @brief Initialize a value from a YAML node
 * @param[in] ctx
 * @param[in] value
 * @param[in] document
 * @param[in] node
 * @return void
 */
void handlebars_value_init_yaml_node(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    struct yaml_document_s * document,
    struct yaml_node_s * node
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Initialize a value from a YAML node without allowing an internal
 *        `longjmp` to escape
 * @param[in] ctx The handlebars context
 * @param[in,out] value The value to replace on success and preserve on failure
 * @param[in] document The YAML document containing node
 * @param[in] node The YAML node to convert
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_init_yaml_node_try(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    struct yaml_document_s * document,
    struct yaml_node_s * node
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Initialize a value from a YAML string
 * @param[in] ctx
 * @param[in] value
 * @param[in] yaml
 * @return void
 */
void handlebars_value_init_yaml_string(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    const char * yaml
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Initialize a value from a YAML string without allowing an internal
 *        `longjmp` to escape
 * @param[in] ctx The handlebars context
 * @param[in,out] value The value to replace on success and preserve on failure
 * @param[in] yaml The YAML string
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_init_yaml_string_try(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    const char * yaml
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Initialize a value from a length-delimited YAML string
 * @param[in] ctx
 * @param[in] value
 * @param[in] yaml
 * @param[in] length
 * @return void
 */
void handlebars_value_init_yaml_stringl(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    const char * yaml,
    size_t length
) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Initialize a value from a length-delimited YAML string without
 *        allowing an internal `longjmp` to escape
 * @param[in] ctx The handlebars context
 * @param[in,out] value The value to replace on success and preserve on failure
 * @param[in] yaml The YAML string
 * @param[in] length The YAML string length
 * @return #HANDLEBARS_SUCCESS or the captured error code
 */
enum handlebars_error_type handlebars_value_init_yaml_stringl_try(
    struct handlebars_context * ctx,
    struct handlebars_value * value,
    const char * yaml,
    size_t length
) HBS_ATTR_NONNULL_ALL HBS_ATTR_WARN_UNUSED_RESULT;

HBS_EXTERN_C_END

#endif /* HANDLEBARS_YAML_H */

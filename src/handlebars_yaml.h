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

HBS_EXTERN_C_END

#endif /* HANDLEBARS_YAML_H */

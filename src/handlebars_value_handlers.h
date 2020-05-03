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

#ifndef HANDLEBARS_VALUE_HANDLERS_H
#define HANDLEBARS_VALUE_HANDLERS_H

#include "handlebars.h"

HBS_EXTERN_C_END

struct handlebars_string;
struct handlebars_value_iterator;
struct json_object;
struct yaml_document_s;
struct yaml_node_s;

typedef struct handlebars_value * (*handlebars_copy_func)(struct handlebars_value * value);
typedef void (*handlebars_value_dtor_func)(struct handlebars_value * value);
typedef void (*handlebars_value_convert_func)(struct handlebars_value * value, bool recurse);
typedef enum handlebars_value_type (*handlebars_value_type_func)(struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_map_find_func)(struct handlebars_value * value, struct handlebars_string * key);
typedef struct handlebars_value * (*handlebars_array_find_func)(struct handlebars_value * value, size_t index);
typedef bool (*handlebars_iterator_init_func)(struct handlebars_value_iterator * it, struct handlebars_value * value);
typedef struct handlebars_value * (*handlebars_call_func)(
    struct handlebars_value * value,
    int argc,
    struct handlebars_value * argv[],
    struct handlebars_options * options
);
typedef long (*handlebars_count_func)(struct handlebars_value * value);

struct handlebars_value_handlers * handlebars_value_get_std_json_handlers(void) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Initialize a value from a JSON object
 * @param[in] ctx The handlebars context
 * @param[in] value The value to initialize
 * @param[in] json The JSON object
 * @return void
 */
void handlebars_value_init_json_object(struct handlebars_context * ctx, struct handlebars_value * value, struct json_object *json);

/**
 * @brief Initialize a value from a JSON string
 * @param[in] ctx The handlebars context
 * @param[in] value The value to initialize
 * @param[in] json The JSON string
 */
void handlebars_value_init_json_string(struct handlebars_context *ctx, struct handlebars_value * value, const char * json);

/**
 * @brief Construct a value from a JSON string
 * @param[in] ctx The handlebars context
 * @param[in] json The JSON string
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_json_string(struct handlebars_context *ctx, const char * json) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Construct a value from a JSON object
 * @param[in] ctx The handlebars context
 * @param[in] json The JSON object
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_json_object(struct handlebars_context *ctx, struct json_object *json) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Initialize a value from a YAML node
 * @param[in] ctx
 * @param[in] value
 * @param[in] document
 * @param[in] node
 * @return void
 */
void handlebars_value_init_yaml_node(struct handlebars_context *ctx, struct handlebars_value * value, struct yaml_document_s * document, struct yaml_node_s * node);

/**
 * @brief Initialize a value from a YAML string
 * @param[in] ctx
 * @param[in] value
 * @param[in] yaml
 * @return void
 */
void handlebars_value_init_yaml_string(struct handlebars_context * ctx, struct handlebars_value * value, const char * yaml);

/**
 * @brief Construct a value from a YAML node
 * @param[in] ctx The handlebars context
 * @param[in] document
 * @param[in] node
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_yaml_node(struct handlebars_context *ctx, struct yaml_document_s * document, struct yaml_node_s * node) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Initialize a value from a YAML string
 * @param[in] ctx The handlebars context
 * @param[in] yaml The YAML string
 * @return The constructed value
 */
struct handlebars_value * handlebars_value_from_yaml_string(struct handlebars_context * ctx, const char * yaml) HBS_ATTR_RETURNS_NONNULL;

#ifndef HANDLEBARS_VALUE_HANDLERS_PRIVATE

handlebars_count_func handlebars_value_handlers_get_count_fn(struct handlebars_value_handlers * handlers) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

#else /* HANDLEBARS_VALUE_HANDLERS_PRIVATE */

struct handlebars_value_handlers {
    const char * name;
    handlebars_copy_func copy;
    handlebars_value_dtor_func dtor;
    handlebars_value_convert_func convert;
    handlebars_value_type_func type;
    handlebars_map_find_func map_find;
    handlebars_array_find_func array_find;
    handlebars_iterator_init_func iterator;
    handlebars_call_func call;
    handlebars_count_func count;
};

HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL
inline handlebars_count_func handlebars_value_handlers_get_count_fn(struct handlebars_value_handlers * handlers) {
    return handlers->count;
}

#endif /* HANDLEBARS_VALUE_HANDLERS_PRIVATE */

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VALUE_HANDLERS_H */

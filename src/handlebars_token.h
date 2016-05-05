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

/**
 * @file
 * @brief Token
 */

#ifndef HANDLEBARS_TOKEN_H
#define HANDLEBARS_TOKEN_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_string;

/**
 * @brief Token structure 
 */
struct handlebars_token {
    int token;
    struct handlebars_string * string;
};

/**
 * @brief Flags to control print behaviour
 */
enum handlebars_token_print_flags
{
    /**
     * @brief Default print behaviour flag
     */
    handlebars_token_print_flag_none = 0,

    /**
     * @brief Use newlines between tokens instead of spaces
     */
    handlebars_token_print_flag_newlines = 1
};

/**
 * @brief Construct a token. Returns NULL on failure.
 *
 * @param[in] context The handlebars context
 * @param[in] token_int Token type
 * @param[in] string Token text
 * @return the new token object
 */
struct handlebars_token * handlebars_token_ctor(
    struct handlebars_context * context,
    int token_int,
    struct handlebars_string * string
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a token
 * 
 * @param[in] token Token type
 * @return void
 */
void handlebars_token_dtor(struct handlebars_token * token) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the token type
 * 
 * @param[in] token Token
 * @return void
 */
int handlebars_token_get_type(struct handlebars_token * token) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the token text.
 * 
 * @param[in] token Token
 * @return The token text
 */
struct handlebars_string * handlebars_token_get_text(struct handlebars_token * token);

/**
 * @brief Get a string for the integral token type
 * 
 * @param[in] type The integral token type
 * @return The string name of the type
 */
const char * handlebars_token_readable_type(int type) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get an integral type for the token name
 * 
 * @param[in] type The token type name
 * @return The integral token type
 */
int handlebars_token_reverse_readable_type(const char * type) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Print a token into a human-readable string
 *
 * @param[in] context The handlebars context
 * @param[in] string The string to append the result
 * @param[in] token The token to print
 * @param[in] flags The print flags
 * @return The printed token
 */
struct handlebars_string * handlebars_token_print_append(
    struct handlebars_context * context,
    struct handlebars_string * string,
    struct handlebars_token * token,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Print a token into a human-readable string
 *
 * @param[in] context The handlebars context
 * @param[in] token The token to print
 * @param[in] flags The print flags
 * @return The printed token
 */
struct handlebars_string * handlebars_token_print(
    struct handlebars_context * context,
    struct handlebars_token * token,
    int flags
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif

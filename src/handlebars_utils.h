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
 * @brief Utilities
 */

#ifndef HANDLEBARS_UTILS_H
#define HANDLEBARS_UTILS_H

#include "handlebars.h"

HBS_EXTERN_C_START

/**
 * Declarations
 */
struct handlebars_context;
struct handlebars_locinfo;
struct handlebars_parser;
union YYSTYPE;

/**
 * @brief Handle an error in the parser. Prints message to stderr
 *
 * @param[in] lloc The parser location info
 * @param[in] parser The handlebars parser
 * @param[in] err The error message
 * @return void
 */
void handlebars_yy_error(
    struct handlebars_locinfo * lloc,
    struct handlebars_parser * parser,
    const char * err
) HBS_ATTR_NORETURN;

/**
 * @brief Handle a fatal error in the parser. Prints message to stderr and exits with code 2.
 *
 * @param[in] msg An error message
 * @param[in] yyscanner The scanner object
 * @return void
 */
void handlebars_yy_fatal_error(
    const char * msg,
    struct handlebars_parser * parser
) HBS_ATTR_NORETURN;

/**
 * @brief Reads input for the lexer. Reads input from the tmpl field of the context object
 *
 * @param[out] buffer The buffer to store input into
 * @param[out] numBytesRead The number of bytes read
 * @param[in] maxBytesToRead The maximum number of bytes to read
 * @param[in] parser The handlebars parser
 * @return void
 */
void handlebars_yy_input(
    char * buffer,
    int *numBytesRead,
    int maxBytesToRead,
    struct handlebars_parser * parser
);

/**
 * @brief Print a parser value
 *
 * @param[in] file The file handle to which to write
 * @param[in] type The parser node type
 * @param[in] value The parser node value
 * @return void
 */
void handlebars_yy_print(
    FILE *file,
    int type,
    union YYSTYPE value
);

/**
 * @brief Custom alloc for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 *
 * @param[in] bytes The number of bytes to allocate
 * @param[in] yyscanner The scanner context. NULL if allocating the yyscanner itself.
 * @return A pointer to the newly allocated memory, or NULL on failure
 */
void * handlebars_yy_alloc(
    size_t bytes,
    void * yyscanner
);

/**
 * @brief Custom realloc for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 *
 * @param[in] ptr The pointer to reallocate
 * @param[in] bytes The desired new size
 * @param[in] yyscanner The scanner context
 * @return The original pointer, or a new pointer, or NULL on failure
 */
void * handlebars_yy_realloc(
    void * ptr,
    size_t bytes,
    void * yyscanner
);

/**
 * @brief Custom free for use with flex/bison. Uses talloc with
 *        handlebars_context as a talloc context.
 *
 * @param[in] ptr The pointer to free
 * @param[in] yyscanner The scanner context
 * @return void
 */
void handlebars_yy_free(
    void * ptr,
    void * yyscanner
);

HBS_EXTERN_C_END

#endif /* HANDLEBARS_UTILS_H */

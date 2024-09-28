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

#ifndef HANDLEBARS_VM_PRIVATE_H
#define HANDLEBARS_VM_PRIVATE_H

#include "handlebars.h"
#include "handlebars_types.h"
#include "handlebars_value_private.h"

HBS_EXTERN_C_START

struct handlebars_cache;
struct handlebars_module;
struct handlebars_string;
struct handlebars_stack;

struct handlebars_vm {
    struct handlebars_context ctx;
    struct handlebars_cache * cache;

    struct handlebars_module * module;

    long depth;
    unsigned long flags;

    struct handlebars_string * buffer;

    struct handlebars_value data;
    struct handlebars_value helpers;
    struct handlebars_value partials;

    struct handlebars_string * last_helper;
    struct handlebars_value * last_context;

    struct handlebars_stack * stack;
    struct handlebars_stack * contextStack;
    struct handlebars_stack * hashStack;
    struct handlebars_stack * blockParamStack;
    struct handlebars_stack * partialBlockStack;

    handlebars_func log_func;
    void * log_ctx;

    struct handlebars_string * delim_open;
    struct handlebars_string * delim_close;
};

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VM_PRIVATE_H */

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

#ifndef HANDLEBARS_VM_H
#define HANDLEBARS_VM_H

#include <setjmp.h>
#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_compiler;
struct handlebars_context;
struct handlebars_map;
struct handlebars_module;
struct handlebars_options;

#ifndef HANDLEBARS_VM_STACK_SIZE
#define HANDLEBARS_VM_STACK_SIZE 96
#endif

#ifndef HANDLEBARS_VM_BUFFER_INIT_SIZE
#define HANDLEBARS_VM_BUFFER_INIT_SIZE 128
#endif

typedef void (*handlebars_log_func)(
    int argc,
    struct handlebars_value * argv[],
    struct handlebars_options * options
);

struct handlebars_vm_stack {
    size_t i;
    struct handlebars_value * v[HANDLEBARS_VM_STACK_SIZE];
};

struct handlebars_vm {
    struct handlebars_context ctx;
    struct handlebars_cache * cache;

    struct handlebars_module * module;

    size_t guid_index;
    long depth;
    long flags;
    handlebars_log_func log_func;
    void * log_ctx;

    struct handlebars_string * buffer;

    struct handlebars_value * context;
    struct handlebars_value * data;
    struct handlebars_value * helpers;
    struct handlebars_value * partials;

    struct handlebars_string * last_helper;
    struct handlebars_value * last_context;

    struct handlebars_vm_stack stack;
    struct handlebars_vm_stack contextStack;
    struct handlebars_vm_stack hashStack;
    struct handlebars_vm_stack blockParamStack;
};

/**
 * @brief Construct a VM
 * @param[in] ctx The parent handlebars context
 * @return The string array
 */
struct handlebars_vm * handlebars_vm_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a VM
 * @param[in] vm The VM to destruct
 * @return The string array
 */
void handlebars_vm_dtor(struct handlebars_vm * vm) HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_vm_execute(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_vm_execute_program(
    struct handlebars_vm * vm,
    long program,
    struct handlebars_value * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

struct handlebars_string * handlebars_vm_execute_program_ex(
    struct handlebars_vm * vm,
    long program,
    struct handlebars_value * context,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL;

#ifdef	__cplusplus
}
#endif

#endif

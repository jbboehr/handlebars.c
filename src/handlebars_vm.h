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

#ifndef HANDLEBARS_VM_H
#define HANDLEBARS_VM_H

#include "handlebars.h"

HBS_EXTERN_C_START

struct handlebars_cache;
struct handlebars_compiler;
struct handlebars_context;
struct handlebars_map;
struct handlebars_module;
struct handlebars_options;
struct handlebars_vm;

#ifndef HANDLEBARS_VM_STACK_SIZE
#define HANDLEBARS_VM_STACK_SIZE 96
#endif

#ifndef HANDLEBARS_VM_BUFFER_INIT_SIZE
#define HANDLEBARS_VM_BUFFER_INIT_SIZE 128
#endif

extern const size_t HANDLEBARS_VM_SIZE;

/**
 * @brief Construct a VM
 * @param[in] ctx The parent handlebars context
 * @return The string array
 */
struct handlebars_vm * handlebars_vm_ctor(
    struct handlebars_context * ctx
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

/**
 * @brief Destruct a VM
 * @param[in] vm The VM to destruct
 * @return The string array
 */
void handlebars_vm_dtor(
    struct handlebars_vm * vm
) HBS_ATTR_NONNULL_ALL;

struct handlebars_string * handlebars_vm_execute(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_NOINLINE;

struct handlebars_string * handlebars_vm_execute_ex(
    struct handlebars_vm * vm,
    struct handlebars_module * module,
    struct handlebars_value * context,
    long program,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) HBS_ATTR_NONNULL(1, 2, 3) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_NOINLINE;

struct handlebars_string * handlebars_vm_execute_program(
    struct handlebars_vm * vm,
    long program,
    struct handlebars_value * context
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_string * handlebars_vm_execute_program_ex(
    struct handlebars_vm * vm,
    long program,
    struct handlebars_value * context,
    struct handlebars_value * data,
    struct handlebars_value * block_params
) HBS_ATTR_NONNULL(1, 3) HBS_ATTR_RETURNS_NONNULL HBS_ATTR_WARN_UNUSED_RESULT;

struct handlebars_value * handlebars_vm_call_helper_str(
    const char * name,
    unsigned int len,
    int argc,
    struct handlebars_value * argv,
    struct handlebars_options * options,
    struct handlebars_vm * vm,
    struct handlebars_value * rv
) HBS_ATTR_NONNULL(1, 4, 5) HBS_ATTR_WARN_UNUSED_RESULT;

void handlebars_vm_set_flags(struct handlebars_vm * vm, unsigned long flags) HBS_ATTR_NONNULL_ALL;
void handlebars_vm_set_helpers(struct handlebars_vm * vm, struct handlebars_value * helpers) HBS_ATTR_NONNULL_ALL;
void handlebars_vm_set_partials(struct handlebars_vm * vm, struct handlebars_value * helpers) HBS_ATTR_NONNULL_ALL;
void handlebars_vm_set_data(struct handlebars_vm * vm, struct handlebars_value * data) HBS_ATTR_NONNULL_ALL;
void handlebars_vm_set_cache(struct handlebars_vm * vm, struct handlebars_cache * cache) HBS_ATTR_NONNULL_ALL;
void handlebars_vm_set_logger(struct handlebars_vm * vm, handlebars_func log_func, void * log_ctx) HBS_ATTR_NONNULL(1, 2);

handlebars_func handlebars_vm_get_log_func(struct handlebars_vm * vm);
void * handlebars_vm_get_log_ctx(struct handlebars_vm * vm);

HBS_EXTERN_C_END

#endif /* HANDLEBARS_VM_H */

/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_module_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

#define HANDLEBARS_FUZZ_MAX_INPUT_SIZE (64 * 1024)

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

static struct handlebars_value * handlebars_fuzz_log(HANDLEBARS_FUNCTION_ARGS)
{
    (void) argc;
    (void) argv;
    (void) options;
    (void) vm;
    return rv;
}

static void handlebars_fuzz_execute_module(
    struct handlebars_context * context,
    struct handlebars_module * module
) {
    struct handlebars_vm * vm = handlebars_vm_ctor(context);
    struct handlebars_map * map = handlebars_map_ctor(context, 3);
    struct handlebars_string * output;
    HANDLEBARS_VALUE_DECL(child);
    HANDLEBARS_VALUE_DECL(input);

    handlebars_value_str(child, handlebars_string_ctor(context, HBS_STRL("value")));
    map = handlebars_map_str_add(map, HBS_STRL("foo"), child);
    handlebars_value_boolean(child, true);
    map = handlebars_map_str_add(map, HBS_STRL("bar"), child);
    handlebars_value_map(input, map);

    handlebars_vm_set_flags(vm, module->flags);
    handlebars_vm_set_logger(vm, handlebars_fuzz_log, NULL);
    output = handlebars_vm_execute(vm, module, input);
    (void) output;

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(child);
}

static void handlebars_fuzz_exercise_module(
    struct handlebars_context * context,
    struct handlebars_module * module,
    bool execute
) {
    struct handlebars_string * printed;

    handlebars_module_patch_pointers(module);
    printed = handlebars_module_print(context, module);
    (void) printed;

    if( execute ) {
        handlebars_fuzz_execute_module(context, module);
    }

    handlebars_module_normalize_pointers(module, NULL);
    handlebars_module_generate_hash(module);
    if( !handlebars_module_verify_ex(module, module->size, NULL) ) {
        __builtin_trap();
    }
    handlebars_module_patch_pointers(module);
    printed = handlebars_module_print(context, module);
    (void) printed;
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    struct handlebars_context * context;
    struct handlebars_module * module;
    bool originally_valid;
    jmp_buf buf;

    if( size > HANDLEBARS_FUZZ_MAX_INPUT_SIZE ) {
        return 0;
    }

    context = handlebars_context_ctor();
    if( context == NULL ) {
        return 0;
    }

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_context_dtor(context);
        return 0;
    }

    module = handlebars_talloc_size(context, size > 0 ? size : 1);
    if( size > 0 ) {
        memcpy(module, data, size);
    }

    originally_valid = handlebars_module_verify_ex(module, size, NULL);
    if( size < sizeof(struct handlebars_module) ) {
        handlebars_context_dtor(context);
        return 0;
    }

    /* Keep mutations inside the serialized-module state space. The original
     * bytes are verified first so header, version, size, and hash handling
     * still receive direct coverage. */
    memset(module->header, 0, sizeof(module->header));
    memcpy(module->header, "HBSCM", sizeof("HBSCM") - 1);
    module->version = handlebars_version();
    module->addr = NULL;
    module->size = size;
    handlebars_module_generate_hash(module);

    if( handlebars_module_verify_ex(module, size, NULL) ) {
        handlebars_fuzz_exercise_module(context, module, originally_valid);
    }

    handlebars_context_dtor(context);
    return 0;
}

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

#include "handlebars.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

#define HANDLEBARS_FUZZ_MAX_INPUT_SIZE (64 * 1024)
#define HANDLEBARS_FUZZ_FLAG_PREFIX_SIZE 5

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

static int handlebars_fuzz_hex_digit(uint8_t c)
{
    if( c >= '0' && c <= '9' ) {
        return c - '0';
    }
    if( c >= 'a' && c <= 'f' ) {
        return c - 'a' + 10;
    }
    if( c >= 'A' && c <= 'F' ) {
        return c - 'A' + 10;
    }
    return -1;
}

static size_t handlebars_fuzz_parse_flags(
    const uint8_t * data,
    size_t size,
    unsigned long * flags
) {
    unsigned long parsed = 0;
    size_t i;

    if( size < HANDLEBARS_FUZZ_FLAG_PREFIX_SIZE || data[4] != '\n' ) {
        return 0;
    }

    for( i = 0; i < 4; i++ ) {
        int digit = handlebars_fuzz_hex_digit(data[i]);
        if( digit < 0 ) {
            return 0;
        }
        parsed = (parsed << 4) | (unsigned long) digit;
    }

    *flags = parsed & handlebars_compiler_flag_all;
    return HANDLEBARS_FUZZ_FLAG_PREFIX_SIZE;
}

static struct handlebars_value * handlebars_fuzz_log(HANDLEBARS_FUNCTION_ARGS)
{
    (void) argc;
    (void) argv;
    (void) options;
    (void) vm;
    return rv;
}

static void handlebars_fuzz_init_input(
    struct handlebars_context * context,
    struct handlebars_value * input
) {
    struct handlebars_map * root = handlebars_map_ctor(context, 5);
    struct handlebars_map * nested = handlebars_map_ctor(context, 1);
    struct handlebars_stack * items = handlebars_stack_ctor(context, 2);
    struct handlebars_map * item;
    HANDLEBARS_VALUE_DECL(value);
    HANDLEBARS_VALUE_DECL(nested_value);
    HANDLEBARS_VALUE_DECL(items_value);
    HANDLEBARS_VALUE_DECL(item_value);

    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("alpha")));
    root = handlebars_map_str_add(root, HBS_STRL("a"), value);
    handlebars_value_boolean(value, true);
    root = handlebars_map_str_add(root, HBS_STRL("b"), value);
    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("root")));
    root = handlebars_map_str_add(root, HBS_STRL("this"), value);

    handlebars_value_integer(value, 42);
    nested = handlebars_map_str_add(nested, HBS_STRL("bar"), value);
    handlebars_value_map(nested_value, nested);
    root = handlebars_map_str_add(root, HBS_STRL("foo"), nested_value);

    item = handlebars_map_ctor(context, 2);
    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("first")));
    item = handlebars_map_str_add(item, HBS_STRL("name"), value);
    handlebars_value_integer(value, 1);
    item = handlebars_map_str_add(item, HBS_STRL("value"), value);
    handlebars_value_map(item_value, item);
    items = handlebars_stack_push(items, item_value);

    item = handlebars_map_ctor(context, 2);
    handlebars_value_str(value, handlebars_string_ctor(context, HBS_STRL("second")));
    item = handlebars_map_str_add(item, HBS_STRL("name"), value);
    handlebars_value_integer(value, 2);
    item = handlebars_map_str_add(item, HBS_STRL("value"), value);
    handlebars_value_map(item_value, item);
    items = handlebars_stack_push(items, item_value);

    handlebars_value_array(items_value, items);
    root = handlebars_map_str_add(root, HBS_STRL("items"), items_value);
    handlebars_value_map(input, root);

    HANDLEBARS_VALUE_UNDECL(item_value);
    HANDLEBARS_VALUE_UNDECL(items_value);
    HANDLEBARS_VALUE_UNDECL(nested_value);
    HANDLEBARS_VALUE_UNDECL(value);
}

static void handlebars_fuzz_init_partials(
    struct handlebars_context * context,
    struct handlebars_value * partials
) {
    struct handlebars_map * map = handlebars_map_ctor(context, 5);
    HANDLEBARS_VALUE_DECL(partial);

    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{a}}")));
    map = handlebars_map_str_add(map, HBS_STRL("a"), partial);
    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{#if b}}true{{/if}}")));
    map = handlebars_map_str_add(map, HBS_STRL("b"), partial);
    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{foo.bar}}")));
    map = handlebars_map_str_add(map, HBS_STRL("foo"), partial);
    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{> foo}}")));
    map = handlebars_map_str_add(map, HBS_STRL("partial"), partial);
    handlebars_value_str(partial, handlebars_string_ctor(context, HBS_STRL("{{@partial-block}}")));
    map = handlebars_map_str_add(map, HBS_STRL("x"), partial);
    handlebars_value_map(partials, map);

    HANDLEBARS_VALUE_UNDECL(partial);
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    struct handlebars_context * context;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_vm * vm;
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    struct handlebars_module * module;
    unsigned long flags = 0;
    size_t prefix_size;
    jmp_buf buf;

    if( size > HANDLEBARS_FUZZ_MAX_INPUT_SIZE ) {
        return 0;
    }

    prefix_size = handlebars_fuzz_parse_flags(data, size, &flags);
    data += prefix_size;
    size -= prefix_size;

    context = handlebars_context_ctor();
    if( context == NULL ) {
        return 0;
    }

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_context_dtor(context);
        return 0;
    }

    parser = handlebars_parser_ctor(context);
    compiler = handlebars_compiler_ctor(context);
    vm = handlebars_vm_ctor(context);
    handlebars_vm_set_logger(vm, handlebars_fuzz_log, NULL);

    tmpl = handlebars_string_ctor(context, (const char *) data, size);
    ast = handlebars_parse_ex(parser, tmpl, flags);
    if( ast == NULL ) {
        handlebars_parser_dtor(parser);
        handlebars_context_dtor(context);
        return 0;
    }

    {
        struct handlebars_string * printed = handlebars_ast_print(context, ast);
        struct handlebars_string * reconstructed = handlebars_ast_to_string(context, ast);
        handlebars_talloc_free(printed);
        handlebars_talloc_free(reconstructed);
    }

    handlebars_compiler_set_flags(compiler, flags);
    program = handlebars_compiler_compile_ex(compiler, ast);
    if( program == NULL ) {
        handlebars_parser_dtor(parser);
        handlebars_context_dtor(context);
        return 0;
    }
    module = handlebars_program_serialize(context, program);
    if( module == NULL ) {
        handlebars_parser_dtor(parser);
        handlebars_context_dtor(context);
        return 0;
    }
    handlebars_module_generate_hash(module);
    if( !handlebars_module_verify_ex(module, handlebars_module_get_size(module), NULL) ) {
        __builtin_trap();
    }
    handlebars_vm_set_flags(vm, flags);

    {
        struct handlebars_string * output;
        HANDLEBARS_VALUE_DECL(input);
        HANDLEBARS_VALUE_DECL(partials);
        handlebars_fuzz_init_input(context, input);
        handlebars_fuzz_init_partials(context, partials);
        handlebars_vm_set_partials(vm, partials);
        output = handlebars_vm_execute(vm, module, input);
        (void) output;
        HANDLEBARS_VALUE_UNDECL(partials);
        HANDLEBARS_VALUE_UNDECL(input);
    }

    handlebars_parser_dtor(parser);
    handlebars_context_dtor(context);
    return 0;
}

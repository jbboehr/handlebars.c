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

#include <stdbool.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars_yaml.h"

#define HANDLEBARS_FUZZ_MAX_INPUT_SIZE (64 * 1024)
#define HANDLEBARS_FUZZ_MAX_DEPTH 32
/* handlebars_value_dump's fixed indentation buffer supports depths 0-30. */
#define HANDLEBARS_FUZZ_MAX_DUMP_DEPTH 30
#define HANDLEBARS_FUZZ_MAX_VALUES 4096

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

static void handlebars_fuzz_touch_value(struct handlebars_value * value)
{
    (void) handlebars_value_get_type(value);
    (void) handlebars_value_count(value);
    (void) handlebars_value_is_empty(value);
    (void) handlebars_value_is_scalar(value);
}

static bool handlebars_fuzz_walk_value(
    struct handlebars_value * value,
    size_t depth,
    size_t * remaining
) {
    bool dump_safe = depth <= HANDLEBARS_FUZZ_MAX_DUMP_DEPTH;
    enum handlebars_value_type type;

    if( *remaining == 0 ) {
        return false;
    }
    (*remaining)--;

    type = handlebars_value_get_type(value);
    handlebars_fuzz_touch_value(value);

    if( depth >= HANDLEBARS_FUZZ_MAX_DEPTH ) {
        return dump_safe;
    }

    /* Drain iterators after the budget expires so native maps clear their
     * iteration state. */
    if( type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        size_t count = (size_t) handlebars_value_count(value);

        HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
            if( *remaining > 0 ) {
                HANDLEBARS_VALUE_DECL(found);

                if( handlebars_value_array_find(value, index, found) != NULL ) {
                    handlebars_fuzz_touch_value(found);
                }
                if( !handlebars_fuzz_walk_value(child, depth + 1, remaining) ) {
                    dump_safe = false;
                }
                HANDLEBARS_VALUE_UNDECL(found);
            } else {
                dump_safe = false;
            }
        } HANDLEBARS_VALUE_FOREACH_END();

        {
            HANDLEBARS_VALUE_DECL(missing);
            (void) handlebars_value_array_find(value, count, missing);
            HANDLEBARS_VALUE_UNDECL(missing);
        }
    } else if( type == HANDLEBARS_VALUE_TYPE_MAP ) {
        HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
            if( *remaining > 0 ) {
                HANDLEBARS_VALUE_DECL(found);

                if( handlebars_value_map_find(value, key, found) != NULL ) {
                    handlebars_fuzz_touch_value(found);
                }
                if( !handlebars_fuzz_walk_value(child, depth + 1, remaining) ) {
                    dump_safe = false;
                }
                HANDLEBARS_VALUE_UNDECL(found);
            } else {
                dump_safe = false;
            }
        } HANDLEBARS_VALUE_FOREACH_END();
    }

    return dump_safe;
}

static void handlebars_fuzz_render(
    struct handlebars_context * context,
    struct handlebars_value * input
) {
    static const char template[] =
        "{{this}}{{a}}{{foo.bar}}"
        "{{#each items}}{{name}}={{value}};{{/each}}"
        "{{#each this}}{{this}}{{/each}}";
    struct handlebars_parser * parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * compiler = handlebars_compiler_ctor(context);
    struct handlebars_vm * vm = handlebars_vm_ctor(context);
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    struct handlebars_module * module;
    struct handlebars_string * output;

    tmpl = handlebars_string_ctor(context, HBS_STRL(template));
    ast = handlebars_parse_ex(parser, tmpl, 0);
    program = handlebars_compiler_compile_ex(compiler, ast);
    module = handlebars_program_serialize(context, program);
    output = handlebars_vm_execute(vm, module, input);
    (void) output;

    handlebars_parser_dtor(parser);
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    struct handlebars_context * context;
    struct handlebars_string * expression;
    bool dump_safe;
    size_t remaining;
    char * dump;
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

    {
        HANDLEBARS_VALUE_DECL(value);

        handlebars_value_init_yaml_stringl(
            context,
            value,
            (const char *) data,
            size
        );

        remaining = HANDLEBARS_FUZZ_MAX_VALUES;
        dump_safe = handlebars_fuzz_walk_value(value, 0, &remaining);
        expression = handlebars_value_expression(context, value, false);
        (void) expression;
        if( dump_safe ) {
            dump = handlebars_value_dump(value, context, 0);
            (void) dump;
        }
        handlebars_fuzz_render(context, value);

        HANDLEBARS_VALUE_UNDECL(value);
    }
    handlebars_context_dtor(context);
    return 0;
}

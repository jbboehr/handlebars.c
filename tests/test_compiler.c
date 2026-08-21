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

#include <check.h>
#include <limits.h>
#include <string.h>
#include <talloc.h>

#define HANDLEBARS_AST_PRIVATE
#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODE_SERIALIZER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_module_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_opcodes.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars_memory.h"
#include "utils.h"



START_TEST(test_compiler_ctor)
{
    struct handlebars_compiler * mycompiler;

    mycompiler = handlebars_compiler_ctor(context);

    ck_assert_ptr_ne(NULL, mycompiler);

    handlebars_compiler_dtor(mycompiler);
}
END_TEST

START_TEST(test_program_size_constant)
{
    ck_assert_uint_eq(HANDLEBARS_PROGRAM_SIZE, sizeof(struct handlebars_program));
}
END_TEST

static struct handlebars_string * compiler_nested_template(size_t levels)
{
    struct handlebars_string * tmpl = handlebars_string_init(context, 1024);

    for( size_t i = 0; i < levels; i++ ) {
        tmpl = handlebars_string_append(context, tmpl, HBS_STRL("{{#if a}}"));
    }
    for( size_t i = 0; i < levels; i++ ) {
        tmpl = handlebars_string_append(context, tmpl, HBS_STRL("{{/if}}"));
    }
    return tmpl;
}

START_TEST(test_compiler_nested_program_stack_limit)
{
    struct handlebars_string * tmpl = compiler_nested_template(HANDLEBARS_COMPILER_STACK_SIZE + 1);

    jmp_buf buf;
    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
        return;
    }

    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    (void) program;
    ck_abort_msg("Expected deeply nested programs to hit the compiler stack limit");
}
END_TEST

START_TEST(test_compiler_reusable_after_escaped_error)
{
    struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
    struct handlebars_string * deep_tmpl = compiler_nested_template(HANDLEBARS_COMPILER_STACK_SIZE + 1);
    struct handlebars_ast_node * deep_ast = handlebars_parse_ex(parser, deep_tmpl, 0);
    struct handlebars_parser * local_parser;
    struct handlebars_string * simple_tmpl;
    struct handlebars_ast_node * simple_ast;
    struct handlebars_program * program;
    jmp_buf * prev = context->e->jmp;
    jmp_buf first;
    jmp_buf second;

    if( handlebars_setjmp_ex(context, &first) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_STACK_OVERFLOW);
    } else {
        program = handlebars_compiler_compile_ex(local_compiler, deep_ast);
        (void) program;
        context->e->jmp = prev;
        ck_abort_msg("Expected deeply nested compilation to fail");
    }

    local_parser = handlebars_parser_ctor(context);
    simple_tmpl = handlebars_string_ctor(context, HBS_STRL("text"));
    simple_ast = handlebars_parse_ex(local_parser, simple_tmpl, 0);
    ck_assert_ptr_nonnull(simple_ast);

    if( handlebars_setjmp_ex(context, &second) ) {
        context->e->jmp = prev;
        ck_abort_msg("Compiler reuse failed after an escaped error: %s", handlebars_error_msg(context));
    }

    program = handlebars_compiler_compile_ex(local_compiler, simple_ast);
    context->e->jmp = prev;
    ck_assert_ptr_nonnull(program);
    ck_assert_int_gt(program->opcodes_length, 0);
    handlebars_parser_dtor(local_parser);
    handlebars_compiler_dtor(local_compiler);
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static void assert_program_storage_consistent(struct handlebars_program * program)
{
    ck_assert_uint_le(program->opcodes_length, program->opcodes_size);
    ck_assert_uint_le(program->children_length, program->children_size);
    ck_assert_uint_le(program->decorators_length, program->decorators_size);

    if( program->opcodes_size > 0 ) {
        ck_assert_ptr_nonnull(program->opcodes);
        ck_assert_uint_le(
            program->opcodes_size,
            talloc_get_size(program->opcodes) / sizeof(*program->opcodes)
        );
    }
    if( program->children_size > 0 ) {
        ck_assert_ptr_nonnull(program->children);
        ck_assert_uint_le(
            program->children_size,
            talloc_get_size(program->children) / sizeof(*program->children)
        );
    }
    if( program->decorators_size > 0 ) {
        ck_assert_ptr_nonnull(program->decorators);
        ck_assert_uint_le(
            program->decorators_size,
            talloc_get_size(program->decorators) / sizeof(*program->decorators)
        );
    }

    for( size_t i = 0; i < program->children_length; i++ ) {
        assert_program_storage_consistent(program->children[i]);
    }
    for( size_t i = 0; i < program->decorators_length; i++ ) {
        assert_program_storage_consistent(program->decorators[i]);
    }
}

START_TEST(test_compiler_allocation_failure_preserves_array_capacity)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("{{#if foo}}bar{{/if}}{{*decorator foo}}")
    );

    for( int fail_at = 1; fail_at <= 100; fail_at++ ) {
        struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
        struct handlebars_ast_node * ast = handlebars_parse_ex(local_parser, tmpl, 0);
        struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
        struct handlebars_program * program;

        handlebars_compiler_set_flags(local_compiler, handlebars_compiler_flag_alternate_decorators);
        handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
        handlebars_memory_fail_counter(fail_at);
        program = handlebars_compiler_compile_ex(local_compiler, ast);
        handlebars_memory_fail_disable();

        assert_program_storage_consistent(program);
        handlebars_compiler_dtor(local_compiler);
        handlebars_parser_dtor(local_parser);
    }
}
END_TEST
#endif

START_TEST(test_delimiter_change_requires_close_delimiter)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{=a =}}x"));
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        return;
    }

    struct handlebars_string * processed = handlebars_preprocess_delimiters(context, tmpl, NULL, NULL);
    (void) processed;
    ck_abort_msg("Expected whitespace-only closing delimiter to be rejected");
}
END_TEST

START_TEST(test_serialize_rejects_invalid_child_program)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{#if foo}}bar{{/if}}"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    jmp_buf buf;

    ck_assert_int_gt(program->children_length, 0);
    program->children_length = 0;

    if( handlebars_setjmp_ex(context, &buf) ) {
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_str_eq(handlebars_error_msg(context), "Invalid child program index: 0");
        return;
    }

    struct handlebars_module * module = handlebars_program_serialize(context, program);
    (void) module;
    ck_abort_msg("Expected invalid child program index to be rejected");
}
END_TEST

static void assert_serialize_rejects_program(
    struct handlebars_program * program,
    const char * expected_error
)
{
    struct handlebars_module * module;
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_ERROR);
        ck_assert_str_eq(handlebars_error_msg(context), expected_error);
        return;
    }

    module = handlebars_program_serialize(context, program);
    (void) module;
    context->e->jmp = prev;
    ck_abort_msg("Expected invalid program to be rejected");
}

START_TEST(test_serialize_rejects_invalid_child_array_length)
{
    struct handlebars_program program = {0};
    struct handlebars_program child = {0};
    struct handlebars_program * children[] = {&child};

    program.children = children;
    program.children_length = 2;
    program.children_size = 1;

    assert_serialize_rejects_program(&program, "Invalid child program array");
}
END_TEST

START_TEST(test_serialize_rejects_invalid_opcode_array_length)
{
    struct handlebars_program program = {0};
    struct handlebars_opcode opcode = {0};
    struct handlebars_opcode * opcodes[] = {&opcode};

    program.opcodes = opcodes;
    program.opcodes_length = 2;
    program.opcodes_size = 1;

    assert_serialize_rejects_program(&program, "Invalid opcode array");
}
END_TEST

START_TEST(test_serialize_rejects_self_referential_program)
{
    struct handlebars_program program = {0};
    struct handlebars_program * children[] = {&program};

    program.children = children;
    program.children_length = 1;
    program.children_size = 1;

    assert_serialize_rejects_program(&program, "Cyclic child program reference");
}
END_TEST

START_TEST(test_serialize_rejects_mutually_recursive_programs)
{
    struct handlebars_program programs[2] = {{0}};
    struct handlebars_program * first_children[] = {&programs[1]};
    struct handlebars_program * second_children[] = {&programs[0]};

    programs[0].children = first_children;
    programs[0].children_length = 1;
    programs[0].children_size = 1;
    programs[1].children = second_children;
    programs[1].children_length = 1;
    programs[1].children_size = 1;

    assert_serialize_rejects_program(&programs[0], "Cyclic child program reference");
}
END_TEST

START_TEST(test_serialize_accepts_shared_acyclic_program)
{
    struct handlebars_program root_program = {0};
    struct handlebars_program child = {0};
    struct handlebars_program * children[] = {&child, &child};
    struct handlebars_module * module;

    root_program.children = children;
    root_program.children_length = 2;
    root_program.children_size = 2;

    module = handlebars_program_serialize(context, &root_program);
    ck_assert_uint_eq(module->program_count, 3);
    ck_assert_uint_eq(module->opcode_count, 3);
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, context));
}
END_TEST

static struct handlebars_program * create_deep_program(size_t depth)
{
    struct handlebars_program * programs;
    struct handlebars_program ** children;

    ck_assert_uint_gt(depth, 0);
    programs = handlebars_talloc_array(context, struct handlebars_program, depth);
    ck_assert_ptr_nonnull(programs);
    memset(programs, 0, sizeof(*programs) * depth);
    children = handlebars_talloc_array(context, struct handlebars_program *, depth);
    ck_assert_ptr_nonnull(children);

    for( size_t i = 0; i + 1 < depth; i++ ) {
        children[i] = &programs[i + 1];
        programs[i].children = &children[i];
        programs[i].children_length = 1;
        programs[i].children_size = 1;
    }

    return programs;
}

START_TEST(test_serialize_rejects_deep_program_cycle)
{
    const size_t depth = 65;
    struct handlebars_program * programs = create_deep_program(depth);
    struct handlebars_program ** child = handlebars_talloc_array(
        context,
        struct handlebars_program *,
        1
    );

    ck_assert_ptr_nonnull(child);
    child[0] = &programs[0];
    programs[depth - 1].children = child;
    programs[depth - 1].children_length = 1;
    programs[depth - 1].children_size = 1;

    assert_serialize_rejects_program(&programs[0], "Cyclic child program reference");
}
END_TEST

START_TEST(test_serialize_accepts_deep_shared_acyclic_program)
{
    const size_t depth = 65;
    struct handlebars_program * programs = create_deep_program(depth);
    struct handlebars_program ** children = handlebars_talloc_array(
        context,
        struct handlebars_program *,
        2
    );
    struct handlebars_module * module;

    ck_assert_ptr_nonnull(children);
    children[0] = &programs[depth - 1];
    children[1] = &programs[depth - 1];
    programs[depth - 2].children = children;
    programs[depth - 2].children_length = 2;
    programs[depth - 2].children_size = 2;

    module = handlebars_program_serialize(context, &programs[0]);
    ck_assert_uint_eq(module->program_count, depth + 1);
    ck_assert_uint_eq(module->opcode_count, depth + 1);
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, context));
}
END_TEST

START_TEST(test_serialize_deep_program_without_c_stack_recursion)
{
    const size_t depth = 100000;
    struct handlebars_program * programs = create_deep_program(depth);
    struct handlebars_module * module = handlebars_program_serialize(context, &programs[0]);

    ck_assert_uint_eq(module->program_count, depth);
    ck_assert_uint_eq(module->opcode_count, depth);
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, context));
}
END_TEST

static void assert_serialize_rejects_operand_count(size_t count)
{
    struct handlebars_program program = {0};
    struct handlebars_opcode opcode = {0};
    struct handlebars_opcode * opcodes[] = {&opcode};
    struct handlebars_operand_string item = {0};
    struct handlebars_module * module;
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    program.opcodes = opcodes;
    program.opcodes_length = 1;
    program.opcodes_size = 1;
    opcode.type = handlebars_opcode_type_lookup_on_context;
    opcode.op1.type = handlebars_operand_type_array;
    opcode.op1.data.array.count = count;
    opcode.op1.data.array.array = &item;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_str_eq(handlebars_error_msg(context), "Serialized module is too large");
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        return;
    }

    module = handlebars_program_serialize(context, &program);
    (void) module;
    context->e->jmp = prev;
    ck_abort_msg("Expected oversized operand array to be rejected");
}

START_TEST(test_serialize_rejects_operand_size_multiplication_overflow)
{
    assert_serialize_rejects_operand_count(
        SIZE_MAX / sizeof(struct handlebars_operand_string) + 1
    );
}
END_TEST

START_TEST(test_serialize_rejects_aggregate_size_overflow)
{
    assert_serialize_rejects_operand_count(
        SIZE_MAX / sizeof(struct handlebars_operand_string)
    );
}
END_TEST

#ifdef HANDLEBARS_MEMORY
static void assert_serialize_allocation_failure(
    struct handlebars_program * program,
    int fail_at
)
{
    struct handlebars_module * module;
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_memory_fail_disable();
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        return;
    }

    handlebars_memory_fail_set_flags(handlebars_memory_fail_flag_alloc);
    handlebars_memory_fail_counter(fail_at);
    module = handlebars_program_serialize(context, program);
    (void) module;
    handlebars_memory_fail_disable();
    context->e->jmp = prev;
    ck_abort_msg("Expected serialized module allocation %d to fail", fail_at);
}

START_TEST(test_serialize_allocation_failures)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(
        context,
        HBS_STRL("{{#if foo}}bar{{/if}}")
    );
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);

    assert_serialize_allocation_failure(program, 1);
    assert_serialize_allocation_failure(program, 2);
    assert_serialize_allocation_failure(program, 3);
}
END_TEST

START_TEST(test_serialize_traversal_allocation_failures)
{
    struct handlebars_program * programs = create_deep_program(97);

    assert_serialize_allocation_failure(&programs[0], 2);
    assert_serialize_allocation_failure(&programs[0], 3);
    assert_serialize_allocation_failure(&programs[0], 4);
    assert_serialize_allocation_failure(&programs[0], 5);
    assert_serialize_allocation_failure(&programs[0], 6);
}
END_TEST
#endif

static struct handlebars_module * serialize_for_verification_flags(
    const char * source,
    unsigned long flags
)
{
    struct handlebars_parser * local_parser = handlebars_parser_ctor(context);
    struct handlebars_compiler * local_compiler = handlebars_compiler_ctor(context);
    struct handlebars_string * tmpl = handlebars_string_ctor(context, source, strlen(source));
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    struct handlebars_module * module;

    handlebars_compiler_set_flags(local_compiler, flags);
    ast = handlebars_parse_ex(local_parser, tmpl, 0);
    program = handlebars_compiler_compile_ex(local_compiler, ast);
    module = handlebars_program_serialize(context, program);

    handlebars_compiler_dtor(local_compiler);
    handlebars_parser_dtor(local_parser);
    return module;
}

static struct handlebars_module * serialize_for_verification(const char * source)
{
    return serialize_for_verification_flags(source, 0);
}

START_TEST(test_serialized_module_verification)
{
    struct handlebars_module * module = serialize_for_verification(
        "{{#if foo}}{{foo.bar}}{{else}}empty{{/if}}"
    );
    struct handlebars_module * copy;
    struct handlebars_string * printed;
    size_t size = module->size;

    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, NULL));
    ck_assert(handlebars_module_verify_ex(module, size, NULL));
    ck_assert(!handlebars_module_verify_ex(module, size - 1, NULL));

    copy = handlebars_talloc_size(context, size);
    memcpy(copy, module, size);
    handlebars_module_patch_pointers(copy);
    handlebars_module_patch_pointers(copy);
    handlebars_module_normalize_pointers(copy, NULL);
    handlebars_module_generate_hash(copy);
    ck_assert(handlebars_module_verify_ex(copy, size, NULL));

    module = serialize_for_verification_flags(
        "{{foo}}",
        handlebars_compiler_flag_mustache_style_lambdas
    );
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, NULL));

    module = serialize_for_verification(
        "{{#if a}}{{#if b}}x{{/if}}{{/if}}{{#if c}}y{{/if}}"
    );
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, NULL));
    printed = handlebars_module_print(context, module);
    {
        const char * program1 = strstr(hbs_str_val(printed), "\nPROGRAM: 1\nOP[");
        const char * program2 = strstr(hbs_str_val(printed), "\nPROGRAM: 2\nOP[");
        const char * program3 = strstr(hbs_str_val(printed), "\nPROGRAM: 3\nOP[");

        ck_assert_ptr_nonnull(program1);
        ck_assert_ptr_nonnull(program2);
        ck_assert_ptr_nonnull(program3);
        ck_assert(program1 < program3);
        ck_assert(program3 < program2);
    }

    module = serialize_for_verification("{{#if foo}}x{{/if}}");
    {
        size_t opcode_count = module->programs[0].opcode_count;
        size_t opcode_offset = module->programs[0].opcode_offset;

        module->programs[0].opcode_count = module->programs[1].opcode_count;
        module->programs[0].opcode_offset = module->programs[1].opcode_offset;
        module->programs[1].opcode_count = opcode_count;
        module->programs[1].opcode_offset = opcode_offset;
    }
    handlebars_module_generate_hash(module);
    ck_assert(handlebars_module_verify(module, NULL));
    printed = handlebars_module_print(context, module);
    {
        const char * program0 = strstr(hbs_str_val(printed), "\nPROGRAM: 0\nOP[");
        const char * program1 = strstr(hbs_str_val(printed), "\nPROGRAM: 1\nOP[");

        ck_assert_ptr_nonnull(program0);
        ck_assert_ptr_nonnull(program1);
        ck_assert(program1 < program0);
    }
}
END_TEST

START_TEST(test_serialized_module_rejects_invalid_layout)
{
    struct handlebars_module * module = serialize_for_verification("{{foo.bar}}");
    struct handlebars_operand * operands;
    bool found_array = false;
    bool found_optional_boolean = false;

    module->programs = (void *) ((unsigned char *) module->programs + sizeof(void *));
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));

    module = serialize_for_verification("{{foo.bar}}");
    module->programs[0].opcode_count++;
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));

    module = serialize_for_verification("{{foo.bar}}");
    module->opcodes[0].type = handlebars_opcode_type_invalid;
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));

    module = serialize_for_verification("{{foo.bar}}");
    for( size_t i = 0; i < module->opcode_count && !found_array; i++ ) {
        operands = &module->opcodes[i].op1;
        for( size_t j = 0; j < 4; j++ ) {
            if( operands[j].type == handlebars_operand_type_array ) {
                operands[j].data.array.count = SIZE_MAX;
                found_array = true;
                break;
            }
        }
    }
    ck_assert(found_array);
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));

    module = serialize_for_verification("text");
    ck_assert_int_eq(module->opcodes[0].type, handlebars_opcode_type_append_content);
    ck_assert_int_eq(module->opcodes[0].op1.type, handlebars_operand_type_string);
    module->opcodes[0].op1.data.string.string = (void *) (
        (unsigned char *) module->opcodes[0].op1.data.string.string + sizeof(void *)
    );
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));

    module = serialize_for_verification("{{foo.bar}}");
    for( size_t i = 0; i < module->opcode_count; i++ ) {
        if( module->opcodes[i].type == handlebars_opcode_type_lookup_on_context
                && module->opcodes[i].op2.type == handlebars_operand_type_null ) {
            module->opcodes[i].op2.data.boolval = true;
            found_optional_boolean = true;
            break;
        }
    }
    ck_assert(found_optional_boolean);
    handlebars_module_generate_hash(module);
    ck_assert(!handlebars_module_verify(module, NULL));
}
END_TEST

START_TEST(test_known_helpers_only_rejects_parent_path)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{..f}}"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program;

    ck_assert_ptr_ne(NULL, ast);
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_known_helpers_only);
    program = handlebars_compiler_compile_ex(compiler, ast);

    ck_assert_ptr_ne(NULL, program);
    ck_assert_int_eq(HANDLEBARS_UNKNOWN_HELPER, handlebars_error_num(context));
    ck_assert_msg(
        strstr(handlebars_error_msg(context), "unknown helper ..") != NULL,
        "Unexpected error message: %s",
        handlebars_error_msg(context)
    );
}
END_TEST

START_TEST(test_string_params_supports_implicit_partial_context)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{> foo}}"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program;

    ck_assert_ptr_ne(NULL, ast);
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_string_params);
    program = handlebars_compiler_compile_ex(compiler, ast);

    ck_assert_ptr_ne(NULL, program);
    ck_assert_int_eq(HANDLEBARS_SUCCESS, handlebars_error_num(context));
}
END_TEST

START_TEST(test_alternate_decorator_compiler_inherits_state)
{
    struct handlebars_string * tmpl = handlebars_string_ctor(context, HBS_STRL("{{*decorator foo}}"));
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, tmpl, 0);
    struct handlebars_program * program;

    ck_assert_ptr_ne(NULL, ast);
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_alternate_decorators);
    program = handlebars_compiler_compile_ex(compiler, ast);

    ck_assert_ptr_ne(NULL, program);
    ck_assert_int_eq(HANDLEBARS_SUCCESS, handlebars_error_num(context));
    ck_assert_int_eq(1, program->decorators_length);
}
END_TEST

START_TEST(test_compiler_ctor_failed_alloc)
{
#ifdef HANDLEBARS_MEMORY
    jmp_buf buf;
    struct handlebars_compiler * mycompiler;

    context->e->jmp = &buf;
    if( setjmp(buf) ) {
        ck_assert(1);
        return;
    }

    handlebars_memory_fail_enable();
    mycompiler = handlebars_compiler_ctor(context);
    (void) mycompiler;
    handlebars_memory_fail_disable();

    ck_assert(0);
#else
    fprintf(stderr, "Skipped, memory testing functions are disabled\n");
#endif
}
END_TEST

START_TEST(test_compiler_dtor)
{
    struct handlebars_compiler * mycompiler;

    mycompiler = handlebars_compiler_ctor(context);
    handlebars_compiler_dtor(mycompiler);
}
END_TEST

START_TEST(test_compiler_get_flags)
{
    ck_assert_int_eq(0, handlebars_compiler_get_flags(compiler));

    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_all);

    ck_assert_int_eq(handlebars_compiler_flag_all, handlebars_compiler_get_flags(compiler));
}
END_TEST

START_TEST(test_compiler_set_flags)
{
    // Make sure it changes option flags
    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_string_params);
    ck_assert_int_eq(handlebars_compiler_flag_string_params, handlebars_compiler_get_flags(compiler));

    handlebars_compiler_set_flags(compiler, handlebars_compiler_flag_track_ids);
    ck_assert_int_eq(handlebars_compiler_flag_track_ids, handlebars_compiler_get_flags(compiler));
}
END_TEST

#ifdef HANDLEBARS_TESTING_EXPORTS
START_TEST(test_compiler_is_known_helper)
{
    struct handlebars_ast_node * id;
    struct handlebars_ast_list * parts;
    struct handlebars_ast_node * path_segment;

    //ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, NULL));

    id = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    id->node.path.parts = parts = handlebars_ast_list_ctor(HBSCTX(parser));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment = handlebars_ast_node_ctor(HBSCTX(parser), HANDLEBARS_AST_NODE_PATH);
    handlebars_ast_list_append(parts, path_segment);
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("if"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("unless"));
    ck_assert_int_eq(1, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL("foobar"));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));

    path_segment->node.path_segment.part = handlebars_string_ctor(HBSCTX(compiler), HBS_STRL(""));
    ck_assert_int_eq(0, handlebars_compiler_is_known_helper(compiler, id));
}
END_TEST

START_TEST(test_compiler_opcode)
{
    struct handlebars_opcode * op1;
    struct handlebars_opcode * op2;
    struct handlebars_program *program;

    op1 = handlebars_opcode_ctor(HBSCTX(compiler), handlebars_opcode_type_append);
    op2 = handlebars_opcode_ctor(HBSCTX(compiler), handlebars_opcode_type_append_escaped);

    handlebars_compiler_opcode(compiler, op1);
    program = handlebars_compiler_get_program(compiler);

    ck_assert_ptr_ne(NULL, program->opcodes);
    ck_assert_int_ne(0, program->opcodes_size);
    ck_assert_int_eq(1, program->opcodes_length);
    ck_assert_ptr_eq(op1, *program->opcodes);

    handlebars_compiler_opcode(compiler, op2);
    program = handlebars_compiler_get_program(compiler);

    ck_assert_ptr_ne(NULL, program->opcodes);
    ck_assert_int_ne(0, program->opcodes_size);
    ck_assert_int_eq(2, program->opcodes_length);
    ck_assert_ptr_eq(op2, *(program->opcodes + 1));
}
END_TEST

START_TEST(test_compiler_opcode_rejects_capacity_overflow)
{
    struct handlebars_program * program = handlebars_compiler_get_program(compiler);
    struct handlebars_opcode * opcode = handlebars_opcode_ctor(
        HBSCTX(compiler),
        handlebars_opcode_type_append
    );
    jmp_buf * prev = context->e->jmp;
    jmp_buf buf;

    program->opcodes_size = UINT_MAX;
    program->opcodes_length = UINT_MAX;

    if( handlebars_setjmp_ex(context, &buf) ) {
        context->e->jmp = prev;
        ck_assert_int_eq(handlebars_error_num(context), HANDLEBARS_NOMEM);
        program->opcodes_size = 0;
        program->opcodes_length = 0;
        return;
    }

    handlebars_compiler_opcode(compiler, opcode);
    context->e->jmp = prev;
    ck_abort_msg("Expected overflowing opcode capacity to be rejected");
}
END_TEST
#endif

static Suite * suite(void);
static Suite * suite(void)
{
    Suite * s = suite_create("Compiler");

	REGISTER_TEST_FIXTURE(s, test_compiler_ctor, "Constructor");
	REGISTER_TEST_FIXTURE(s, test_program_size_constant, "Program size constant");
	REGISTER_MEMORY_TEST_FIXTURE(s, test_compiler_ctor_failed_alloc, "Constructor (failed alloc)");
	REGISTER_TEST_FIXTURE(s, test_compiler_dtor, "Destructor");
	REGISTER_TEST_FIXTURE(s, test_compiler_get_flags, "Get Flags");
	REGISTER_TEST_FIXTURE(s, test_compiler_set_flags, "Set Flags");
	REGISTER_TEST_FIXTURE(s, test_compiler_nested_program_stack_limit, "Nested program stack limit");
	REGISTER_TEST_FIXTURE(s, test_compiler_reusable_after_escaped_error, "Compiler reuse after escaped error");
#ifdef HANDLEBARS_MEMORY
	REGISTER_TEST_FIXTURE(s, test_compiler_allocation_failure_preserves_array_capacity, "Compiler allocation failure preserves array capacity");
#endif
	REGISTER_TEST_FIXTURE(s, test_delimiter_change_requires_close_delimiter, "Delimiter change requires close delimiter");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_invalid_child_program, "Reject invalid child program index");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_invalid_child_array_length, "Reject invalid child array length");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_invalid_opcode_array_length, "Reject invalid opcode array length");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_self_referential_program, "Reject self-referential program");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_mutually_recursive_programs, "Reject mutually recursive programs");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_deep_program_cycle, "Reject deeply nested program cycle");
	REGISTER_TEST_FIXTURE(s, test_serialize_accepts_shared_acyclic_program, "Accept shared acyclic program");
	REGISTER_TEST_FIXTURE(s, test_serialize_accepts_deep_shared_acyclic_program, "Accept deeply nested shared acyclic program");
	REGISTER_TEST_FIXTURE(s, test_serialize_deep_program_without_c_stack_recursion, "Serialize deeply nested program without C stack recursion");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_operand_size_multiplication_overflow, "Reject overflowing operand array size");
	REGISTER_TEST_FIXTURE(s, test_serialize_rejects_aggregate_size_overflow, "Reject overflowing serialized module size");
#ifdef HANDLEBARS_MEMORY
	REGISTER_TEST_FIXTURE(s, test_serialize_allocation_failures, "Serialized module allocation failures");
	REGISTER_TEST_FIXTURE(s, test_serialize_traversal_allocation_failures, "Serialized module traversal allocation failures");
#endif
	REGISTER_TEST_FIXTURE(s, test_serialized_module_verification, "Verify serialized module layout");
	REGISTER_TEST_FIXTURE(s, test_serialized_module_rejects_invalid_layout, "Reject invalid serialized module layout");
	REGISTER_TEST_FIXTURE(s, test_known_helpers_only_rejects_parent_path, "Reject parent path as unknown helper");
	REGISTER_TEST_FIXTURE(s, test_string_params_supports_implicit_partial_context, "String params with implicit partial context");
	REGISTER_TEST_FIXTURE(s, test_alternate_decorator_compiler_inherits_state, "Alternate decorator compiler state");
#ifdef HANDLEBARS_TESTING_EXPORTS
	REGISTER_TEST_FIXTURE(s, test_compiler_is_known_helper, "Is Known Helper");
	REGISTER_TEST_FIXTURE(s, test_compiler_opcode, "Push opcode");
	REGISTER_TEST_FIXTURE(s, test_compiler_opcode_rejects_capacity_overflow, "Reject overflowing opcode capacity");
#endif

    return s;
}

int main(void)
{
    return default_main(&suite);
}

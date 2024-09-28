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

#include <assert.h>
#include <errno.h>
#include <check.h>
#include <stdio.h>
#include <talloc.h>

// json-c undeprecated json_object_object_get, but the version in xenial
// is too old, so let's silence deprecated warnings for json-c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <json.h>
#include <json_object.h>
#include <json_tokener.h>
#pragma GCC diagnostic pop

#define HANDLEBARS_COMPILER_PRIVATE
#define HANDLEBARS_OPCODES_PRIVATE

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_parser.h"
#include "handlebars_string.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

#if (__GNUC__ >= 3) || defined(__clang__)
typedef __typeof__(json_object_array_length(NULL)) json_array_length_t;
#else
typedef int json_array_length_t;
#endif


struct compiler_test {
    const char * suite_name;
    char * description;
    char * it;
    char * tmpl;
    struct handlebars_string * expected;
    int exception;
    char * message;
    char ** known_helpers;

    bool opt_compat;
    bool opt_data;
    bool opt_known_helpers_only;
    bool opt_string_params;
    bool opt_track_ids;
    bool opt_prevent_indent;
    bool opt_explicit_partial_context;
    bool opt_ignore_standalone;
    bool strict;
    bool assume_objects;
    long flags;

    struct handlebars_context * ctx;
};

static const char * suite_names[] = {
  "basic", "blocks", "builtins", "data", "helpers", "partials",
  "regressions", "strict", "string-params", "subexpressions", "track-ids",
  "whitespace-control", NULL
};

static struct compiler_test * tests = NULL;
static size_t tests_len = 0;
static size_t tests_size = 0;
static const char * export_dir = NULL;


static int loadTestOpcodeOperand(
    struct handlebars_context * test_context,
    struct handlebars_program * program,
    struct handlebars_opcode * opcode,
    struct handlebars_operand * operand,
    json_object * object
) {
    struct handlebars_string * str;
    if( !object ) {
        return 0;
    }
    switch( json_object_get_type(object) ) {
        case json_type_null:
            handlebars_operand_set_null(operand);
            break;
        case json_type_boolean:
            handlebars_operand_set_boolval(operand, json_object_get_boolean(object) ? true : false);
            break;
        case json_type_string:
            str = handlebars_string_ctor(test_context, json_object_get_string(object), json_object_get_string_len(object));
            handlebars_operand_set_stringval(test_context, opcode, operand, str);
            break;
        case json_type_int:
            handlebars_operand_set_longval(operand, json_object_get_int(object));
            break;
        case json_type_array: {
            json_array_length_t array_len = json_object_array_length(object);
            char ** arr = handlebars_talloc_array(opcode, char *, array_len + 1);
            char ** arrptr = arr;
            json_object * array_item;

            // Iterate over array
            for( json_array_length_t i = 0; i < array_len; i++ ) {
                array_item = json_object_array_get_idx(object, i);
                *arrptr++ = handlebars_talloc_strdup(opcode, json_object_get_string(array_item));
            }
            *arrptr++ = NULL;
            handlebars_operand_set_arrayval(test_context, opcode, operand, (const char **) arr);
            break;
        }
        case json_type_double: {
            char tmp[64];
            snprintf(tmp, 63, "%g", json_object_get_double(object));
            str = handlebars_string_ctor(test_context, tmp, strlen(tmp));
            handlebars_operand_set_stringval(test_context, opcode, operand, str);
            //handlebars_operand_set_stringval(opcode, operand, json_object_get_string(object));
            break;
        }
        default:
            return 1;
            break;
    }

    return 0;
}

static int loadTestOpcodeLoc(
    struct handlebars_context * test_context,
    struct handlebars_program * program,
    struct handlebars_opcode * opcode,
    json_object * object
) {
    struct json_object * cur = NULL;
    struct json_object * line = NULL;
    struct json_object * column = NULL;

    // Get start
    cur = json_object_object_get(object, "start");
    if( !cur || json_object_get_type(cur) != json_type_object ) {
        fprintf(stderr, "Opcode loc start was not an object!\n");
        goto error;
    } else {
        line = json_object_object_get(cur, "line");
        column = json_object_object_get(cur, "column");
        if( line && column ) {
            opcode->loc.first_line = json_object_get_int(line);
            opcode->loc.first_column = json_object_get_int(column);
        } else {
            fprintf(stderr, "Opcode loc start was missing line or column!\n");
            goto error;
        }
    }

    // Get end
    cur = json_object_object_get(object, "end");
    if( !cur || json_object_get_type(cur) != json_type_object ) {
        fprintf(stderr, "Opcode loc end was not an object!\n");
        goto error;
    } else {
        line = json_object_object_get(cur, "line");
        column = json_object_object_get(cur, "column");
        if( line && column ) {
            opcode->loc.last_line = json_object_get_int(line);
            opcode->loc.last_column = json_object_get_int(column);
        } else {
            fprintf(stderr, "Opcode loc end was missing line or column!\n");
            goto error;
        }
    }

    return 0;

error:
    return 1;
}

static struct handlebars_opcode * loadTestOpcode(
    struct handlebars_context * test_context,
    struct handlebars_program * program,
    json_object * object
) {
    struct handlebars_opcode * opcode = NULL;
    enum handlebars_opcode_type type;
    struct json_object * array_item = NULL;
    struct json_object * cur = NULL;
    json_array_length_t array_len = 0;

    // Get type
    cur = json_object_object_get(object, "opcode");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        type = handlebars_opcode_reverse_readable_type(json_object_get_string(cur));
        if( type < 0 ) {
            fprintf(stderr, "Unknown opcode: %s\n", json_object_get_string(cur));
        }
    } else {
        fprintf(stderr, "Opcode was not a string!\n");
        goto error;
    }

    // Construct opcode
    opcode = handlebars_opcode_ctor(test_context, type);

    // Get args
    cur = json_object_object_get(object, "args");
    if( !cur || json_object_get_type(cur) != json_type_array ) {
        fprintf(stderr, "Opcode args was not an array!\n");
        goto error;
    }

    array_len = json_object_array_length(cur);

    switch( array_len ) {
		case 4: {
            array_item = json_object_array_get_idx(cur, 3);
            loadTestOpcodeOperand(test_context, program, opcode, &opcode->op4, array_item);
		}
        /* fallthrough */
        case 3: {
            array_item = json_object_array_get_idx(cur, 2);
            loadTestOpcodeOperand(test_context, program, opcode, &opcode->op3, array_item);
        }
        /* fallthrough */
        case 2: {
            array_item = json_object_array_get_idx(cur, 1);
            loadTestOpcodeOperand(test_context, program, opcode, &opcode->op2, array_item);
        }
        /* fallthrough */
        case 1: {
            array_item = json_object_array_get_idx(cur, 0);
            loadTestOpcodeOperand(test_context, program, opcode, &opcode->op1, array_item);
            break;
        }
        case 0:
        default:
            break;
    }

    // Get loc
    cur = json_object_object_get(object, "loc");
    if( !cur || json_object_get_type(cur) != json_type_object ) {
    	// In v4, I guess they don't always have locations
        //fprintf(stderr, "Opcode loc was not an object!\n");
        //goto error;
    } else {
        loadTestOpcodeLoc(test_context, program, opcode, cur);
    }

error:
    return opcode;
}

/*
static int loadTestCompilerDepths(struct handlebars_compiler * compiler, json_object * object)
{
    json_array_length_t array_len = json_object_array_length(object);
    struct json_object * array_item = NULL;
    unsigned long depths = 0;
    for( json_array_length_t i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(object, i);
        int32_t v = json_object_get_int(array_item);
        depths += (1 << v - 1);
    }
    compiler->depths = depths;
}
*/

static int loadTestProgram(struct handlebars_context * test_context, struct handlebars_program * program, json_object * object)
{
    int error = 0;
    json_object * cur = NULL;
    struct json_object * array_item = NULL;
    json_array_length_t array_len = 0;

    // Load depths
    /*
    cur = json_object_object_get(object, "depths");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        cur = json_object_object_get(cur, "list");
        if( cur && json_object_get_type(cur) == json_type_array ) {
            loadTestCompilerDepths(compiler, cur);
        } else {
            fprintf(stderr, "No depth list!\n");
        }
    } else {
        fprintf(stderr, "No depth info!\n");
    }
    */

    // Load opcodes
    cur = json_object_object_get(object, "opcodes");
    if( !cur || json_object_get_type(cur) != json_type_array ) {
        fprintf(stderr, "Opcodes was not an array!\n");
        goto error;
    }

    // Get number of opcodes
    array_len = json_object_array_length(cur);

    // Allocate opcodes array
    program->opcodes_size += array_len;
    program->opcodes = talloc_zero_array(program, struct handlebars_opcode *, program->opcodes_size);

    // Iterate over array
    for( json_array_length_t i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(cur, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: opcode was not an object, was: %s\n", json_object_get_string(array_item));
            continue;
        }

        program->opcodes[program->opcodes_length++] = loadTestOpcode(test_context, program, array_item);
    }

    // Load children
    cur = json_object_object_get(object, "children");
    if( !cur || json_object_get_type(cur) != json_type_array ) {
        fprintf(stderr, "Children was not an array!\n");
        goto error;
    }

    // Get number of children
    array_len = json_object_array_length(cur);

    // Allocate children array
    program->children_size += array_len;
    program->children = talloc_zero_array(program, struct handlebars_program *, program->children_size);

    // Iterate over array
    for( json_array_length_t i = 0; i < array_len; i++ ) {
        struct handlebars_program * subprogram = handlebars_talloc_zero(program, struct handlebars_program);

        array_item = json_object_array_get_idx(cur, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: child was not an object, was: %s\n", json_object_get_string(array_item));
            continue;
        }

        program->children[program->children_length++] = subprogram;
        loadTestProgram(test_context, subprogram, array_item);
    }

error:
    return error;
}

static struct handlebars_string * loadTestOpcodesPrint(json_object * object)
{
    struct handlebars_context * test_context;
    struct handlebars_program * program;
    struct handlebars_string * output;

    test_context = handlebars_context_ctor_ex(tests);
    program = handlebars_talloc_zero(test_context, struct handlebars_program);

    loadTestProgram(test_context, program, object);

    output = handlebars_program_print(test_context, program, 0);
    output = talloc_steal(tests, output);

    handlebars_context_dtor(test_context);
    return output;
}

static int loadSpecTestKnownHelpers(struct compiler_test * test, json_object * object)
{
    // Let's just allocate a nice fat array >.>
    char ** known_helpers = talloc_zero_array(tests, char *, 32);
    char ** ptr = known_helpers;
    const char ** ptr2 = handlebars_builtins_names();

    for( ; *ptr2 ; ++ptr2 ) {
        *ptr = handlebars_talloc_strdup(tests, *ptr2);
        ptr++;
    }

    json_object_object_foreach(object, key, value) {
        *ptr = handlebars_talloc_strdup(tests, key);
        (void) value;
        ptr++;
    }

    *ptr++ = NULL;

    test->known_helpers = known_helpers;

    return 0;
}

static int loadSpecTestCompileOptions(struct compiler_test * test, json_object * object)
{
    json_object * cur = NULL;

    // Get compat
    cur = json_object_object_get(object, "compat");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_compat = json_object_get_boolean(cur);
        if( test->opt_compat ) {
            test->flags |= handlebars_compiler_flag_compat;
        }
    }

    // Get data
    cur = json_object_object_get(object, "data");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_data = json_object_get_boolean(cur);
        //test->flags |= handlebars_compiler_flag_compat;
    }

    // Get knownHelpersOnly
    cur = json_object_object_get(object, "knownHelpersOnly");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_known_helpers_only = json_object_get_boolean(cur);
        if( test->opt_known_helpers_only ) {
            test->flags |= handlebars_compiler_flag_known_helpers_only;
        }
    }

    // Get string params
    cur = json_object_object_get(object, "stringParams");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_string_params = json_object_get_boolean(cur);
        if( test->opt_string_params ) {
            test->flags |= handlebars_compiler_flag_string_params;
        }
    }

    // Get track ids
    cur = json_object_object_get(object, "trackIds");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_track_ids = json_object_get_boolean(cur);
        if( test->opt_track_ids ) {
            test->flags |= handlebars_compiler_flag_track_ids;
        }
    }

    // Get prevent indent
    cur = json_object_object_get(object, "preventIndent");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_prevent_indent = json_object_get_boolean(cur);
        if( test->opt_prevent_indent ) {
            test->flags |= handlebars_compiler_flag_prevent_indent;
        }
    }

    // Get explicit partial context
    cur = json_object_object_get(object, "explicitPartialContext");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_explicit_partial_context = json_object_get_boolean(cur);
        if( test->opt_explicit_partial_context ) {
            test->flags |= handlebars_compiler_flag_explicit_partial_context;
        }
    }

    // Get explicit partial context
    cur = json_object_object_get(object, "ignoreStandalone");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->opt_ignore_standalone = json_object_get_boolean(cur);
        if( test->opt_ignore_standalone ) {
            test->flags |= handlebars_compiler_flag_ignore_standalone;
        }
    }

    // Get explicit partial context
    cur = json_object_object_get(object, "strict");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->strict = json_object_get_boolean(cur);
        if( test->strict ) {
            test->flags |= handlebars_compiler_flag_strict;
        }
    }

    // Get explicit partial context
    cur = json_object_object_get(object, "assumeObjects");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->assume_objects = json_object_get_boolean(cur);
        if( test->assume_objects ) {
            test->flags |= handlebars_compiler_flag_assume_objects;
        }
    }

    // Get known helpers
    cur = json_object_object_get(object, "knownHelpers");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        loadSpecTestKnownHelpers(test, cur);
    }

    return 0;
}

static int loadSpecTest(const char * name, json_object * object)
{
    json_object * cur = NULL;
    int nreq = 0;

    // Get test
    struct compiler_test * test = &(tests[tests_len++]);
    memset(test, 0, sizeof(struct compiler_test));
    test->suite_name = name;
    test->known_helpers = NULL;
    test->flags = 0;

    // Get description
    cur = json_object_object_get(object, "description");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->description = handlebars_talloc_strdup(tests, json_object_get_string(cur));
    }

    // Get it
    cur = json_object_object_get(object, "it");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->it = handlebars_talloc_strdup(tests, json_object_get_string(cur));
    }

    // Get template
    cur = json_object_object_get(object, "template");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->tmpl = handlebars_talloc_strdup(tests, json_object_get_string(cur));
    }

    // Get opcodes
    cur = json_object_object_get(object, "opcodes");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        test->expected = loadTestOpcodesPrint(cur);
        nreq++;
    }

    // Get exception
    cur = json_object_object_get(object, "exception");
    if( cur && json_object_get_type(cur) == json_type_boolean ) {
        test->exception = (int) json_object_get_boolean(cur);
        nreq++;
    } else {
        test->exception = 0;
    }

    // Get message
    cur = json_object_object_get(object, "message");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->message = handlebars_talloc_strdup(tests, json_object_get_string(cur));
        nreq++;
    }

    // Get compile options
    cur = json_object_object_get(object, "compileOptions");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        loadSpecTestCompileOptions(test, cur);
    }

    // Get helpers
    /*cur = json_object_object_get(object, "helpers");
    if( cur && json_object_get_type(cur) == json_type_object ) {
        loadSpecTestHelpers(test, cur);
    }*/

    // Check
    if( nreq <= 0 ) {
        fprintf(stderr, "Warning: expected or exception/message must be specified\n");
    }

    return 0;
}

static int loadSpec(const char * name)
{
    char * filename = handlebars_talloc_asprintf(tests, "%s/%s.json", export_dir, name);
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    json_array_length_t array_len = 0;

    // Read JSON file
    error = file_get_contents((const char *) filename, &data, &data_len);
    if( error != 0 ) {
        fprintf(stderr, "Failed to read spec file: %s, code: %d\n", filename, error);
        goto error;
    }

    // Parse JSON
    result = json_tokener_parse(data);
    // @todo: parsing errors seem to cause segfaults....
    if( result == NULL ) {
        fprintf(stderr, "Failed so parse JSON\n");
        error = 1;
        goto error;
    }

    // Root object should be array
    if( json_object_get_type(result) != json_type_array ) {
        fprintf(stderr, "Root JSON value was not array\n");
        error = 1;
        goto error;
    }

    // Get number of test cases
    array_len = json_object_array_length(result);

    // (Re)allocate tests array
    tests_size += array_len;
    tests = talloc_realloc(root, tests, struct compiler_test, tests_size);

    // Iterate over array
    for( json_array_length_t i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(result, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: test case was not an object\n");
            continue;
        }
        loadSpecTest(name, array_item);
    }

error:
    if( filename ) {
        handlebars_talloc_free(filename);
    }
    if( data ) {
        free(data);
    }
    HBS_TEST_JSON_DTOR(tests, result);
    return error;
}

static int loadAllSpecs(void)
{
    const char ** suite_name_ptr;
    int error = 0;

    for( suite_name_ptr = suite_names; *suite_name_ptr != NULL; suite_name_ptr++ ) {
        error = loadSpec(*suite_name_ptr);
        if( error != 0 ) {
            break;
        }
    }

    return error;
}

START_TEST(handlebars_spec_compiler)
{
    struct compiler_test * test = &tests[_i];
    struct handlebars_string * actual;

    // NOTE: works but handlebars.js doesn't concatenate adjacent content blocks
    if( 0 == strcmp(test->it, "escaping") && 0 == strcmp(test->description, "basic context") ) {
    	return;
    } else if( 0 == strcmp(test->it, "helper for nested raw block gets raw content") ) {
    	return;
    }

    // Parse
    struct handlebars_ast_node * ast = handlebars_parse_ex(parser, handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl)), test->flags);

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    if( test->known_helpers ) {
        handlebars_compiler_set_known_helpers(compiler, (const char **) test->known_helpers);
    }

    struct handlebars_program * program = handlebars_compiler_compile_ex(compiler, ast);
    ck_assert_int_eq(0, handlebars_error_num(context));

    // Printer
    actual = handlebars_program_print(context, program, 0);

    // Check
    /*if( test->exception ) {
       ck_assert_int_ne(0, compiler->errnum);
    } else {*/
        //ck_assert_str_eq(printer->output, test->expected);
        if (!handlebars_string_eq(actual, test->expected)) {
            char * tmp = handlebars_talloc_asprintf(tests,
                "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                test->suite_name,
                test->description, test->it, test->flags,
                test->tmpl, hbs_str_val(test->expected), hbs_str_val(actual));
            ck_abort_msg("%s", tmp);
        }
    /* } */
}
END_TEST

static Suite * suite(void);
static Suite * suite(void)
{
    // Load the spec
    if( 0 != loadAllSpecs() ) {
        abort();
    }
    fprintf(stderr, "Loaded %zu test cases\n", tests_len);

    // Setup the suite
    const char * title = "Handlebars Compiler Spec";
    Suite * s = suite_create(title);

    TCase * tc_handlebars_spec_compiler = tcase_create(title);
    tcase_add_checked_fixture(tc_handlebars_spec_compiler, default_setup, default_teardown);
    tcase_add_loop_test(tc_handlebars_spec_compiler, handlebars_spec_compiler, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_compiler);

    return s;
}

int main(int argc, char *argv[])
{
    // Get the export dir
    export_dir = getenv("handlebars_export_dir");
    if( export_dir == NULL && argc >= 2 ) {
        export_dir = argv[1];
    }
    if( export_dir == NULL ) {
        export_dir = "./spec/handlebars/export";
    }

    // Run the suite
    return default_main(&suite);
}


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <check.h>
#include <stdio.h>
#include <talloc.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_string.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"
#include "utils.h"

struct compiler_test {
    const char * suite_name;
    char * description;
    char * it;
    char * tmpl;
    char * expected;
    int exception;
    char * message;
    char ** known_helpers;
    
    short opt_compat;
    short opt_data;
    short opt_known_helpers_only;
    short opt_string_params;
    short opt_track_ids;
    short opt_prevent_indent;
    short opt_explicit_partial_context;
    short opt_ignore_standalone;
    long flags;

    struct handlebars_context * ctx;
};

static const char * suite_names[] = {
  "basic", "blocks", "builtins", "data", "helpers", "partials",
  "regressions", "string-params", "subexpressions", "track-ids",
  "whitespace-control", NULL
};

static TALLOC_CTX * rootctx = NULL;
static struct compiler_test * tests = NULL;
static size_t tests_len = 0;
static size_t tests_size = 0;
static char * export_dir = NULL;
static const int opcode_printer_flags = 0; //handlebars_opcode_printer_flag_locations;


static int loadTestOpcodeOperand(
        struct handlebars_compiler * compiler,
        struct handlebars_opcode * opcode,
        struct handlebars_operand * operand,
        json_object * object
) {
    if( !object ) {
        return 0;
    }
    switch( json_object_get_type(object) ) {
        case json_type_null:
            handlebars_operand_set_null(operand);
            break;
        case json_type_boolean:
            handlebars_operand_set_boolval(operand, json_object_get_boolean(object) ? 1 : 0);
            break;
        case json_type_string:
            handlebars_operand_set_stringval(compiler, operand, json_object_get_string(object));
            break;
        case json_type_int:
            handlebars_operand_set_longval(operand, json_object_get_int(object));
            break;
        case json_type_array: {
            size_t array_len = json_object_array_length(object);
            char ** arr = handlebars_talloc_array(opcode, char *, array_len + 1);
            char ** arrptr = arr;
            json_object * array_item;
            
            // Iterate over array
            for( int i = 0; i < array_len; i++ ) {
                array_item = json_object_array_get_idx(object, i);
                *arrptr++ = handlebars_talloc_strdup(opcode, json_object_get_string(array_item));
            }
            *arrptr++ = NULL;
            handlebars_operand_set_arrayval(compiler, operand, arr);
            break;
        }
        case json_type_double: {
            char tmp[64];
            snprintf(tmp, 63, "%g", json_object_get_double(object));
            handlebars_operand_set_stringval(compiler, operand, tmp);
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
        struct handlebars_compiler * compiler,
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

static struct handlebars_opcode * loadTestOpcode(struct handlebars_compiler * compiler, json_object * object)
{
    struct handlebars_opcode * opcode = NULL;
    enum handlebars_opcode_type type;
    struct json_object * array_item = NULL;
    struct json_object * cur = NULL;
    int array_len = 0;
    
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
    opcode = handlebars_opcode_ctor(compiler, type);
    
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
            loadTestOpcodeOperand(compiler, opcode, &opcode->op4, array_item);
		}
        /* no break */
        case 3: {
            array_item = json_object_array_get_idx(cur, 2);
            loadTestOpcodeOperand(compiler, opcode, &opcode->op3, array_item);
        }
        /* no break */
        case 2: {
            array_item = json_object_array_get_idx(cur, 1);
            loadTestOpcodeOperand(compiler, opcode, &opcode->op2, array_item);
        }
        /* no break */
        case 1: {
            array_item = json_object_array_get_idx(cur, 0);
            loadTestOpcodeOperand(compiler, opcode, &opcode->op1, array_item);
            break;
        }
    }
    
    // Get loc
    cur = json_object_object_get(object, "loc");
    if( !cur || json_object_get_type(cur) != json_type_object ) {
    	// In v4, I guess they don't always have locations
        //fprintf(stderr, "Opcode loc was not an object!\n");
        //goto error;
    } else {
        loadTestOpcodeLoc(compiler, opcode, cur);
    }
    
error:
    return opcode;
}

/*
static int loadTestCompilerDepths(struct handlebars_compiler * compiler, json_object * object)
{
    int array_len = json_object_array_length(object);
    struct json_object * array_item = NULL;
    unsigned long depths = 0;
    for( int i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(object, i);
        int32_t v = json_object_get_int(array_item);
        depths += (1 << v - 1);
    }
    compiler->depths = depths;
}
*/

static int loadTestCompiler(struct handlebars_compiler * compiler, json_object * object)
{
    int error = 0;
    json_object * cur = NULL;
    struct json_object * array_item = NULL;
    int array_len = 0;
    
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
    compiler->opcodes_size += array_len;
    compiler->opcodes = talloc_zero_array(compiler, struct handlebars_opcode *, compiler->opcodes_size);
    
    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
        array_item = json_object_array_get_idx(cur, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: opcode was not an object, was: %s\n", json_object_get_string(array_item));
            continue;
        }
        
        compiler->opcodes[compiler->opcodes_length++] = loadTestOpcode(compiler, array_item);
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
    compiler->children_size += array_len;
    compiler->children = talloc_zero_array(compiler, struct handlebars_compiler *, compiler->children_size);
    
    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
        struct handlebars_compiler * subcompiler = handlebars_compiler_ctor(HBSCTX(compiler));
        
        array_item = json_object_array_get_idx(cur, i);
        if( json_object_get_type(array_item) != json_type_object ) {
            fprintf(stderr, "Warning: child was not an object, was: %s\n", json_object_get_string(array_item));
            continue;
        }
        
        compiler->children[compiler->children_length++] = subcompiler;
        loadTestCompiler(subcompiler, array_item);
    }

error:
    return error;
}

static char * loadTestOpcodesPrint(json_object * object)
{
    struct handlebars_context * context;
    struct handlebars_compiler * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    char * output;

    context = handlebars_context_ctor_ex(rootctx);
    parser = handlebars_parser_ctor(context);
    compiler = handlebars_compiler_ctor(HBSCTX(context));
    printer = handlebars_opcode_printer_ctor(compiler);
    
    loadTestCompiler(compiler, object);
    
    printer->flags = opcode_printer_flags;
    handlebars_opcode_printer_print(printer, compiler);
    output = talloc_steal(rootctx, printer->output);
    
error:
    handlebars_compiler_dtor(compiler);
    handlebars_context_dtor(context);
    return output;
}

static int loadSpecTestKnownHelpers(struct compiler_test * test, json_object * object)
{
    struct json_object * array_item = NULL;
    int array_len = 0;
    // Let's just allocate a nice fat array >.>
    char ** known_helpers = talloc_zero_array(rootctx, char *, 32);
    char ** ptr = known_helpers;
    const char ** ptr2 = handlebars_builtins_names();

    for( ; *ptr2 ; ++ptr2 ) {
        *ptr = handlebars_talloc_strdup(rootctx, *ptr2);
        ptr++;
    }

    json_object_object_foreach(object, key, value) {
        *ptr = handlebars_talloc_strdup(rootctx, key);
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
        test->description = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
    }
    
    // Get it
    cur = json_object_object_get(object, "it");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->it = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
    }
    
    // Get template
    cur = json_object_object_get(object, "template");
    if( cur && json_object_get_type(cur) == json_type_string ) {
        test->tmpl = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
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
        test->message = handlebars_talloc_strdup(rootctx, json_object_get_string(cur));
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
    char * filename = handlebars_talloc_asprintf(rootctx, "%s/%s.json", export_dir, name);
    int error = 0;
    char * data = NULL;
    size_t data_len = 0;
    struct json_object * result = NULL;
    struct json_object * array_item = NULL;
    int array_len = 0;
    
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
    tests = talloc_realloc(rootctx, tests, struct compiler_test, tests_size);
    
    // Iterate over array
    for( int i = 0; i < array_len; i++ ) {
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
    if( result ) {
        json_object_put(result);
    }
    return error;
}

static int loadAllSpecs()
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
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    
    // NOTE: works but handlebars.js doesn't concatenate adjacent content blocks
    if( 0 == strcmp(test->it, "escaping") && 0 == strcmp(test->description, "basic context") ) {
    	return;
    } else if( 0 == strcmp(test->it, "helper for nested raw block gets raw content") ) {
    	return;
    }

    // Initialize
    ctx = handlebars_context_ctor();
    parser = handlebars_parser_ctor(ctx);
    parser->ignore_standalone = test->opt_ignore_standalone;
    compiler = handlebars_compiler_ctor(ctx);
    printer = handlebars_opcode_printer_ctor(ctx);

    // Parse
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), test->tmpl, strlen(test->tmpl));
    handlebars_parse(parser);

    // Compile
    handlebars_compiler_set_flags(compiler, test->flags);
    if( test->known_helpers ) {
        compiler->known_helpers = (const char **) test->known_helpers;
    }

    handlebars_compiler_compile(compiler, parser->program);
    ck_assert_int_eq(0, ctx->num);
    
    // Printer
    printer->flags = opcode_printer_flags;
    handlebars_opcode_printer_print(printer, compiler);
    //fprintf(stdout, "%s\n", printer->output);
    
    // Check
    /*if( test->exception ) {
       ck_assert_int_ne(0, compiler->errnum);
    } else {*/
        //ck_assert_str_eq(printer->output, test->expected);
        if( strcmp(printer->output, test->expected) != 0 ) {
            char * tmp = handlebars_talloc_asprintf(rootctx,
                "Failed.\nSuite: %s\nTest: %s - %s\nFlags: %ld\nTemplate:\n%s\nExpected:\n%s\nActual:\n%s\n",
                test->suite_name,
                test->description, test->it, test->flags,
                test->tmpl, test->expected, printer->output);
            ck_abort_msg(tmp);
        }
    /* } */
    
    handlebars_context_dtor(ctx);
}
END_TEST

Suite * parser_suite(void)
{
    const char * title = "Handlebars Compiler Spec";
    Suite * s = suite_create(title);
    
    TCase * tc_handlebars_spec_compiler = tcase_create(title);
    // tcase_add_checked_fixture(tc_ ## name, setup, teardown);
    tcase_add_loop_test(tc_handlebars_spec_compiler, handlebars_spec_compiler, 0, tests_len - 1);
    suite_add_tcase(s, tc_handlebars_spec_compiler);
    
    return s;
}

int main(void)
{
    int number_failed;
    Suite * s;
    SRunner * sr;
    int memdebug = 0;
    int iswin = 0;
    int error = 0;

    talloc_set_log_stderr();
    
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN32__)
    iswin = 1;
#endif
    memdebug = getenv("MEMDEBUG") ? atoi(getenv("MEMDEBUG")) : 0;
    
    if( memdebug ) {
        talloc_enable_leak_report_full();
    }
    rootctx = talloc_new(NULL);
    
    // Get the export dir
    export_dir = getenv("handlebars_export_dir");
    if( export_dir == NULL ) {
        export_dir = "./spec/handlebars/export";
    }
    
    // Load the spec
    error = loadAllSpecs();
    if( error != 0 ) {
        goto error;
    }
    fprintf(stderr, "Loaded %lu test cases\n", tests_len);
    
    // Run tests
    s = parser_suite();
    sr = srunner_create(s);
    if( iswin || memdebug ) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    error = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    
error:
    talloc_free(rootctx);
    if( memdebug ) {
        talloc_report_full(NULL, stderr);
    }
    return error;
}

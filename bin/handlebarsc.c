 /**
 * Copyright (C) 2020 John Boehr
 *
 * This file is part of handlebars.c.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include <assert.h>
#include <getopt.h>

#ifdef HANDLEBARS_HAVE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_printer.h"
#include "handlebars_cache.h"
#include "handlebars_closure.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_json.h"
#include "handlebars_helpers.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_module_printer.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_partial_loader.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars_yaml.h"

#ifdef _MSC_VER
#define BOOLEAN HBS_BOOLEAN
#endif

#define __BUFF_SIZE 1024



static TALLOC_CTX * root;
static char * input_name = NULL;
static char * input_data_name = NULL;
static char * input_buf = NULL;
static size_t input_buf_length = 0;
static const char * partial_path = ".";
static const char * partial_extension = ".hbs";
static unsigned long compiler_flags = 0;
static short enable_partial_loader = 0;
static long run_count = 1;
static bool convert_input = true;
static bool newline_at_eof = true;
static size_t pool_size = 2 * 1024 * 1024;
static bool pretty_print = true;

enum handlebarsc_mode {
    handlebarsc_mode_usage = 0,
    handlebarsc_mode_version,
    handlebarsc_mode_lex,
    handlebarsc_mode_parse,
    handlebarsc_mode_compile,
    handlebarsc_mode_module,
    handlebarsc_mode_execute,
    handlebarsc_mode_debuginfo
};

enum handlebarsc_flag {
    // short flags
    handlebarsc_flag_help = 'h',
    handlebarsc_flag_version = 'V',
    handlebarsc_flag_template = 't',
    handlebarsc_flag_data = 'D',
    handlebarsc_flag_no_newline = 'n',

    // misc flags
    handlebarsc_flag_pool_size = 500,
    handlebarsc_flag_no_convert_input = 501,
    handlebarsc_flag_run_count = 502,
    handlebarsc_flag_partial_ext = 503,
    handlebarsc_flag_partial_path = 504,
    handlebarsc_flag_partial_loader = 505,
    handlebarsc_flag_flags = 506,
    handlebarsc_flag_pretty_print = 507,

    // modes
    handlebarsc_flag_lex = 600,
    handlebarsc_flag_parse = 601,
    handlebarsc_flag_compile = 602,
    handlebarsc_flag_execute = 603,
    handlebarsc_flag_debuginfo = 604,
    handlebarsc_flag_module = 605
};

static enum handlebarsc_mode mode = handlebarsc_mode_execute;

#define HBSC_OPT(a, b, c) {HBS_S1(a), b, 0, c},
#define HBSC_OPT_END {0, 0, 0, 0}

/**
 * http://linux.die.net/man/3/getopt_long
 */
static void readOpts(int argc, char * argv[])
{
    int c;
    //int digit_optind = 0;
    //int this_option_optind = optind ? optind : 1;
    int option_index = 0;

    static struct option long_options[] = {
        // modes
        HBSC_OPT(help, no_argument, handlebarsc_flag_help)
        HBSC_OPT(lex, no_argument, handlebarsc_flag_lex)
        HBSC_OPT(parse, no_argument, handlebarsc_flag_parse)
        HBSC_OPT(compile, no_argument, handlebarsc_flag_compile)
        HBSC_OPT(module, no_argument, handlebarsc_flag_module)
        HBSC_OPT(execute, no_argument, handlebarsc_flag_execute)
        HBSC_OPT(version, no_argument, handlebarsc_flag_version)
        HBSC_OPT(debuginfo, no_argument, handlebarsc_flag_debuginfo)
        // input
        HBSC_OPT(template, required_argument, handlebarsc_flag_template)
        HBSC_OPT(data, required_argument, handlebarsc_flag_data)
        // compiler flags
        HBSC_OPT(flags, required_argument, handlebarsc_flag_flags)
        // loaders
        HBSC_OPT(partial-loader, no_argument, handlebarsc_flag_partial_loader)
        HBSC_OPT(partial-path, required_argument, handlebarsc_flag_partial_path)
        HBSC_OPT(partial-ext, required_argument, handlebarsc_flag_partial_ext)
        // misc
        HBSC_OPT(run-count, required_argument, handlebarsc_flag_run_count)
        HBSC_OPT(no-convert-input, no_argument, handlebarsc_flag_no_convert_input)
        HBSC_OPT(no-newline, no_argument, handlebarsc_flag_no_newline)
        HBSC_OPT(pool-size, required_argument, handlebarsc_flag_pool_size)
        HBSC_OPT(pretty-print, no_argument, handlebarsc_flag_pretty_print)
        // end
        HBSC_OPT_END
    };

start:
    c = getopt_long(argc, argv, "hnVt:D:", long_options, &option_index);
    if( c == -1 ) {
        return;
    }

    switch( c ) {
        // modes
        case handlebarsc_flag_help:
            mode = handlebarsc_mode_usage;
            break;

        case handlebarsc_flag_lex:
            mode = handlebarsc_mode_lex;
            break;

        case handlebarsc_flag_parse:
            mode = handlebarsc_mode_parse;
            break;

        case handlebarsc_flag_compile:
            mode = handlebarsc_mode_compile;
            break;

        case handlebarsc_flag_module:
            mode = handlebarsc_mode_module;
            break;

        case handlebarsc_flag_execute:
            mode = handlebarsc_mode_execute;
            break;

        case handlebarsc_flag_version:
            mode = handlebarsc_mode_version;
            break;

        case handlebarsc_flag_debuginfo:
            mode = handlebarsc_mode_debuginfo;
            break;

        // compiler flags
        case handlebarsc_flag_flags:
            // we could do this more efficiently
            if (NULL != strstr(optarg, "compat")) {
                compiler_flags |= handlebars_compiler_flag_compat;
            }
            if (NULL != strstr(optarg, "known_helpers_only")) {
                compiler_flags |= handlebars_compiler_flag_known_helpers_only;
            }
            if (NULL != strstr(optarg, "string_params")) {
                compiler_flags |= handlebars_compiler_flag_string_params;
            }
            if (NULL != strstr(optarg, "track_ids")) {
                compiler_flags |= handlebars_compiler_flag_track_ids;
            }
            if (NULL != strstr(optarg, "no_escape")) {
                compiler_flags |= handlebars_compiler_flag_no_escape;
            }
            if (NULL != strstr(optarg, "ignore_standalone")) {
                compiler_flags |= handlebars_compiler_flag_ignore_standalone;
            }
            if (NULL != strstr(optarg, "alternate_decorators")) {
                compiler_flags |= handlebars_compiler_flag_alternate_decorators;
            }
            if (NULL != strstr(optarg, "strict")) {
                compiler_flags |= handlebars_compiler_flag_strict;
            }
            if (NULL != strstr(optarg, "assume_objects")) {
                compiler_flags |= handlebars_compiler_flag_assume_objects;
            }
            if (NULL != strstr(optarg, "mustache_style_lambdas")) {
                compiler_flags |= handlebars_compiler_flag_mustache_style_lambdas;
            }
            break;

        // partials
        case handlebarsc_flag_partial_loader:
            enable_partial_loader = 1;
            break;

        case handlebarsc_flag_partial_path:
            partial_path = optarg;
            break;

        case handlebarsc_flag_partial_ext:
            partial_extension = optarg;
            break;

        // input
        case handlebarsc_flag_template:
            input_name = optarg;
            break;
        case handlebarsc_flag_data:
            input_data_name = optarg;
            break;

        // misc
        case handlebarsc_flag_run_count:
            sscanf(optarg, "%ld", &run_count);
            break;

        case handlebarsc_flag_no_convert_input:
            convert_input = false;
            break;

        case handlebarsc_flag_no_newline:
            newline_at_eof = false;
            break;

        case handlebarsc_flag_pool_size:
            sscanf(optarg, "%zu", &pool_size);
            break;

        case handlebarsc_flag_pretty_print:
            pretty_print = true;
            break;

        default: assert(0); break; // LCOV_EXCL_LINE
    }

    goto start;
}

static char * read_stdin(void)
{
    char buffer[__BUFF_SIZE];
    FILE * in = stdin;
    char * buf = talloc_strdup(root, "");

    // Read
    while( fgets(buffer, __BUFF_SIZE, in) != NULL ) {
        buf = talloc_strdup_append_buffer(buf, buffer);
    }

    return buf;
}

static char * file_get_contents(char * filename)
{
    FILE * f;
    long size;
    char * buf;

    // No input file?
    if( !filename ) {
        fprintf(stderr, "No input file!\n");
        exit(1);
    } else if( strcmp(filename, "-") == 0 ) {
        return read_stdin();
    }

    f = fopen(filename, "rb");
    if( !f ) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buf = talloc_array(root, char, size + 1);
    size_t read = fread(buf, size, 1, f);
    fclose(f);

    buf[size] = 0;

    if (!read) {
        fprintf(stderr, "Failed to read file: %s\n", filename);
        exit(1);
    }

    return buf;
}

static void readInput(void)
{
    input_buf = file_get_contents(input_name);
    input_buf_length = talloc_array_length(input_buf) - 1;
}

static int do_usage(void)
{
    fprintf(stdout,
        "Usage: handlebarsc [OPTIONS]\n"
        "Example: handlebarsc -t foo.hbs -D bar.json\n"
        "\n"
        "Mode options:\n"
        "  -h, --help            Show this message\n"
        "  -V, --version         Print the version\n"
        "  --execute             Execute the specified template (default)\n"
        "  --lex                 Lex the specified template into tokens\n"
        "  --parse               Parse the specified template into an AST\n"
        "  --compile             Compile the specified template into opcodes\n"
        "  --module              Compile and serialize the specified template into a module\n"
        "\n"
        "Input options:\n"
        "  -t, --template=FILE   The template to operate on\n"
        "  -D, --data=FILE       The input data file. Supports JSON and YAML.\n"
        "\n"
        "Behavior options:\n"
        "  -n, --no-newline      Do not print a newline after execution\n"
        "  --flags=FLAGS         The flags to pass to the compiler separated by commas. One or more of:\n"
        "                        compat, known_helpers_only, string_params, track_ids, no_escape,\n"
        "                        ignore_standalone, alternate_decorators, strict, assume_objects,\n"
        "                        mustache_style_lambdas\n"
        "  --no-convert-input    Do not convert data to native types (use JSON wrapper)\n"
        "  --partial-loader      Specify to enable loading partials dynamically\n"
        "  --partial-path=DIR    The directory in which to look for partials\n"
        "  --partial-ext=EXT     The file extension of partials, including the '.'\n"
        "  --pool-size=SIZE      The size of the memory pool to use, 0 to disable (default 2 MB)\n"
        "  --run-count=NUM       The number of times to execute (for benchmarking)\n"
        "\n"
        "The partial loader will concat the partial-path, given partial name in the template,\n"
        "and the partial-extension to resolve the file from which to load the partial.\n"
        "\n"
        "If a FILE is specified as '-', it will be read from STDIN.\n"
        "\n"
        "handlebarsc home page: https://github.com/jbboehr/handlebars.c"
    );
    return 0;
}

static int do_version(void)
{
    fprintf(stdout,
        "handlebarsc v%s\n"
        "Copyright (C) 2020 John Boehr\n"
        "License AGPLv3.0+: Affero GNU GPL version 3.0 or later <https://www.gnu.org/licenses/agpl-3.0.html>.\n"
        "This is free software: you are free to change and redistribute it.\n"
        "There is NO WARRANTY, to the extent permitted by law.\n",
        handlebars_version_string());
    return 0;
}

static int do_debuginfo(void)
{
#ifdef HANDLEBARS_HAVE_JSON
    fprintf(stderr, "JSON support: enabled\n");
#else
    fprintf(stderr, "JSON support: disabled\n");
#endif

#ifdef HANDLEBARS_HAVE_YAML
    fprintf(stderr, "YAML support: enabled\n");
#else
    fprintf(stderr, "YAML support: disabled\n");
#endif

    fprintf(stderr, "XXHash version: %s (%u)\n", HANDLEBARS_XXHASH_VERSION, HANDLEBARS_XXHASH_VERSION_ID);

    fprintf(stderr, "\n");
#define HBS_DEBUGINFO_PRINTSIZE(typ, nam, siz) fprintf(stderr, "%-6s" " " "%-29s" " " "%zu" "\n", typ, nam, siz)
    HBS_DEBUGINFO_PRINTSIZE("void *", "", sizeof(void *));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_cache", HANDLEBARS_CACHE_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_cache_stat", sizeof(struct handlebars_cache_stat));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_closure", HANDLEBARS_CLOSURE_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_context", sizeof(struct handlebars_context));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_compiler", HANDLEBARS_COMPILER_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_map", HANDLEBARS_MAP_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_module", HANDLEBARS_MODULE_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_module_table_entry", HANDLEBARS_MODULE_TABLE_ENTRY_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_opcode", HANDLEBARS_OPCODE_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("enum", "handlebars_opcode_type", sizeof(enum handlebars_opcode_type));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_operand", HANDLEBARS_OPERAND_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("enum", "handlebars_operand_type", sizeof(enum handlebars_operand_type));
    HBS_DEBUGINFO_PRINTSIZE("union", "handlebars_operand_internals", HANDLEBARS_OPERAND_INTERNALS_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_options", HANDLEBARS_OPTIONS_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_parser", HANDLEBARS_PARSER_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_program", HANDLEBARS_PROGRAM_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_stack", handlebars_stack_size(0));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_string", HANDLEBARS_STRING_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_value", HANDLEBARS_VALUE_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("union", "handlebars_value_internals", HANDLEBARS_VALUE_INTERNALS_SIZE);
    HBS_DEBUGINFO_PRINTSIZE("enum", "handlebars_value_type", sizeof(enum handlebars_value_type));
    HBS_DEBUGINFO_PRINTSIZE("struct", "handlebars_vm", HANDLEBARS_VM_SIZE);
    return 0;
}

static int do_lex(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * tmpl;
    struct handlebars_token ** tokens;
    jmp_buf jmp;

    ctx = handlebars_context_ctor_ex(root);

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_message(ctx));
        handlebars_context_dtor(ctx);
        return 1;
    }

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    parser = handlebars_parser_ctor(ctx);

    // Lex
    tokens = handlebars_lex_ex(parser, tmpl);

    for ( ; *tokens; tokens++ ) {
        struct handlebars_string * tmp = handlebars_token_print(ctx, *tokens, 1);
        fwrite(hbs_str_val(tmp), sizeof(char), hbs_str_len(tmp), stdout);
        fflush(stdout);
        handlebars_talloc_free(tmp);
    }

    handlebars_context_dtor(ctx);
    return 0;
}

static int do_parse(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * tmpl;
    struct handlebars_string * output;
    struct handlebars_ast_node * ast;
    jmp_buf jmp;

    ctx = handlebars_context_ctor_ex(root);

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_message(ctx));
        handlebars_context_dtor(ctx);
        return 1;
    }

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    // Parse
    parser = handlebars_parser_ctor(ctx);

    ast = handlebars_parse_ex(parser, tmpl, compiler_flags);

    output = handlebars_ast_print(HBSCTX(parser), ast);
    fwrite(hbs_str_val(output), sizeof(char), hbs_str_len(output), stdout);

    handlebars_context_dtor(ctx);
    return 0;
}

static int do_compile(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_string * output;
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    jmp_buf jmp;

    ctx = handlebars_context_ctor_ex(root);

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_message(ctx));
        handlebars_context_dtor(ctx);
        return 1;
    }

    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);

    handlebars_compiler_set_flags(compiler, compiler_flags);

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    // Parse
    ast = handlebars_parse_ex(parser, tmpl, compiler_flags);

    // Compile
    program = handlebars_compiler_compile_ex(compiler, ast);

    // Print
    output = handlebars_program_print(ctx, program, 0);
    fwrite(hbs_str_val(output), sizeof(char), hbs_str_len(output), stdout);

    handlebars_context_dtor(ctx);
    return 0;
}

static int do_module(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_string * output;
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    struct handlebars_module * module;
    jmp_buf jmp;

    ctx = handlebars_context_ctor_ex(root);

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_message(ctx));
        handlebars_context_dtor(ctx);
        return 1;
    }

    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);

    handlebars_compiler_set_flags(compiler, compiler_flags);

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    // Parse
    ast = handlebars_parse_ex(parser, tmpl, compiler_flags);

    // Compile
    program = handlebars_compiler_compile_ex(compiler, ast);

    // Serialize
    module = handlebars_program_serialize(ctx, program);
    handlebars_module_generate_hash(module);

    // Print
    if (pretty_print) {
        output = handlebars_module_print(ctx, module);
        fwrite(hbs_str_val(output), sizeof(char), hbs_str_len(output), stdout);
    } else {
        handlebars_module_normalize_pointers(module, (void *) 0);
        fwrite((char *) module, sizeof(char), handlebars_module_get_size(module), stdout);
    }

    handlebars_context_dtor(ctx);
    return 0;
}

static int do_execute(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    HANDLEBARS_VALUE_DECL(partials);
    jmp_buf jmp;

    ctx = handlebars_context_ctor_ex(root);

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", handlebars_error_message(ctx));
        handlebars_context_dtor(ctx);
        return 1;
    }

    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);

    if (enable_partial_loader) {
        struct handlebars_string *partial_path_str = NULL;
        struct handlebars_string *partial_extension_str = NULL;
        partial_path_str = handlebars_string_ctor(ctx, partial_path, strlen(partial_path));
        partial_extension_str = handlebars_string_ctor(ctx, partial_extension, strlen(partial_extension));
        (void) handlebars_value_partial_loader_init(ctx, partial_path_str, partial_extension_str, partials);
    }

    handlebars_compiler_set_flags(compiler, compiler_flags);

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(parser), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    // Read context
    HANDLEBARS_VALUE_DECL(input);
    if( input_data_name ) {
        size_t input_data_name_len = strlen(input_data_name);
        char * input_str = file_get_contents(input_data_name);
        size_t input_str_size = talloc_array_length(input_str);
        if (input_str && input_str_size > 1) {
            if (handlebars_value_is_empty(input) && input_data_name_len > 5 && (0 == strcmp(input_data_name + input_data_name_len - 5, ".yaml") ||
                    0 == strcmp(input_data_name + input_data_name_len - 4, ".yml"))) {
#ifdef HANDLEBARS_HAVE_YAML
                handlebars_value_init_yaml_string(ctx, input, input_str);
#else
                fprintf(stderr, "Failed to process input data: YAML support is disabled");
                exit(1);
#endif
            }
            if (handlebars_value_is_empty(input)) {
#ifdef HANDLEBARS_HAVE_JSON
                // assume json
                handlebars_value_init_json_stringl(ctx, input, input_str, input_str_size - 1);
                if (convert_input) {
                    handlebars_value_convert(input);
                }
#else
                fprintf(stderr, "Failed to process input data: JSON support is disabled");
                exit(1);
#endif
            }
        }
    }

    // Parse
    ast = handlebars_parse_ex(parser, tmpl, compiler_flags);

    // Compile
    program = handlebars_compiler_compile_ex(compiler, ast);

    // Serialize
    struct handlebars_module * module = handlebars_program_serialize(ctx, program);

    // Execute
    struct handlebars_string * buffer = NULL;
    do {
        if (buffer) {
            handlebars_talloc_free(buffer);
            buffer = NULL;
        }

        struct handlebars_vm * vm;
        vm = handlebars_vm_ctor(ctx);
        handlebars_vm_set_flags(vm, compiler_flags);
        handlebars_vm_set_partials(vm, partials);

        buffer = handlebars_vm_execute(vm, module, input);
        buffer = talloc_steal(ctx, buffer);

        handlebars_vm_dtor(vm);
    } while(--run_count > 0);

    if (buffer) {
        fwrite(hbs_str_val(buffer), sizeof(char), hbs_str_len(buffer), stdout);
    }

    if (newline_at_eof) {
        fwrite("\n", sizeof(char), 1, stdout);
    }

    HANDLEBARS_VALUE_UNDECL(input);
    HANDLEBARS_VALUE_UNDECL(partials);
    handlebars_context_dtor(ctx);
    return 0;
}

int main(int argc, char * argv[])
{
#ifdef HANDLEBARS_HAVE_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        pool_size = 0;
    }
#endif

    root = talloc_new(NULL);

    if( argc <= 1 ) {
        do_usage();
        return 1;
    }

    readOpts(argc, argv);

    if (pool_size > 0) {
        void * old_root = root;
        root = talloc_pool(NULL, pool_size);
        talloc_steal(root, old_root);
    }

    if( optind < argc ) {
        while( optind < argc ) {
            input_name = argv[optind++];
            break;
        }
    }

    switch( mode ) {
        case handlebarsc_mode_version: return do_version();
        case handlebarsc_mode_lex: return do_lex();
        case handlebarsc_mode_parse: return do_parse();
        case handlebarsc_mode_compile: return do_compile();
        case handlebarsc_mode_module: return do_module();
        case handlebarsc_mode_execute: return do_execute();
        case handlebarsc_mode_debuginfo: return do_debuginfo();
        case handlebarsc_mode_usage: return do_usage();

        // LCOV_EXCL_START
        default:
            assert(0);
            do_usage();
            return 1;
        // LCOV_EXCL_STOP
    }
}

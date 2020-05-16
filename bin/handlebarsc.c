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

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_printer.h"
#include "handlebars_cache.h"
#include "handlebars_compiler.h"
#include "handlebars_delimiters.h"
#include "handlebars_json.h"
#include "handlebars_memory.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_parser.h"
#include "handlebars_partial_loader.h"
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

enum handlebarsc_mode {
    handlebarsc_mode_usage = 0,
    handlebarsc_mode_version,
    handlebarsc_mode_lex,
    handlebarsc_mode_parse,
    handlebarsc_mode_compile,
    handlebarsc_mode_execute,
    handlebarsc_mode_debug
};

static enum handlebarsc_mode mode = handlebarsc_mode_execute;

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
        {"help",      no_argument,              0,  'h' },
        {"lex",       no_argument,              0,  'l' },
        {"parse",     no_argument,              0,  'p' },
        {"compile",   no_argument,              0,  'c' },
        {"execute",   no_argument,              0,  'e' },
        {"version",   no_argument,              0,  'V' },
        {"debug",     no_argument,              0,  'd' },
        // input
        {"template",  required_argument,        0,  't' },
        {"data",      required_argument,        0,  'D' },
        // compiler flags
        {"flags",     required_argument,        0,  'F' },
        // loaders
        {"partial-loader", no_argument,         0,  'P' },
        {"partial-path",   required_argument,   0,  'A' },
        {"partial-ext",    required_argument,   0,  'X' },
        // misc
        {"run-count",    required_argument,     0,  'N' },
        {"no-convert-input", no_argument,       0,  'C' },
        {"no-newline", no_argument,             0,  'n' },
        // end
        {0,           0,                        0,  0   }
    };

start:
    c = getopt_long(argc, argv, "hlpcVt:D:", long_options, &option_index);
    if( c == -1 ) {
        return;
    }

    switch( c ) {
        // modes
        case 'h':
            mode = handlebarsc_mode_usage;
            break;
        case 'l':
            mode = handlebarsc_mode_lex;
            break;
        case 'p':
            mode = handlebarsc_mode_parse;
            break;
        case 'c':
            mode = handlebarsc_mode_compile;
            break;
        case 'e':
            mode = handlebarsc_mode_execute;
            break;
        case 'V':
            mode = handlebarsc_mode_version;
            break;
        case 'd':
            mode = handlebarsc_mode_debug;
            break;

        // compiler flags
        case 'F':
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

        case 'P':
            enable_partial_loader = 1;
            break;
        case 'A':
            partial_path = optarg;
            break;
        case 'X':
            partial_extension = optarg;
            break;

        // input
        case 't':
            input_name = optarg;
            break;
        case 'D':
            input_data_name = optarg;
            break;

        // misc
        case 'N':
            sscanf(optarg, "%ld", &run_count);
            break;
        case 'C':
            convert_input = false;
            break;
        case 'n':
            newline_at_eof = false;
            break;

        default: break;
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
        "  -e, --execute         Execute the specified template (default)\n"
        "  -l, --lex             Lex the specified template into tokens\n"
        "  -p, --parse           Parse the specified template into an AST\n"
        "  -c, --compile         Compile the specified template into opcodes\n"
        "  -V, --version         Print the version\n"
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

static int do_debug(void)
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

    fprintf(stderr, "sizeof(void *): %lu\n", (long unsigned) sizeof(void *));
    fprintf(stderr, "sizeof(struct handlebars_cache): %lu\n", (long unsigned) HANDLEBARS_CACHE_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_cache_stat): %lu\n", (long unsigned) sizeof(struct handlebars_cache_stat));
    fprintf(stderr, "sizeof(struct handlebars_context): %lu\n", (long unsigned) sizeof(struct handlebars_context));
    fprintf(stderr, "sizeof(struct handlebars_compiler): %lu\n", (long unsigned) HANDLEBARS_COMPILER_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_map): %lu\n", (long unsigned) handlebars_map_size());
    fprintf(stderr, "sizeof(struct handlebars_opcode): %lu\n", (long unsigned) HANDLEBARS_OPCODE_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_operand): %lu\n", (long unsigned) HANDLEBARS_OPERAND_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_options): %lu\n", (long unsigned) HANDLEBARS_OPTIONS_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_parser): %lu\n", (long unsigned) HANDLEBARS_PARSER_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_program): %lu\n", (long unsigned) HANDLEBARS_PROGRAM_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_stack): %lu\n", (long unsigned) handlebars_stack_size(0));
    fprintf(stderr, "sizeof(struct handlebars_string): %lu\n", (long unsigned) HANDLEBARS_STRING_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_value): %lu\n", (long unsigned) HANDLEBARS_VALUE_SIZE);
    fprintf(stderr, "sizeof(union handlebars_value_internals): %lu\n", (long unsigned) HANDLEBARS_VALUE_INTERNALS_SIZE);
    fprintf(stderr, "sizeof(struct handlebars_vm): %lu\n", (long unsigned) HANDLEBARS_VM_SIZE);
    return 0;
}

static int do_lex(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * tmpl;
    struct handlebars_token ** tokens;
    jmp_buf jmp;

    ctx = handlebars_context_ctor();

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

    ctx = handlebars_context_ctor();

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

    ctx = handlebars_context_ctor();

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

static int do_execute(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_string * tmpl;
    struct handlebars_ast_node * ast;
    struct handlebars_program * program;
    struct handlebars_value * partials;
    jmp_buf jmp;

    ctx = handlebars_context_ctor();

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
        partials = handlebars_value_partial_loader_ctor(ctx, partial_path_str, partial_extension_str);
    } else {
        partials = handlebars_value_ctor(ctx);
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
    struct handlebars_value * context = NULL;
    if( input_data_name ) {
        size_t input_data_name_len = strlen(input_data_name);
        char * context_str = file_get_contents(input_data_name);
        if (context_str && strlen(context_str)) {
            if (!context && input_data_name_len > 5 && (0 == strcmp(input_data_name + input_data_name_len - 5, ".yaml") ||
                    0 == strcmp(input_data_name + input_data_name_len - 4, ".yml"))) {
#ifdef HANDLEBARS_HAVE_YAML
                context = handlebars_value_from_yaml_string(ctx, context_str);
#else
                fprintf(stderr, "Failed to process input data: YAML support is disabled");
                exit(1);
#endif
            }
            if (!context) {
#ifdef HANDLEBARS_HAVE_JSON
                // assume json
                context = handlebars_value_from_json_string(ctx, context_str);
                if (convert_input) {
                    handlebars_value_convert(context);
                }
#else
                fprintf(stderr, "Failed to process input data: JSON support is disabled");
                exit(1);
#endif
            }
        }
    }
    if( !context ) {
        context = handlebars_value_ctor(ctx);
    }
    handlebars_value_addref(context); // need this for multiple runs

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
        handlebars_vm_set_helpers(vm, handlebars_value_ctor(ctx));
        handlebars_vm_set_partials(vm, partials);

        buffer = handlebars_vm_execute(vm, module, context);
        buffer = talloc_steal(ctx, buffer);

        handlebars_vm_dtor(vm);
    } while(--run_count > 0);

    if (buffer) {
        fwrite(hbs_str_val(buffer), sizeof(char), hbs_str_len(buffer), stdout);
    }

    if (newline_at_eof) {
        fwrite("\n", sizeof(char), 1, stdout);
    }

    handlebars_context_dtor(ctx);
    return 0;
}

int main(int argc, char * argv[])
{
    root = talloc_new(NULL);

    if( argc <= 1 ) {
        return do_usage();
    }

    readOpts(argc, argv);

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
        case handlebarsc_mode_execute: return do_execute();
        case handlebarsc_mode_debug: return do_debug();
        default: return do_usage();
    }
}

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
#include "handlebars_memory.h"

#include "handlebars_ast.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_opcodes.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_opcode_serializer.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#define __BUFF_SIZE 1024

TALLOC_CTX * root;
char * input_name = NULL;
char * input_data_name = NULL;
char * input_buf = NULL;
size_t input_buf_length = 0;
unsigned long compiler_flags = 0;

enum handlebarsc_mode {
    handlebarsc_mode_usage = 0,
    handlebarsc_mode_version,
    handlebarsc_mode_lex,
    handlebarsc_mode_parse,
    handlebarsc_mode_compile,
    handlebarsc_mode_execute,
    handlebarsc_mode_debug
};

static enum handlebarsc_mode mode;

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
        {"compat",    no_argument,              0,  'C' },
        {"known-helpers-only", no_argument,     0,  'K' },
        {"string-params", no_argument,          0,  'S' },
        {"track-ids", no_argument,              0,  'T' },
        {"no-escape", no_argument,              0,  'U' },
        {"ignore-standalone", no_argument,      0,  'G' },
        {"alternate-decorators", no_argument,   0,  'A' },
        {"strict",    no_argument,              0,  's' },
        {"assume-objects", no_argument,         0,  'a' },
        {0,           0,                        0,  0   }
    };

start:
    c = getopt_long(argc, argv, "hlpcVt:", long_options, &option_index);
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
        case 'C':
            compiler_flags |= handlebars_compiler_flag_compat;
            break;
        case 'K':
            compiler_flags |= handlebars_compiler_flag_known_helpers_only;
            break;
        case 'S':
            compiler_flags |= handlebars_compiler_flag_string_params;
            break;
        case 'T':
            compiler_flags |= handlebars_compiler_flag_track_ids;
            break;
        case 'U':
            compiler_flags |= handlebars_compiler_flag_no_escape;
            break;
        case 'G':
            compiler_flags |= handlebars_compiler_flag_ignore_standalone;
            break;
        case 'A':
            compiler_flags |= handlebars_compiler_flag_alternate_decorators;
            break;
        case 's':
            compiler_flags |= handlebars_compiler_flag_strict;
            break;
        case 'a':
            compiler_flags |= handlebars_compiler_flag_assume_objects;
            break;

        // input
        case 't':
            input_name = optarg;
            break;
        case 'D':
            input_data_name = optarg;
    }

    goto start;
}

static char * read_stdin()
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
    fread(buf, size, 1, f);
    fclose(f);

    buf[size] = 0;

    return buf;
}

static void readInput(void)
{
    input_buf = file_get_contents(input_name);
    input_buf_length = talloc_array_length(input_buf) - 1;
}

static int do_usage(void)
{
    fprintf(stderr, "Usage: handlebarsc [--lex|--parse|--compile|--execute] [--data JSON_FILE] [TEMPLATE]\n");
    return 0;
}

static int do_version(void)
{
    fprintf(stderr, "handlebarsc v%s\n", handlebars_version_string());
    return 0;
}

static int do_debug(void)
{
    fprintf(stderr, "sizeof(void *): %lu\n", (long unsigned) sizeof(void *));
    fprintf(stderr, "sizeof(struct handlebars_context): %lu\n", (long unsigned) sizeof(struct handlebars_context));
    fprintf(stderr, "sizeof(struct handlebars_compiler): %lu\n", (long unsigned) sizeof(struct handlebars_compiler));
    fprintf(stderr, "sizeof(struct handlebars_opcode): %lu\n", (long unsigned) sizeof(struct handlebars_opcode));
    fprintf(stderr, "sizeof(struct handlebars_operand): %lu\n", (long unsigned) sizeof(struct handlebars_operand));
    fprintf(stderr, "sizeof(struct handlebars_parser): %lu\n", (long unsigned) sizeof(struct handlebars_parser));
    fprintf(stderr, "sizeof(struct handlebars_program): %lu\n", (long unsigned) sizeof(struct handlebars_program));
    fprintf(stderr, "sizeof(struct handlebars_value): %lu\n", (long unsigned) sizeof(struct handlebars_value));
    fprintf(stderr, "sizeof(union handlebars_value_internals): %lu\n", (long unsigned) sizeof(union handlebars_value_internals));
    fprintf(stderr, "sizeof(struct handlebars_vm): %lu\n", (long unsigned) sizeof(struct handlebars_vm));
    return 0;
}

static int do_lex(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_token * token = NULL;
    struct handlebars_string * tmpl;
    int token_int = 0;
    struct handlebars_string * output;
    jmp_buf jmp;


    ctx = handlebars_context_ctor();

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
        goto error;
    }

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    parser = handlebars_parser_ctor(ctx);
    parser->tmpl = tmpl;

    // Run
    do {
        YYSTYPE yylval_param;
        YYLTYPE yylloc_param;
        YYSTYPE * lval;

        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, parser->scanner);
        if( token_int == END || token_int == INVALID ) {
            break;
        }
        lval = handlebars_yy_get_lval(parser->scanner);

        // Make token object
        token = handlebars_token_ctor(ctx, token_int, lval->string);

        // Print token
        output = handlebars_token_print(ctx, token, 0);
        fprintf(stdout, "%s\n", output->val);
        fflush(stdout);
        handlebars_talloc_free(output);
    } while( token && token_int != END && token_int != INVALID );

error:
handlebars_context_dtor(ctx);
    return 0;
}

static int do_parse(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_string * tmpl;
    int error = 0;
    jmp_buf jmp;

    ctx = handlebars_context_ctor();

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
        goto error;
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
    parser->tmpl = tmpl;

    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }

    handlebars_parse(parser);

    struct handlebars_string * output = handlebars_ast_print(HBSCTX(parser), parser->program);
    fprintf(stdout, "%s\n", output->val);

error:
    handlebars_context_dtor(ctx);
    return error;
}

static int do_compile(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_string * output;
    struct handlebars_string * tmpl;
    int error = 0;
    jmp_buf jmp;

    ctx = handlebars_context_ctor();

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
        goto error;
    }

    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);

    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }

    handlebars_compiler_set_flags(compiler, compiler_flags);

    // Read
    readInput();
    tmpl = handlebars_string_ctor(HBSCTX(ctx), input_buf, strlen(input_buf));

    // Preprocess
    if( compiler_flags & handlebars_compiler_flag_compat ) {
        tmpl = handlebars_preprocess_delimiters(ctx, tmpl, NULL, NULL);
    }

    // Parse
    parser->tmpl = tmpl;
    handlebars_parse(parser);

    // Compile
    handlebars_compiler_compile(compiler, parser->program);

    // Print
    output = handlebars_program_print(ctx, compiler->program, 0);
    fprintf(stdout, "%.*s\n", (int) output->len, output->val);

error:
    handlebars_context_dtor(ctx);
    return error;
}

static int do_execute(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_vm * vm;
    struct handlebars_string * tmpl;
    int error = 0;
    jmp_buf jmp;

    ctx = handlebars_context_ctor();

    // Save jump buffer
    if( handlebars_setjmp_ex(ctx, &jmp) ) {
        fprintf(stderr, "ERROR: %s\n", ctx->e->msg);
        goto error;
    }

    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);
    vm = handlebars_vm_ctor(ctx);
    vm->helpers = handlebars_value_ctor(ctx);
    vm->partials = handlebars_value_ctor(ctx);

    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
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
        char * context_str = file_get_contents(input_data_name);
        if( context_str && strlen(context_str) ) {
            context = handlebars_value_from_json_string(ctx, context_str);
        }
    }
    if( !context ) {
        context = handlebars_value_ctor(ctx);
    }

    // Parse
    parser->tmpl = tmpl;
    handlebars_parse(parser);

    // Compile
    handlebars_compiler_compile(compiler, parser->program);

    // Serialize
    struct handlebars_module * module = handlebars_program_serialize(ctx, compiler->program);

    // Execute
    handlebars_vm_execute(vm, module, context);

    fprintf(stdout, "%.*s", (int) vm->buffer->len, vm->buffer->val);

error:
    handlebars_context_dtor(ctx);
    return error;
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

#include <assert.h>
#include <getopt.h>

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_printer.h"
#include "handlebars_compiler.h"
#include "handlebars_memory.h"
#include "handlebars_opcode_printer.h"
#include "handlebars_string.h"
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
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
int compiler_flags = 0;

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
    fprintf(stderr, "sizeof(struct handlebars_context): %ld\n", sizeof(struct handlebars_context));
    fprintf(stderr, "sizeof(struct handlebars_compiler): %ld\n", sizeof(struct handlebars_compiler));
    fprintf(stderr, "sizeof(struct handlebars_parser): %ld\n", sizeof(struct handlebars_parser));
    fprintf(stderr, "sizeof(struct handlebars_value): %ld\n", sizeof(struct handlebars_value));
    fprintf(stderr, "sizeof(struct handlebars_vm): %ld\n", sizeof(struct handlebars_vm));
    return 0;
}

static int do_lex(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_token * token = NULL;
    int token_int = 0;
    
    readInput();
    ctx = handlebars_context_ctor();
    parser = handlebars_parser_ctor(ctx);
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), input_buf, strlen(input_buf));
    
    // Run
    do {
        YYSTYPE yylval_param;
        YYLTYPE yylloc_param;
        YYSTYPE * lval;
        char * text;
        char * output;
        
        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, parser->scanner);
        if( token_int == END || token_int == INVALID ) {
            break;
        }
        lval = handlebars_yy_get_lval(parser->scanner);
        
        // Make token object
        text = (lval->text == NULL ? "" : lval->text);
        token = handlebars_token_ctor(ctx, token_int, text, strlen(text));
        
        // Print token
        output = handlebars_token_print(token, 0);
        fprintf(stdout, "%s\n", output);
        fflush(stdout);
    } while( token && token_int != END && token_int != INVALID );
    
    return 0;
}

static int do_parse(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    char * output;
    int error = 0;
    
    readInput();
    ctx = handlebars_context_ctor();
    parser = handlebars_parser_ctor(ctx);
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), input_buf, strlen(input_buf));
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }

    handlebars_parse(parser);
    
    if( ctx->num != NULL ) {
        output = handlebars_error_message(ctx);
        fprintf(stderr, "%s\n", output);
        error = ctx->num;
        goto error;
    }

    output =  handlebars_ast_print(parser, parser->program, 0);
    fprintf(stdout, "%s\n", output);

error:
    handlebars_context_dtor(ctx);
    return error;
}

static int do_compile(void)
{
    struct handlebars_context * ctx;
    struct handlebars_parser * parser;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    int error = 0;
    
    ctx = handlebars_context_ctor();
    parser = handlebars_parser_ctor(ctx);
    compiler = handlebars_compiler_ctor(ctx);
    printer = handlebars_opcode_printer_ctor(ctx);
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }
    
    handlebars_compiler_set_flags(compiler, compiler_flags);
    
    // Read
    readInput();
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), input_buf, strlen(input_buf));
    
    // Parse
    handlebars_parse(parser);

    if( parser->ctx.num ) {
        fprintf(stderr, "ERROR: %s\n", parser->ctx.msg);
        goto error;
    }
    
    // Compile
    handlebars_compiler_compile(compiler, parser->program);

    if( compiler->ctx.num ) {
        fprintf(stderr, "ERROR: %s\n", compiler->ctx.msg);
        goto error;
    }
    
    // Printer
    //printer->flags = handlebars_opcode_printer_flag_locations;
    handlebars_opcode_printer_print(printer, compiler);
    fprintf(stdout, "%s\n", printer->output);

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
    int error = 0;

    ctx = handlebars_context_ctor();
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
    parser->tmpl = handlebars_string_ctor(HBSCTX(parser), input_buf, strlen(input_buf));

    // Read context
    char * context_str = file_get_contents(input_data_name);
    struct handlebars_value * context;
    if( context_str && strlen(context_str) ) {
        context = handlebars_value_from_json_string(ctx, context_str);
    } else {
        context = handlebars_value_ctor(ctx);
    }

    // Parse
    handlebars_parse(parser);

    if( parser->ctx.num ) {
        fprintf(stderr, "ERROR: %s\n", parser->ctx.msg);
        goto error;
    }

    // Compile
    handlebars_compiler_compile(compiler, parser->program);

    if( compiler->ctx.num ) {
        fprintf(stderr, "ERROR: %s\n", compiler->ctx.msg);
        goto error;
    }

    handlebars_vm_execute(vm, compiler, context);

    fprintf(stdout, vm->buffer);

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

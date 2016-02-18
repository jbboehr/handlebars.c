
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
#include "handlebars_token.h"
#include "handlebars_token_list.h"
#include "handlebars_token_printer.h"
#include "handlebars_utils.h"
#include "handlebars.tab.h"
#include "handlebars.lex.h"

#define __BUFF_SIZE 1024

char * input_name = NULL;
char * input_buf = NULL;
size_t input_buf_length = 0;
int compiler_flags = 0;

enum handlebarsc_mode {
    handlebarsc_mode_usage = 0,
    handlebarsc_mode_version,
    handlebarsc_mode_lex,
    handlebarsc_mode_parse,
    handlebarsc_mode_compile,
    handlebarsc_mode_execute
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
        // input
        {"template",  required_argument,        0,  't' },
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
    }
    
    goto start;
}

static void readInput(void)
{
    char buffer[__BUFF_SIZE];
    char * tmp;
    FILE * in;
    int close = 1;
    
    // No input file?
    if( !input_name ) {
        fprintf(stderr, "No input file!\n");
        exit(1);
    }
    
    // Read from stdin
    if( strcmp(input_name, "-") == 0 ) {
        in = stdin;
        close = 0;
    } else {
        in = fopen(input_name, "r");
    }
    
    // Initialize buffer
    input_buf = talloc_strdup(NULL, "");
    if( input_buf == NULL ) {
        exit(1);
    }
    
    // Read
    while( fgets(buffer, __BUFF_SIZE, in) != NULL ) {
        input_buf = talloc_strdup_append_buffer(input_buf, buffer);
    }
    input_buf_length = strlen(input_buf);
    
    // Trim the newline off the end >.>
    tmp = input_buf + input_buf_length - 1;
    while( input_buf_length > 0 && (*tmp == '\n' || *tmp == '\r') ) {
        *tmp = 0;
        input_buf_length--;
    }
    
    if( close ) {
        fclose(in);
    }
    
    if( input_buf_length <= 0 ) {
        exit(1);
    }
}

static int do_usage(void)
{
    fprintf(stderr, "Usage: handlebarsc [--lex|--parse|--compile] [TEMPLATE]\n");
    return 0;
}

static int do_version(void)
{
    fprintf(stderr, "handlebarsc v%s\n", handlebars_version_string());
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
    parser->tmpl = input_buf;
    
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
    parser->tmpl = input_buf;
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }

    handlebars_parse(parser);
    
    if( ctx->num != NULL ) {
        output = handlebars_context_get_errmsg(ctx);
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
    compiler = handlebars_compiler_ctor(ctx, parser);
    printer = handlebars_opcode_printer_ctor(ctx);
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        parser->ignore_standalone = 1;
    }
    
    handlebars_compiler_set_flags(compiler, compiler_flags);
    
    // Read
    readInput();
    parser->tmpl = input_buf;
    
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
    return 1;
}

int main(int argc, char * argv[])
{
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
        default: return do_usage();
    }
}

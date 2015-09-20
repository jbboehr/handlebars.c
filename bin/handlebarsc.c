
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
#include "handlebars_context.h"
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
    handlebarsc_mode_compile
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
        {"help",      no_argument,          0,  'h' },
        {"lex",       no_argument,          0,  'l' },
        {"parse",     no_argument,          0,  'p' },
        {"compile",   no_argument,          0,  'c' },
        {"version",   no_argument,          0,  'V' },
        // input
        {"template",  required_argument,    0,  't' },
        // compiler flags
        {"compat",    no_argument,          0,  'C' },
        {"known-helpers-only", no_argument, 0,  'K' },
        {"string-params", no_argument,      0,  'S' },
        {"track-ids", no_argument,          0,  'T' },
        {"no-escape", no_argument,          0,  'U' },
        {"ignore-standalone", no_argument,  0,  'G' },
        {0,           0,                    0,  0   }
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
    struct handlebars_token * token = NULL;
    int token_int = 0;
    
    readInput();
    ctx = handlebars_context_ctor();
    ctx->tmpl = input_buf;
    
    // Run
    do {
        YYSTYPE yylval_param;
        YYLTYPE yylloc_param;
        YYSTYPE * lval;
        char * text;
        char * output;
        
        token_int = handlebars_yy_lex(&yylval_param, &yylloc_param, ctx->scanner);
        if( token_int == END || token_int == INVALID ) {
            break;
        }
        lval = handlebars_yy_get_lval(ctx->scanner);
        
        // Make token object
        text = (lval->text == NULL ? "" : lval->text);
        token = handlebars_token_ctor(token_int, text, strlen(text), ctx);
        
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
    char * output;
    //int retval;
    int error = 0;
    
    readInput();
    ctx = handlebars_context_ctor();
    ctx->tmpl = input_buf;
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        ctx->ignore_standalone = 1;
    }
    
    /*retval =*/ handlebars_yy_parse(ctx);
    
    if( ctx->error != NULL ) {
        output = handlebars_context_get_errmsg(ctx);
        fprintf(stderr, "%s\n", output);
        error = ctx->errnum;
        goto error;
    }

    struct handlebars_ast_printer_context printctx = handlebars_ast_print2(ctx->program, 0);
    fprintf(stdout, "%s\n", printctx.output);

error:
    handlebars_context_dtor(ctx);
    return error;
}

static int do_compile(void)
{
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    //int retval;
    int error = 0;
    
    ctx = handlebars_context_ctor();
    compiler = handlebars_compiler_ctor(ctx);
    printer = handlebars_opcode_printer_ctor(ctx);
    
    if( compiler_flags & handlebars_compiler_flag_ignore_standalone ) {
        ctx->ignore_standalone = 1;
    }
    
    handlebars_compiler_set_flags(compiler, compiler_flags);
    
    // Read
    readInput();
    ctx->tmpl = input_buf;
    
    // Parse
    /*retval =*/ handlebars_yy_parse(ctx);

    if( ctx->error ) {
        fprintf(stderr, "%s\n", ctx->error);
        error = ctx->errnum;
        goto error;
    } else if( ctx->errnum ) {
        fprintf(stderr, "Parser error %d\n", compiler->errnum);
        error = ctx->errnum;
        goto error;
    }
    
    // Compile
    handlebars_compiler_compile(compiler, ctx->program);

    if( compiler->error ) {
        fprintf(stderr, "%s\n", compiler->error);
        error = compiler->errnum;
        goto error;
    } else if( compiler->errnum ) {
        fprintf(stderr, "Compiler error %d\n", compiler->errnum);
        error = compiler->errnum;
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
        default: return do_usage();
    }
}

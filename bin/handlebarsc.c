
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>

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
char * stdin_buf;
size_t stdin_buf_length;

static void readStdin(void)
{
    char buffer[__BUFF_SIZE];
    char * tmp;
    
    stdin_buf = talloc_strdup(NULL, "");
    if( stdin_buf == NULL ) {
        exit(1);
    }
    
    while (fgets(buffer, __BUFF_SIZE, stdin) != NULL) {
        stdin_buf = talloc_strdup_append_buffer(stdin_buf, buffer);
    }
    stdin_buf_length = strlen(stdin_buf);
    
    // Trim the newline off the end >.>
    tmp = stdin_buf + stdin_buf_length - 1;
    while( stdin_buf_length > 0 && (*tmp == '\n' || *tmp == '\r') ) {
        *tmp = 0;
        stdin_buf_length--;
    }
    
    if( stdin_buf_length <= 0 ) {
        exit(1);
    }
}

static int do_usage(void)
{
    fprintf(stderr, "Usage: handlebarsc [lex|parse|compile] [TEMPLATE]\n");
    return 0;
}

static int do_lex(void)
{
    struct handlebars_context * ctx;
    struct handlebars_token * token = NULL;
    int token_int = 0;
    
    readStdin();
    ctx = handlebars_context_ctor();
    ctx->tmpl = stdin_buf;
    
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
    
    readStdin();
    ctx = handlebars_context_ctor();
    ctx->tmpl = stdin_buf;
    
    /*retval =*/ handlebars_yy_parse(ctx);
    
    if( ctx->error != NULL ) {
        output = handlebars_context_get_errmsg(ctx);
        fprintf(stdout, "%s\n", output);
        handlebars_talloc_free(output);
        
        error = 1;
    } else {
        //char * output = handlebars_ast_print(ctx->program, 0);
        struct handlebars_ast_printer_context printctx = handlebars_ast_print2(ctx->program, 0);
        //_handlebars_ast_print(ctx->program, &printctx);
        char * output = printctx.output;
        fprintf(stdout, "%s\n", output);
        handlebars_talloc_free(output);
        
        errno = 0;
    }
    
    return error;
}

static int do_compile(void)
{
    struct handlebars_context * ctx;
    struct handlebars_compiler * compiler;
    struct handlebars_opcode_printer * printer;
    char * output;
    //int retval;
    int error = 0;
    
    ctx = handlebars_context_ctor();
    compiler = handlebars_compiler_ctor(ctx);
    printer = handlebars_opcode_printer_ctor(ctx);
    
    // Read
    readStdin();
    ctx->tmpl = stdin_buf;
    
    // Parse
    /*retval =*/ handlebars_yy_parse(ctx);
    
    // Compile
    handlebars_compiler_compile(compiler, ctx->program);
    if( compiler->errnum ) {
        goto error;
    }
    
    // Printer
    handlebars_opcode_printer_print(printer, compiler);
    fprintf(stdout, "%s\n", printer->output);
    
error:
    handlebars_context_dtor(ctx);
    return error;
}

int main( int argc, char * argv[])
{
    if( argc <= 1 ) {
        return do_usage();
    }
    
    if( strcmp(argv[1], "lex") == 0 ) {
        return do_lex();
    } else if( strcmp(argv[1], "parse") == 0 ) {
        return do_parse();
    } else if( strcmp(argv[1], "compile") == 0 ) {
        return do_compile();
    } else {
        return do_usage();
    }
}

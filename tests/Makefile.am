
check_PROGRAMS = test_main \
                 test_ast \
                 test_ast_list \
                 test_context \
                 test_token \
                 test_token_list \
                 test_token_printer \
                 test_utils \
                 test_spec_handlebars_parser \
                 test_spec_handlebars_tokenizer

test_main_SOURCES = utils.c test_main.c
test_ast_SOURCES = utils.c test_ast.c
test_ast_list_SOURCES = utils.c test_ast_list.c
test_context_SOURCES = utils.c test_context.c
test_token_SOURCES = utils.c test_token.c
test_token_list_SOURCES = utils.c test_token_list.c
test_token_printer_SOURCES = utils.c test_token_printer.c
test_utils_SOURCES = utils.c test_utils.c
test_spec_handlebars_parser_SOURCES = utils.c test_spec_handlebars_parser.c
test_spec_handlebars_tokenizer_SOURCES = utils.c test_spec_handlebars_tokenizer.c

TESTS = test_main \
        test_ast \
        test_ast_list \
        test_context \
        test_token \
        test_token_list \
        test_token_printer \
        test_utils \
        test_spec_handlebars_parser \
        test_spec_handlebars_tokenizer

TESTS_ENVIRONMENT = \
	top_srcdir=$(top_srcdir) \
	top_builddir=$(top_builddir) \
	handlebars_spec_dir=$(top_srcdir)/spec/handlebars/spec \
	handlebars_tokenizer_spec=$(top_srcdir)/spec/handlebars/spec/tokenizer.json \
	handlebars_parser_spec=$(top_srcdir)/spec/handlebars/spec/parser.json
	
AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src
AM_LDFLAGS = -static  # for speed and Valgrind
AM_CFLAGS = @CHECK_CFLAGS@
LDADD = $(top_builddir)/src/libhandlebars.la @LIBCHECK_LIBS@


check_PROGRAMS = test_spec_handlebars_tokenizer

test_spec_handlebars_tokenizer_SOURCES = utils.c test_spec_handlebars_tokenizer.c

TESTS = test_spec_handlebars_tokenizer
TESTS_ENVIRONMENT = \
	top_srcdir=$(top_srcdir) \
	top_builddir=$(top_builddir) \
	handlebars_spec_dir=$(top_srcdir)/spec/handlebars/spec \
	handlebars_tokenizer_spec=$(top_srcdir)/spec/handlebars/spec/tokenizer.json
	
AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src
AM_LDFLAGS = -static  # for speed and Valgrind
AM_CFLAGS = @CHECK_CFLAGS@
LDADD = $(top_builddir)/src/libhandlebars.la @LIBCHECK_LIBS@

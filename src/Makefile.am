
EXTRA_DIST = handlebars.l handlebars.y

if YYDEBUG
	BISONFLAGS = -d -Wall # --debug --warnings=all
	FLEXFLAGS = -d -Cem # --debug --ecs --meta-ecs
else
	BISONFLAGS = -Wnone # --warnings=none
	FLEXFLAGS = -Cfra # --align --full
endif

# Flex/bison
handlebars.tab.c: handlebars.lex.c
	${BISON} $(BISONFLAGS) -b handlebars handlebars.y
handlebars.lex.c:
	${FLEX} $(FLEXFLAGS) -o handlebars.lex.c handlebars.l

# Main
include_HEADERS = handlebars.tab.h

GENSOURCE = handlebars.tab.h handlebars.tab.c handlebars.lex.h handlebars.lex.c
CLEANFILES = $(GENSOURCE)

lib_LTLIBRARIES=libhandlebars.la

libhandlebars_la_SOURCES = $(GENSOURCE) \
	handlebars_context.h \
	handlebars_context.c \
	handlebars_token.h \
	handlebars_token.c \
	handlebars_token_list.h \
	handlebars_token_list.c \
	handlebars_token_printer.h \
	handlebars_token_printer.c \
	handlebars_utils.h \
	handlebars_utils.c

# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libhandlebars_la_LDFLAGS = \
	-no-undefined \
	-module \
	-shared \
	-export-symbols-regex '^handlebars' \
	-version-info $(HANDLEBARS_LT_CURRENT):$(HANDLEBARS_LT_REVISION):$(HANDLEBARS_LT_AGE)
#	-release $(HANDLEBARS_LT_RELEASE)
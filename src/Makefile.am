
EXTRA_DIST = handlebars.l handlebars.y

if YYDEBUG
	BISONFLAGS = -d -Wall # --debug --warnings=all
	FLEXFLAGS = -d -Cem # --debug --ecs --meta-ecs
else
	BISONFLAGS = -Wnone # --warnings=none
	FLEXFLAGS = -Cfra # --align --full
endif

# flex/bison
if BISON
handlebars.tab.h: handlebars.tab.c
handlebars.tab.c: handlebars.lex.c handlebars.y
	${BISON} $(BISONFLAGS) -b handlebars handlebars.y
endif
if FLEX
handlebars.lex.h: handlebars.lex.c
handlebars.lex.c: handlebars.l
	${FLEX} $(FLEXFLAGS) -o handlebars.lex.c handlebars.l
endif
	
# re2c
if RE2C
MAINTAINERCLEANFILES = handlebars_scanners.c
handlebars_scanners.c:
	$(RE2C) -si8 -o handlebars_scanners.c handlebars_scanners.re
endif

# Main
include_HEADERS = \
	handlebars.h \
	handlebars_ast.h \
	handlebars_ast_helpers.h \
	handlebars_ast_list.h \
	handlebars_ast_printer.h \
	handlebars_compiler.h \
	handlebars_context.h \
	handlebars_data.h \
	handlebars_memory.h \
	handlebars_opcode_printer.h \
	handlebars_opcodes.h \
	handlebars_scanners.h \
	handlebars_token.h \
	handlebars_token_list.h \
	handlebars_token_printer.h \
	handlebars_utils.h \
	handlebars_whitespace.h \
	handlebars.tab.h \
	handlebars.lex.h

GENSOURCE = handlebars.tab.h handlebars.tab.c handlebars.lex.h handlebars.lex.c
CLEANFILES = $(GENSOURCE)
BUILT_SOURCES = $(GENSOURCE)

lib_LTLIBRARIES=libhandlebars.la

libhandlebars_la_SOURCES = \
	$(GENSOURCE) \
	handlebars_memory.h \
	handlebars_memory.c \
	handlebars.h \
	handlebars.c \
	handlebars_ast.h \
	handlebars_ast.c \
	handlebars_ast_helpers.h \
	handlebars_ast_helpers.c \
	handlebars_ast_list.h \
	handlebars_ast_list.c \
	handlebars_ast_printer.h \
	handlebars_ast_printer.c \
	handlebars_compiler.h \
	handlebars_compiler.c \
	handlebars_context.h \
	handlebars_context.c \
	handlebars_data.h \
	handlebars_data.c \
	handlebars_opcode_printer.h \
	handlebars_opcode_printer.c \
	handlebars_opcodes.h \
	handlebars_opcodes.c \
	handlebars_private.h \
	handlebars_scanners.h \
	handlebars_scanners.c \
	handlebars_token.h \
	handlebars_token.c \
	handlebars_token_list.h \
	handlebars_token_list.c \
	handlebars_token_printer.h \
	handlebars_token_printer.c \
	handlebars_utils.h \
	handlebars_utils.c \
	handlebars_vm.h \
	handlebars_vm.c \
	handlebars_whitespace.h \
	handlebars_whitespace.c

# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libhandlebars_la_LDFLAGS = \
	-no-undefined \
	-version-info $(HANDLEBARS_LT_CURRENT):$(HANDLEBARS_LT_REVISION):$(HANDLEBARS_LT_AGE)
#	-export-symbols-regex '^_?handlebars' \
#	-release $(HANDLEBARS_LT_RELEASE)



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

# gperf
if GPERF
handlebars_helpers_ht.h: handlebars_helpers_ht.gperf
	$(GPERF) --struct-type --readonly-tables --pic --output-file=handlebars_helpers_ht.h handlebars_helpers_ht.gperf
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
    handlebars.lex.h \
    handlebars.tab.h \
	handlebars_ast.h \
	handlebars_ast_helpers.h \
	handlebars_ast_list.h \
	handlebars_ast_printer.h \
	handlebars_cache.h \
	handlebars_compiler.h \
	handlebars_config.h \
	handlebars_helpers.h \
	handlebars_map.h \
	handlebars_memory.h \
	handlebars_opcode_printer.h \
	handlebars_opcode_serializer.h \
	handlebars_opcodes.h \
	handlebars_private.h \
	handlebars_scanners.h \
	handlebars_stack.h \
	handlebars_string.h \
	handlebars_token.h \
	handlebars_utils.h \
	handlebars_value.h \
	handlebars_value_handlers.h \
	handlebars_vm.h \
	handlebars_whitespace.h

GENSOURCE = handlebars.tab.h handlebars.tab.c handlebars.lex.h handlebars.lex.c handlebars_helpers_ht.h
CLEANFILES = $(GENSOURCE)
BUILT_SOURCES = $(GENSOURCE)

lib_LTLIBRARIES=libhandlebars.la

libhandlebars_la_SOURCES = \
	$(GENSOURCE) \
	handlebars_config.h \
	handlebars_memory.h \
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
	handlebars_cache.h \
	handlebars_cache.c \
	handlebars_compiler.h \
	handlebars_compiler.c \
	handlebars_helpers.h \
	handlebars_helpers.c \
	handlebars_map.h \
	handlebars_map.c \
	handlebars_opcode_printer.h \
	handlebars_opcode_printer.c \
	handlebars_opcode_serializer.h \
	handlebars_opcode_serializer.c \
	handlebars_opcodes.h \
	handlebars_opcodes.c \
	handlebars_private.h \
	handlebars_scanners.h \
	handlebars_scanners.c \
	handlebars_stack.h \
	handlebars_stack.c \
	handlebars_string.h \
	handlebars_string.c \
	handlebars_token.h \
	handlebars_token.c \
	handlebars_utils.h \
	handlebars_utils.c \
	handlebars_value.h \
	handlebars_value.c \
	handlebars_value_handlers.h \
	handlebars_value_handlers.c \
	handlebars_vm.h \
	handlebars_vm.c \
	handlebars_whitespace.h \
	handlebars_whitespace.c

if HANDLEBARS_MEMORY
libhandlebars_la_SOURCES += handlebars_memory.c
endif

# https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
libhandlebars_la_LDFLAGS = \
	-no-undefined \
	-version-info $(HANDLEBARS_LT_CURRENT):$(HANDLEBARS_LT_REVISION):$(HANDLEBARS_LT_AGE)
#	-export-symbols-regex '^_?handlebars' \
#	-release $(HANDLEBARS_LT_RELEASE)


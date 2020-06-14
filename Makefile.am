

SUBDIRS = src . bin
if DOXYGEN
SUBDIRS += doc
endif
if CHECK
SUBDIRS += tests
endif

EXTRA_DIST = LICENSE.md LICENSE-AGPL.md LICENSE-GPL.md README.md CHANGELOG.md spec/handlebars/export spec/handlebars/spec spec/mustache/specs vendor

ACLOCAL_AMFLAGS = -I m4 --install

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = handlebars.pc

# Alias for test (old habits die hard)
test: check

my-clean-local:
	find -name 'vgcore.*' -delete

if CODE_COVERAGE_ENABLED
CODE_COVERAGE_OUTPUT_FILE = $(PACKAGE_NAME)-coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = $(PACKAGE_NAME)-coverage
CODE_COVERAGE_LCOV_RMOPTS = \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '/usr/*' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '/nix/store/*' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '*/tests/*' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '*/vendor/*' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '*/handlebars.tab.c' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '*/handlebars.lex.c' \
	--remove $(CODE_COVERAGE_OUTPUT_FILE) '*/handlebars_scanners.c'
include $(top_srcdir)/build/aminclude_static.am
clean-local: code-coverage-clean my-clean-local
distclean-local: code-coverage-dist-clean
else
clean-local: my-clean-local
endif

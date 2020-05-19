

SUBDIRS = src . bin
if DOXYGEN
SUBDIRS += doc
endif
if CHECK
SUBDIRS += tests
endif

EXTRA_DIST = LICENSE.md LICENSE-AGPL.md LICENSE-GPL.md README.md CHANGELOG.md spec/handlebars/export spec/handlebars/spec spec/mustache/specs vendor build/redhat-hardened-cc1 build/redhat-hardened-ld

ACLOCAL_AMFLAGS = -I m4 --install
AM_CFLAGS = $(WARN_CFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = handlebars.pc

# Alias for test (old habits die hard)
test: check

if CODE_COVERAGE_ENABLED
CODE_COVERAGE_OUTPUT_FILE = $(PACKAGE_NAME)-coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = $(PACKAGE_NAME)-coverage
include $(top_srcdir)/build/aminclude_static.am
clean-local: code-coverage-clean
distclean-local: code-coverage-dist-clean
endif

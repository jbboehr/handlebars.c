
SUBDIRS = src . bin tests

EXTRA_DIST = LICENSE README.md

ACLOCAL_AMFLAGS = -I m4

# These flags are gcc specific
if GCC
export AM_CFLAGS = -Wall -Wextra -Wdeclaration-after-statement  \
		-Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = handlebars.pc

# Alias for test (old habits die hard)
test: check

maintainer-clean-local:
	-rm -f aclocal.m4 config.h.in configure config/*
	-find ${builddir} -name Makefile.in -exec rm -f '{}' ';'

.PHONY: bootstrap
bootstrap: maintainer-clean
	./bootstrap

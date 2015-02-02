
SUBDIRS = src . doc bin tests

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

# Coverage
if GCOV
clean-local: gcov-clean lcov-clean

coverage-init: 
	$(LCOV) --directory . --zerocounters -q

coverage-main:
	$(LCOV) --capture --compat-libtool --directory . --output-file handlebars_c_test.info
	$(LCOV) --remove handlebars_c_test.info "/usr*" --compat-libtool --output-file handlebars_c_test.info
	genhtml -o test_coverage -t "handlebars.c test coverage" --num-spaces 4 handlebars_c_test.info

coverage: all coverage-init test coverage-main

gcov-clean:
	find . -name '*.gcda' -delete
	find . -name '*.gcno' -delete

lcov-clean:
	rm -rf test_coverage
	rm -f handlebars_c_test.info
endif

.PHONY: bootstrap
bootstrap: maintainer-clean
	./bootstrap

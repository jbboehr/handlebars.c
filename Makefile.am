
SUBDIRS = src . doc bin tests

EXTRA_DIST = LICENSE.md LICENSE-AGPL.md LICENSE-GPL.md README.md CHANGELOG.md spec/handlebars/export spec/handlebars/spec spec/mustache/specs vendor

ACLOCAL_AMFLAGS = -I m4 --install
AM_CFLAGS = $(WARN_CFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS)

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
	$(LCOV) --rc lcov_branch_coverage=1 --compat-libtool --directory . --capture --output-file handlebars_c_test.info
	$(LCOV) --rc lcov_branch_coverage=1 --compat-libtool --directory . --remove handlebars_c_test.info "/usr*" --remove handlebars_c_test.info "handlebars.tab.c" --remove handlebars_c_test.info "handlebars.lex.c" --remove handlebars_c_test.info "handlebars_scanners.c" --output-file handlebars_c_test.info
	genhtml --rc genhtml_branch_coverage=1 -o test_coverage -t "handlebars.c test coverage" --num-spaces 4 handlebars_c_test.info

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

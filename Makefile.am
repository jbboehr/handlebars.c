# Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

SUBDIRS = src . bin
if DOXYGEN
SUBDIRS += doc
endif
if CHECK
SUBDIRS += tests
endif
SUBDIRS += bench

EXTRA_DIST = LICENSE.md LICENSE-AGPL.md LICENSE-GPL.md README.md CHANGELOG.md spec/handlebars/export spec/handlebars/spec spec/mustache/specs vendor

ACLOCAL_AMFLAGS = -I m4 --install

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = handlebars.pc

# Alias for test (old habits die hard)
test: check

my-clean-local:
	find . -name 'vgcore.*' -delete

if CODE_COVERAGE_ENABLED
CODE_COVERAGE_OUTPUT_FILE = $(PACKAGE_NAME)-coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = $(PACKAGE_NAME)-coverage
CODE_COVERAGE_LCOV_RMOPTS = $(CODE_COVERAGE_LCOV_RMOPTS_DEFAULT) \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '/usr/*' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '/nix/store/*' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '*/tests/*' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '*/vendor/*' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '*/handlebars.tab.c' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '*/handlebars.lex.c' \
	--remove "$(CODE_COVERAGE_OUTPUT_FILE).tmp" '*/handlebars_scanners.c'
include $(top_srcdir)/build/aminclude_static.am
clean-local: code-coverage-clean my-clean-local
distclean-local: code-coverage-dist-clean
else
clean-local: my-clean-local
endif

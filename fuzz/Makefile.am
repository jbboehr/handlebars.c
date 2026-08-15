# Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

AM_CPPFLAGS = -I$(top_builddir)/src -I$(top_srcdir)/src
AM_CFLAGS = $(WARN_CFLAGS) $(HARDENING_BIN_CFLAGS) $(FUZZING_CFLAGS) $(JSON_CFLAGS) $(LMDB_CFLAGS) $(PTHREAD_CFLAGS) $(TALLOC_CFLAGS) $(YAML_CFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS) $(HARDENING_BIN_LDFLAGS) $(FUZZER_LDFLAGS)
LDADD = $(top_builddir)/src/libhandlebars.la $(JSON_LIBS) $(LMDB_LIBS) $(PTHREAD_LIBS) $(TALLOC_LIBS) $(YAML_LIBS)

EXTRA_DIST = README.md handlebars.dict corpus/template

if FUZZING
noinst_PROGRAMS = fuzz_template
endif

fuzz_template_SOURCES = fuzz_template.c

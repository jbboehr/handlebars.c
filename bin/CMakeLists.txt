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

set(SOURCES handlebarsc.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src ${CMAKE_CURRENT_SOURCE_DIR}/../src)

link_libraries(${LIBS} handlebars_static)

add_executable(handlebarsc handlebarsc.c)

install(TARGETS handlebarsc
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

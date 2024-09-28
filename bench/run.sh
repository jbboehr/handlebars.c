#!/usr/bin/env bash
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

set -e -o pipefail

# https://stackoverflow.com/a/4774063
SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

HANDLEBARSC="${SCRIPTPATH}/../bin/handlebarsc"
TIME=`which time`
BC=`which bc`
RUN_COUNT=10000

cd ${SCRIPTPATH}

function strip_extension() {
	set -e -o pipefail

	filename=$(basename -- "$1")
	extension="${filename##*.}"
	filename="${filename%.*}"
	echo $filename

	return 0
}

function run_test_inner() (
	set -e -o pipefail

	local template_path="templates/${1}"
	local DATA="templates/${2}"
	local EXTRA_OPTS="${3} ${4} --partial-loader --partial-path ./partials --partial-ext .handlebars"
	local time_outputfile=`mktemp`
	local actual_outputfile=`mktemp`
	local expected_file=templates/$(strip_extension $template_path).expected
	local time_format="real %e\nuser %U\nsys %S\nmrss %M KB"

	# header
	echo "----- Running: ${1} -----"
	if [ ! -z "${4}" ]; then
		echo "Extra flags: ${4}"
	fi

	# execute command
	${TIME} -f "${time_format}" \
		--output=${time_outputfile} \
		${HANDLEBARSC} \
		--run-count ${RUN_COUNT} \
		${EXTRA_OPTS} \
		--data ${DATA} \
		${template_path} \
		1>${actual_outputfile}

	# print time
	real_time=`cat ${time_outputfile} | grep real | awk '{ print $2 }'`
	real_microsecs_per_run=`${BC} -l <<< "1000000 * ${real_time}/${RUN_COUNT}"`
	echo "runs ${RUN_COUNT}"
	cat ${time_outputfile}
	printf "each %g us\n" "${real_microsecs_per_run}"

	# compare with expected file
	trap "echo FAIL; echo Expected: `cat ${expected_file}`; echo Actual: `cat ${actual_outputfile}`; echo; exit 1" ERR
	diff --ignore-all-space --text ${expected_file} ${actual_outputfile}
	trap - ERR
	echo "PASS"

	echo

	rm ${time_outputfile} ${actual_outputfile}

	return 0
)

function run_test() (
	set -e -o pipefail

	run_test_inner "${1}" "${2}" "${3}"

	run_test_inner "${1}" "${2}" "${3}" "--no-convert-input"

	return 0
)

run_test "array-each.handlebars" "array-each.json"
run_test "array-each.mustache" "array-each.json" "--flags compat"

run_test "complex.handlebars" "complex.json"
run_test "complex.mustache" "complex.json" "--flags compat"

run_test "data.handlebars" "data.json"

run_test "depth-1.handlebars" "depth-1.json"
run_test "depth-1.mustache" "depth-1.json" "--flags compat"

run_test "depth-2.handlebars" "depth-2.json"
run_test "depth-2.mustache" "depth-2.json" "--flags compat"

run_test "object-mustache.handlebars" "object-mustache.json"

run_test "object.handlebars" "object.json"
run_test "object.mustache" "object.json"  "--flags compat"

run_test "partial.handlebars" "partial.json"
run_test "partial.mustache" "partial.json"  "--flags compat"

run_test "partial-recursion.handlebars" "partial-recursion.json"
run_test "partial-recursion.mustache" "partial-recursion.json"  "--flags compat --partial-ext mustache"

run_test "paths.handlebars" "paths.json"
run_test "paths.mustache" "paths.json"  "--flags compat"

run_test "string.handlebars" "string.json"
run_test "string.mustache" "string.json"  "--flags compat"

run_test "variables.handlebars" "variables.json"
run_test "variables.mustache" "variables.json"  "--flags compat"

exit 0

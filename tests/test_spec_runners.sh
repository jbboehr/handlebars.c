#!/bin/sh

# Test functions are invoked indirectly by check.
# shellcheck disable=SC2329

set -eu

: "${SPEC_COMPILER:?SPEC_COMPILER is required}"
: "${SPEC_PARSER:?SPEC_PARSER is required}"
: "${SPEC_TOKENIZER:?SPEC_TOKENIZER is required}"
: "${SPEC_RUNTIME:?SPEC_RUNTIME is required}"
: "${SPEC_RUNTIME_SOURCE:?SPEC_RUNTIME_SOURCE is required}"

# Each child must run its entire fixture selection and report normal Check
# output, independently of filters and log destinations in the caller.
unset CK_RUN_SUITE CK_RUN_CASE CK_INCLUDE_TAGS CK_EXCLUDE_TAGS
unset CK_LOG_FILE_NAME CK_XML_LOG_FILE_NAME CK_TAP_LOG_FILE_NAME TEST_NUM TEST_RUNS
CK_VERBOSITY=normal
export CK_VERBOSITY

temporary=$(mktemp -d "${TMPDIR:-/tmp}/handlebars-spec-runners.XXXXXX")
trap 'rm -f "$temporary"/export/*.json "$temporary"/runtime/*.json "$temporary"/*.json "$temporary"/*.log; rmdir "$temporary/export" "$temporary/runtime" "$temporary"' 0
mkdir "$temporary/export" "$temporary/runtime"
log="$temporary/runner.log"
checked=0
failed=0

check() {
    description=$1
    shift
    checked=$((checked + 1))
    if "$@"; then
        echo "ok $checked - $description"
    else
        echo "not ok $checked - $description"
        sed 's/^/# /' "$log"
        failed=1
    fi
}

run_fixture() {
    case "$runner" in
        compiler) env handlebars_export_dir="$temporary/export" "$SPEC_COMPILER" ;;
        parser) env handlebars_parser_spec="$fixture" "$SPEC_PARSER" ;;
        tokenizer) env handlebars_tokenizer_spec="$fixture" "$SPEC_TOKENIZER" ;;
    esac
}

singleton_runs() {
    run_fixture >"$log" 2>&1 &&
        grep -Fxq 'Loaded 1 test cases' "$log" &&
        grep -Fq 'Checks: 1, Failures: 0, Errors: 0' "$log"
}

last_fixture_fails() {
    status=0
    run_fixture >"$log" 2>&1 || status=$?
    test "$status" -eq 1 &&
        grep -Fxq 'Loaded 2 test cases' "$log" &&
        grep -Fq 'Checks: 2, Failures: 1, Errors: 0' "$log" &&
        grep -Fq 'RUNNER_SENTINEL' "$log"
}

trailing_content_rejected() {
    status=0
    run_fixture >"$log" 2>&1 || status=$?
    test "$status" -ne 0 &&
        ! grep -Fq 'Running suite(s):' "$log"
}

# The compiler runner expects all export files. Put the controlled cases in
# basic.json and leave the other groups empty.
for name in basic blocks builtins data helpers partials regressions strict \
    string-params subexpressions track-ids whitespace-control
do
    printf '[]\n' >"$temporary/export/$name.json"
done

for runner in compiler parser tokenizer; do
    fixture="$temporary/fixture.json"
    case "$runner" in
        compiler)
            fixture="$temporary/export/basic.json"
            good='{"description":"runner contract","it":"literal","template":"ok","opcodes":{"opcodes":[{"opcode":"appendContent","args":["ok"]}],"children":[]}}'
            bad='{"description":"runner contract","it":"last fixture","template":"ok","opcodes":{"opcodes":[{"opcode":"appendContent","args":["RUNNER_SENTINEL"]}],"children":[]}}'
            ;;
        parser)
            good='{"description":"runner contract","it":"number","template":"{{123}}","expected":"{{ NUMBER{123} [] }}\n"}'
            bad='{"description":"runner contract","it":"last fixture","template":"{{123}}","expected":"RUNNER_SENTINEL"}'
            ;;
        tokenizer)
            good='{"description":"runner contract","it":"content","template":"ok","expected":[{"name":"CONTENT","text":"ok"}]}'
            bad='{"description":"runner contract","it":"last fixture","template":"ok","expected":[{"name":"CONTENT","text":"RUNNER_SENTINEL"}]}'
            ;;
    esac
    printf '[%s]\n' "$good" >"$fixture"
    check "$runner executes a singleton fixture" singleton_runs
    printf '[%s] \t\r\n' "$good" >"$fixture"
    check "$runner accepts trailing JSON whitespace" singleton_runs
    printf '[%s]// comment' "$good" >"$fixture"
    check "$runner accepts a json-c line comment at EOF" singleton_runs
    printf '[%s]\n{}\n' "$good" >"$fixture"
    check "$runner rejects content after its fixture array" trailing_content_rejected
    printf '[%s]\000{}\n' "$good" >"$fixture"
    check "$runner rejects content after an embedded NUL" trailing_content_rejected
    printf '[%s,%s]\n' "$good" "$bad" >"$fixture"
    check "$runner executes the final fixture" last_fixture_fails
done

# This inventory is independent of the runtime loader. Work only on copies
# so missing-file checks cannot change the real specification fixtures.
runtime_suites='basic blocks builtins data helpers partials regressions strict subexpressions whitespace-control'
for name in $runtime_suites; do
    cp "$SPEC_RUNTIME_SOURCE/$name.json" "$temporary/runtime/$name.json"
done

run_runtime() {
    # Loading and the exclusion budget still cover the complete inventory.
    # One render case is enough to check successful runner startup here.
    env handlebars_spec_dir="$temporary/runtime" TEST_NUM=0 "$SPEC_RUNTIME"
}

runtime_starts() {
    # TEST_NUM selects one fixture, plus the exclusion-budget check. Exception
    # and excluded fixtures may omit the AST check or the runtime check.
    run_runtime >"$log" 2>&1 &&
        grep -Fq 'Running suite(s):' "$log" &&
        grep -Eq 'Checks: [1-3], Failures: 0, Errors: 0' "$log"
}

runtime_rejects_unavailable() {
    status=0
    run_runtime >"$log" 2>&1 || status=$?
    test "$status" -eq 1 &&
        grep -Fq "Failed to read spec file: $temporary/runtime/$name.json" "$log" &&
        ! grep -Fq 'Running suite(s):' "$log"
}

runtime_rejects_invalid() {
    status=0
    run_runtime >"$log" 2>&1 || status=$?
    test "$status" -eq 1 && grep -Fq "$1" "$log" &&
        ! grep -Fq 'Running suite(s):' "$log"
}

check 'runtime starts with all required fixture files' runtime_starts
for name in $runtime_suites; do
    rm "$temporary/runtime/$name.json"
    check "runtime rejects missing $name.json before running tests" runtime_rejects_unavailable
    cp "$SPEC_RUNTIME_SOURCE/$name.json" "$temporary/runtime/$name.json"
done

name=basic
chmod a-r "$temporary/runtime/$name.json"
if test -r "$temporary/runtime/$name.json"; then
    checked=$((checked + 1))
    echo "ok $checked - unreadable fixture # SKIP current user can still read it"
else
    check "runtime rejects unreadable $name.json before running tests" runtime_rejects_unavailable
fi
chmod u+r "$temporary/runtime/$name.json"

# distcheck makes source fixtures read-only. Only this private copy is edited.
chmod u+w "$temporary/runtime/whitespace-control.json"
printf '{\n' >"$temporary/runtime/whitespace-control.json"
check 'runtime rejects malformed fixture JSON' runtime_rejects_invalid \
    "Failed to parse JSON in spec file: $temporary/runtime/whitespace-control.json"
printf '{}\n' >"$temporary/runtime/whitespace-control.json"
check 'runtime rejects a non-array fixture root' runtime_rejects_invalid \
    "Root JSON value was not array in spec file: $temporary/runtime/whitespace-control.json"
cp "$SPEC_RUNTIME_SOURCE/whitespace-control.json" "$temporary/runtime/whitespace-control.json"
printf '{}\n' >>"$temporary/runtime/whitespace-control.json"
check 'runtime rejects content after a fixture array' runtime_rejects_invalid \
    "Failed to parse JSON in spec file: $temporary/runtime/whitespace-control.json"
cp "$SPEC_RUNTIME_SOURCE/whitespace-control.json" "$temporary/runtime/whitespace-control.json"
printf ' \t\r\n' >>"$temporary/runtime/whitespace-control.json"
check 'runtime accepts trailing JSON whitespace' runtime_starts
cp "$SPEC_RUNTIME_SOURCE/whitespace-control.json" "$temporary/runtime/whitespace-control.json"
printf '// comment' >>"$temporary/runtime/whitespace-control.json"
check 'runtime accepts a json-c line comment at EOF' runtime_starts
cp "$SPEC_RUNTIME_SOURCE/whitespace-control.json" "$temporary/runtime/whitespace-control.json"
printf '\000{}\n' >>"$temporary/runtime/whitespace-control.json"
check 'runtime rejects content after an embedded NUL' runtime_rejects_invalid \
    "Failed to parse JSON in spec file: $temporary/runtime/whitespace-control.json"

echo "1..$checked"
exit "$failed"

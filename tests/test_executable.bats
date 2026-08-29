#!/usr/bin/env bats

if [ -z "$HANDLEBARSC" ]; then
    HANDLEBARSC="./bin/handlebarsc"
fi

if [ -z "$TEST_DIR" ]; then
    TEST_DIR="./tests"
fi

if [ -z "$BENCH_DIR" ]; then
    BENCH_DIR="./bench"
fi

TEMPLATE="$TEST_DIR/fixture1.hbs"
PARTIAL_FLAGS=(--partial-loader --partial-path "$BENCH_DIR/partials" --partial-ext .handlebars)

function setup {
    if [ -z "${BATS_TEST_TMPDIR:-}" ]; then
        BATS_TEST_TMPDIR="$BATS_RUN_TMPDIR/test-$BATS_TEST_NUMBER"
        mkdir -p "$BATS_TEST_TMPDIR"
    fi
}

if ! "$HANDLEBARSC" --debuginfo 2>&1 | grep -i 'JSON support: enabled' >/dev/null; then
    HAVE_JSON=true
else
    HAVE_JSON=false
fi
if ! "$HANDLEBARSC" --debuginfo 2>&1 | grep -i 'YAML support: enabled' >/dev/null; then
    HAVE_YAML=true
else
    HAVE_YAML=false
fi

function skip_if_no_json {
    if [ "$HAVE_JSON" = "true" ]; then
        skip
    fi
    return 0
}

function skip_if_no_yaml {
    if [ "$HAVE_YAML" = "true" ]; then
        skip
    fi
    return 0
}

function helper_process_is_running {
    local pid="$1"
    local state

    if ! kill -0 "$pid" 2>/dev/null; then
        return 1
    fi
    state=$(ps -o stat= -p "$pid" 2>/dev/null) || return 1
    case "$state" in
        *Z*) return 1 ;;
        *) return 0 ;;
    esac
}

function wait_for_helper_process_exit {
    local pid="$1"
    local attempt

    for attempt in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
        if ! helper_process_is_running "$pid"; then
            return 0
        fi
        sleep 0.05
    done
    return 1
}

load "../vendor/bats-support/output"
load "../vendor/bats-support/error"
load "../vendor/bats-support/lang"
load "../vendor/bats-assert/assert"

@test "no arguments produces help" {
    run "$HANDLEBARSC"
    assert_failure
    assert_output --partial "Usage: handlebarsc"
}

@test "invalid option" {
    run "$HANDLEBARSC" --invalid
    assert_failure
    assert_output --partial "unrecognized option"
}

@test "--help" {
    run "$HANDLEBARSC" --help
    assert_success
    assert_output --partial "Usage: handlebarsc"
    assert_output --partial "Example: handlebarsc"
}

@test "--version" {
    run "$HANDLEBARSC" --version
    assert_success
    assert_output --partial "handlebarsc v"
    assert_output --partial "Affero"
}

@test "--debuginfo" {
    run "$HANDLEBARSC" --debuginfo
    assert_success
    assert_output --partial "JSON support"
    assert_output --partial "YAML support"
    assert_output --partial "XXHash version"
}

@test "--lex" {
    run "$HANDLEBARSC" --lex "$TEMPLATE"
    assert_output --partial "OPEN "
}

@test "--lex (invalid file)" {
    run "$HANDLEBARSC" --lex nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--lex (no file)" {
    run "$HANDLEBARSC" --lex
    assert_failure
    assert_output --partial "No input file"
}

@test "--lex (empty file)" {
    local empty_file
    empty_file=$(mktemp)
    run "$HANDLEBARSC" --lex "$empty_file"
    rm "$empty_file"
    assert_success
    assert_output ""
}

@test "--lex --template <TEMPLATE>" {
    run "$HANDLEBARSC" --lex --template "$TEST_DIR/fixture1.hbs"
    assert_success
    assert_output --partial "OPEN "
}

@test "--lex -" {
    run bash -c '"$1" --lex - < "$2"' _ "$HANDLEBARSC" "$TEMPLATE"
    assert_success
    assert_output --partial "OPEN "
}

@test "--parse" {
    run "$HANDLEBARSC" --parse "$TEMPLATE"
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--parse (invalid file)" {
    run "$HANDLEBARSC" --parse nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--parse (no file)" {
    run "$HANDLEBARSC" --parse
    assert_failure
    assert_output --partial "No input file"
}

@test "--parse (parse error)" {
    run "$HANDLEBARSC" --parse "$TEST_DIR/fixture2.hbs"
    assert_failure
    assert_output --partial "syntax error"
}

@test "--parse -" {
    run bash -c '"$1" --parse - < "$2"' _ "$HANDLEBARSC" "$TEMPLATE"
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--parse --template <TEMPLATE>" {
    run "$HANDLEBARSC" --parse --template "$TEMPLATE"
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--compile" {
    run "$HANDLEBARSC" --compile "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile (invalid file)" {
    run "$HANDLEBARSC" --compile nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--compile (no file)" {
    run "$HANDLEBARSC" --compile
    assert_failure
    assert_output --partial "No input file"
}

@test "--compile (compile error)" {
    run "$HANDLEBARSC" --compile "$TEST_DIR/fixture3.hbs"
    assert_failure
    assert_output --partial "Unsupported number of partial arguments"
}

@test "--compile -" {
    run bash -c '"$1" --compile - < "$2"' _ "$HANDLEBARSC" "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile --template <TEMPLATE>" {
    run "$HANDLEBARSC" --compile "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile --flags no_escape" {
    run "$HANDLEBARSC" --compile --flags no_escape "$TEMPLATE"
    assert_output --partial "append"
    refute_output --partial "appendEscaped"
}

@test "--module" {
    run "$HANDLEBARSC" --module "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module (invalid file)" {
    run "$HANDLEBARSC" --module nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--module (no file)" {
    run "$HANDLEBARSC" --module
    assert_failure
    assert_output --partial "No input file"
}

@test "--module (compile error)" {
    run "$HANDLEBARSC" --module "$TEST_DIR/fixture3.hbs"
    assert_failure
    assert_output --partial "Unsupported number of partial arguments"
}

@test "--module -" {
    run bash -c '"$1" --module - < "$2"' _ "$HANDLEBARSC" "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module --template <TEMPLATE>" {
    run "$HANDLEBARSC" --module "$TEMPLATE"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module --flags no_escape" {
    run "$HANDLEBARSC" --module --flags no_escape "$TEMPLATE"
    assert_output --partial "append"
    refute_output --partial "appendEscaped"
}

@test "--execute" {
    skip_if_no_json
    run "$HANDLEBARSC" --execute --data "$TEST_DIR/fixture1.json" "$TEMPLATE"
    assert_success
    assert_output "|bar|"
}

@test "--execute (invalid file)" {
    run "$HANDLEBARSC" --execute nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--execute (directory)" {
    run "$HANDLEBARSC" --execute "$TEST_DIR"
    assert_failure
    assert_output --partial "Input is not a regular file"
}

@test "--execute (no file)" {
    run "$HANDLEBARSC" --execute
    assert_failure
    assert_output --partial "No input file"
}

@test "--execute (empty file)" {
    local empty_file
    empty_file=$(mktemp)
    run "$HANDLEBARSC" --execute "$empty_file"
    rm "$empty_file"
    assert_success
    assert_output ""
}

@test "--execute (is default mode)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$TEST_DIR/fixture1.json" "$TEMPLATE"
    assert_success
    assert_output "|bar|"
}

@test "--execute (yaml)" {
    skip_if_no_yaml
    run "$HANDLEBARSC" --execute --data "$TEST_DIR/fixture1.yaml" "$TEMPLATE"
    assert_success
    assert_output "|bar|"
}

@test "--execute --template <TEMPLATE>" {
    skip_if_no_json
    run "$HANDLEBARSC" --execute --data "$TEST_DIR/fixture1.json" --template "$TEMPLATE"
    assert_success
    assert_output "|bar|"
}

@test "--execute --flags strict (runtime error)" {
    run "$HANDLEBARSC" --execute --flags strict "$TEMPLATE"
    assert_failure
    assert_output --partial '"foo" not defined in object'
}

@test "--execute bare @partial-block" {
    run bash -c 'printf "%s" "{{@partial-block}}" | "$1" --execute -' _ "$HANDLEBARSC"
    assert_success
    assert_output ""
}

@test "--execute bare @partial-block (strict)" {
    run bash -c 'printf "%s" "{{@partial-block}}" | "$1" --execute --flags strict -' _ "$HANDLEBARSC"
    assert_failure
    assert_output --partial '"partial-block" not defined in object'
}

@test "--execute empty trimmed block preserves whitespace semantics" {
    run bash -c 'printf "%s" "A  {{~#each x~}}{{/each}}  B" | "$1" --execute -' _ "$HANDLEBARSC"
    assert_success
    assert_output "A  B"
}

@test "--execute empty right-trimmed block preserves whitespace semantics" {
    run bash -c 'printf "%s" "A  {{#each x~}}{{/each}}  B" | "$1" --execute -' _ "$HANDLEBARSC"
    assert_success
    assert_output "A    B"
}

@test "--execute -n" {
    skip_if_no_json
    # wc on OSX outputs leading whitespace
    result1=$("$HANDLEBARSC" --execute --data "$TEST_DIR/fixture1.yaml" "$TEMPLATE" | wc -l | sed 's/ *//g')
    assert_equal "$result1" "1"
    result2=$("$HANDLEBARSC" --execute -n --data "$TEST_DIR/fixture1.yaml" "$TEMPLATE" | wc -l | sed 's/ *//g')
    assert_equal "$result2" "0"
    result3=$("$HANDLEBARSC" --execute --no-newline --data "$TEST_DIR/fixture1.yaml" "$TEMPLATE" | wc -l | sed 's/ *//g')
    assert_equal "$result3" "0"
}

@test "--execute --pool-size 0" {
    # not really any way to check if this works, just checking if nothing is broken when specified
    skip_if_no_yaml
    run "$HANDLEBARSC" --execute --pool-size 0 --data "$TEST_DIR/fixture1.json" "$TEST_DIR/fixture1.hbs"
    assert_success
    assert_output "|bar|"
}

@test "--helper-exec invokes an executable with primitive arguments" {
    run bash -c 'printf "%s" "A{{external \"hello\" 42}}B" | "$1" --execute --no-newline --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output "Ahello|42B"
}

@test "--helper-exec resolves a bare executable through PATH" {
    run bash -c 'printf "%s" "{{external \"path\"}}" | PATH="$2:$PATH" "$1" --execute --no-newline --helper-exec external=helper_executable.sh -' _ \
        "$HANDLEBARSC" "$TEST_DIR"
    assert_success
    assert_output "path"
}

@test "--helper-exec returns an ordinary escaped Handlebars string" {
    run bash -c 'printf "%s" "{{external \"<strong>plain</strong>\"}}" | "$1" --execute --no-newline --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output "&lt;strong&gt;plain&lt;/strong&gt;"
}

@test "--helper-exec rejects unsupported invocation shapes" {
    run bash -c 'printf "%s" "{{external value key=1}}" | "$1" --execute --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "does not support hash arguments"

    run bash -c 'printf "%s" "{{#external}}x{{/external}}" | "$1" --execute --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "does not support block invocation"

    if [ "$HAVE_YAML" = "false" ]; then
        run bash -c 'printf "%s" "{{external this}}" | "$1" --execute --data "$3" --helper-exec "external=$2" -' _ \
            "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/fixture1.yaml"
        assert_failure
        assert_output --partial "unsupported argument type"
    elif [ "$HAVE_JSON" = "false" ]; then
        run bash -c 'printf "%s" "{{external this}}" | "$1" --execute --data "$3" --helper-exec "external=$2" -' _ \
            "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/fixture1.json"
        assert_failure
        assert_output --partial "unsupported argument type"
    fi
}

@test "external helper failures do not publish partial render output" {
    run bash -c 'printf "%s" "SENTINEL_BEFORE{{external \"fail\"}}after" | "$1" --execute --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    refute_output --partial "SENTINEL_BEFORE"
    assert_output --partial "fixture stderr"
    assert_output --partial "exited with status 7"
}

@test "external helper nonzero exits terminate descendants discovered after I/O completes" {
    local pid_file
    local descendant_pid
    local descendant_leaked=false

    pid_file=$(mktemp)
    run bash -c 'printf "%s" "SENTINEL_BEFORE{{external \"fail-descendant\"}}after" | HELPER_PID_FILE="$3" "$1" --execute --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$pid_file"
    if [ -s "$pid_file" ]; then
        descendant_pid=$(cat "$pid_file")
        if ! wait_for_helper_process_exit "$descendant_pid"; then
            descendant_leaked=true
            kill "$descendant_pid" 2>/dev/null || true
        fi
    else
        descendant_leaked=missing-pid
    fi
    rm "$pid_file"

    assert_failure
    refute_output --partial "SENTINEL_BEFORE"
    assert_output --partial "exited with status 7"
    assert_equal "$descendant_leaked" false
}

@test "external helper protocol errors terminate descendants discovered after I/O completes" {
    local pid_file
    local descendant_pid
    local descendant_leaked=false

    skip_if_no_json
    pid_file=$(mktemp)
    run bash -c 'printf "%s" "SENTINEL_BEFORE{{jsonInvalidDescendant}}after" | HELPER_PID_FILE="$3" "$1" --execute --helper-json "jsonInvalidDescendant=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$pid_file"
    if [ -s "$pid_file" ]; then
        descendant_pid=$(cat "$pid_file")
        if ! wait_for_helper_process_exit "$descendant_pid"; then
            descendant_leaked=true
            kill "$descendant_pid" 2>/dev/null || true
        fi
    else
        descendant_leaked=missing-pid
    fi
    rm "$pid_file"

    assert_failure
    refute_output --partial "SENTINEL_BEFORE"
    assert_output --partial "invalid JSON response"
    assert_equal "$descendant_leaked" false
}

@test "successful external helpers terminate stdio-closed descendants" {
    local pid_file
    local descendant_pid
    local descendant_leaked=false

    pid_file=$(mktemp)
    run bash -c 'printf "%s" "{{external \"success-descendant\"}}" | HELPER_PID_FILE="$3" "$1" --execute --no-newline --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$pid_file"
    if [ -s "$pid_file" ]; then
        descendant_pid=$(cat "$pid_file")
        if ! wait_for_helper_process_exit "$descendant_pid"; then
            descendant_leaked=true
            kill "$descendant_pid" 2>/dev/null || true
        fi
    else
        descendant_leaked=missing-pid
    fi
    rm "$pid_file"

    assert_success
    assert_output complete
    assert_equal "$descendant_leaked" false
}

@test "external helper timeout bounds continuously readable output" {
    command -v timeout >/dev/null || skip "timeout command unavailable"
    run timeout 1s bash -c 'printf "%s" "{{external \"stream\"}}" | "$1" --execute --helper-timeout-ms 20 --helper-output-limit 0 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "timed out"
}

@test "external helper timeout and output limits terminate the child" {
    run bash -c 'printf "%s" "{{external \"sleep\"}}" | "$1" --execute --helper-timeout-ms 20 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "timed out"

    run bash -c 'printf "%s" "{{external \"large\"}}" | "$1" --execute --helper-output-limit 16 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "output limit"

    run bash -c 'printf "%s" "{{external \"large\"}}" | "$1" --execute --no-newline --helper-output-limit 128 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_equal "${#output}" 128
}

@test "external helper completion wins over the deadline after stdout closes" {
    run bash -c 'printf "%s" "{{external \"close-stdout\"}}" | "$1" --execute --no-newline --helper-timeout-ms 20 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output ""
}

@test "observable helper completion wins over the deadline with buffered stdout" {
    local template_file="$BATS_TEST_TMPDIR/helper-buffered-completion.hbs"

    printf '%s' '{{external "stop-parent-then-complete"}}' > "$template_file"
    run "$HANDLEBARSC" --execute --no-newline \
        --helper-timeout-ms 20 \
        --helper-exec "external=$TEST_DIR/helper_executable.sh" \
        "$template_file"

    assert_success
    assert_output complete
}

@test "successful helpers remain waitable when SIGCHLD is inherited ignored" {
    local template_file="$BATS_TEST_TMPDIR/helper-ignored-sigchld.hbs"

    printf '%s' '{{external}}' > "$template_file"
    run bash -c '
        trap "" CHLD
        exec "$1" --execute --no-newline --helper-exec "external=$2" "$3"
    ' _ "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$template_file"

    assert_success
    assert_output ""
}

@test "external helper deadline still applies after stdout closes" {
    command -v timeout >/dev/null || skip "timeout command unavailable"
    run timeout 1s bash -c 'printf "%s" "{{external \"close-stdout-late\"}}" | "$1" --execute --helper-timeout-ms 20 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "timed out"
}

@test "external helper timeout terminates a descendant holding stdout open" {
    local pid_file
    local descendant_pid
    local descendant_leaked=false

    command -v timeout >/dev/null || skip "timeout command unavailable"
    pid_file="$BATS_TEST_TMPDIR/timeout-descendant.pid"
    run timeout 1s bash -c 'printf "%s" "{{external \"timeout-descendant\"}}" | HELPER_PID_FILE="$3" "$1" --execute --helper-timeout-ms 20 --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$pid_file"
    if [ -s "$pid_file" ]; then
        descendant_pid=$(cat "$pid_file")
        if ! wait_for_helper_process_exit "$descendant_pid"; then
            descendant_leaked=true
            kill "$descendant_pid" 2>/dev/null || true
        fi
    else
        descendant_leaked=missing-pid
    fi
    assert_failure
    assert_output --partial "timed out"
    assert_equal "$descendant_leaked" false
}

@test "JSON helpers work for every initially closed stdio combination" {
    local mask
    local request_file
    local template_file

    skip_if_no_json
    request_file="$BATS_TEST_TMPDIR/helper-request.json"
    template_file="$BATS_TEST_TMPDIR/template.hbs"
    printf '%s' '{{jsonSafe}}' > "$template_file"

    for mask in 1 2 3 4 5 6 7; do
        : > "$request_file"
        run bash -c '
            case "$1" in
                1) exec 0<&- ;;
                2) exec 1>&- ;;
                3) exec 0<&- 1>&- ;;
                4) exec 2>&- ;;
                5) exec 0<&- 2>&- ;;
                6) exec 1>&- 2>&- ;;
                7) exec 0<&- 1>&- 2>&- ;;
            esac
            HELPER_REQUEST_FILE="$5" "$2" --execute --no-newline --helper-json "jsonSafe=$3" "$4"
        ' _ "$mask" "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$template_file" "$request_file"

        assert_success "closed stdio mask $mask"
        if (( (mask & 2) == 0 )); then
            assert_output '<strong>safe</strong>'
        else
            assert_output ""
        fi
        grep -q '"helper":"jsonSafe"' "$request_file"
    done
}

@test "external helper spawn failures are reported" {
    run bash -c 'printf "%s" "{{external}}" | "$1" --execute --helper-exec external=/definitely/not/an/executable -' _ \
        "$HANDLEBARSC"
    assert_failure
    assert_output --partial "Failed to execute external helper external"
}

@test "external helper registration validates collisions and permits explicit override" {
    run bash -c 'printf "%s" "{{external}}" | "$1" --execute --helper-exec "external=$2" --helper-exec "external=$3" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/helper_executable_alt.sh"
    assert_failure
    assert_output --partial "registered more than once"

    run bash -c 'printf "%s" "{{if \"value\"}}" | "$1" --execute --helper-exec "if=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "conflicts with a built-in helper"

    run bash -c 'printf "%s" "{{if \"value\"}}" | "$1" --execute --no-newline --allow-helper-override --helper-exec "if=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output "value"

    run bash -c 'printf "%s" "{{external}}" | "$1" --execute --no-newline --allow-helper-override --helper-exec "external=$2" --helper-exec "external=$3" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/helper_executable_alt.sh"
    assert_success
    assert_output "alternate"
}

@test "external helper registration participates in compile and module modes" {
    run bash -c 'printf "%s" "{{external \"value\"}}" | "$1" --compile --flags known_helpers_only --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output --partial "invokeKnownHelper"

    run bash -c 'printf "%s" "{{external \"value\"}}" | "$1" --module --flags known_helpers_only --helper-exec "external=/definitely/not/an/executable" -' _ \
        "$HANDLEBARSC"
    assert_success
}

@test "lex and parse modes reject external helper options" {
    run bash -c 'printf "%s" "{{external}}" | "$1" --parse --helper-exec "external=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "only valid in compile, module, and execute modes"
}

@test "external helper option values are validated" {
    run "$HANDLEBARSC" --execute --helper-exec invalid "$TEST_DIR/fixture1.hbs"
    assert_failure
    assert_output --partial "NAME=COMMAND"

    run "$HANDLEBARSC" --execute --helper-timeout-ms invalid "$TEST_DIR/fixture1.hbs"
    assert_failure
    assert_output --partial "non-negative integer"
}

@test "--helper-json sends structured requests and accepts structured values" {
    skip_if_no_json
    run bash -c 'printf "%s" "{{jsonRequest \"a\" 2 true null items key=\"v\"}}" | "$1" --execute --no-newline --data "$3" --helper-json "jsonRequest=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/helper_data.json"
    assert_success
    assert_output "request-ok"

    run bash -c 'printf "%s" "{{jsonNested this}}" | "$1" --execute --no-newline --no-convert-input --data "$3" --helper-json "jsonNested=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh" "$TEST_DIR/helper_data.json"
    assert_success
    assert_output "nested-ok"
}

@test "--helper-json rejects malformed outbound UTF-8 before invoking the helper" {
    local data_file
    local marker_file
    local malformed_kind
    local template_file="$BATS_TEST_TMPDIR/helper-json-invalid-outbound-utf8.hbs"

    skip_if_no_json
    printf '%s' 'SENTINEL_BEFORE{{jsonInvalidOutbound this}}after' > "$template_file"

    for malformed_kind in value key; do
        data_file="$BATS_TEST_TMPDIR/helper-json-invalid-outbound-$malformed_kind.json"
        marker_file="$BATS_TEST_TMPDIR/helper-json-invalid-outbound-$malformed_kind.marker"
        case "$malformed_kind" in
            value) printf '%b' '{"value":"\377"}' > "$data_file" ;;
            key) printf '%b' '{"\377":"value"}' > "$data_file" ;;
        esac

        run env HELPER_MARKER_FILE="$marker_file" \
            "$HANDLEBARSC" --execute --no-newline \
                --data "$data_file" \
                --helper-json "jsonInvalidOutbound=$TEST_DIR/helper_executable.sh" \
                "$template_file"

        assert_failure
        refute_output --partial "SENTINEL_BEFORE"
        assert_output --partial "malformed UTF-8"
        [ ! -e "$marker_file" ]
    done
}

@test "--helper-json rejects malformed UTF-8 helper names before invoking the helper" {
    local helper_name
    local marker_file="$BATS_TEST_TMPDIR/helper-json-invalid-name.marker"
    local template_file="$BATS_TEST_TMPDIR/helper-json-invalid-name.hbs"

    skip_if_no_json
    printf -v helper_name '%b' 'jsonInvalidName\377'
    printf 'SENTINEL_BEFORE{{%s}}after' "$helper_name" > "$template_file"

    run env HELPER_MARKER_FILE="$marker_file" \
        "$HANDLEBARSC" --execute --no-newline \
            --helper-json "$helper_name=$TEST_DIR/helper_executable.sh" \
            "$template_file"

    assert_failure
    refute_output --partial "SENTINEL_BEFORE"
    if [ -e "$marker_file" ]; then
        fail "external helper was invoked"
    fi
    assert_output --partial "malformed UTF-8"
}

@test "--helper-json controls escaping explicitly" {
    skip_if_no_json
    run bash -c 'printf "%s" "{{jsonSafe}}|{{jsonEscaped}}" | "$1" --execute --no-newline --helper-json "jsonSafe=$2" --helper-json "jsonEscaped=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output "<strong>safe</strong>|&lt;strong&gt;escaped&lt;/strong&gt;"
}

@test "--helper-json returns structured values and rejects block use" {
    skip_if_no_json
    run bash -c 'printf "%s" "{{#each (jsonArray)}}{{this}}|{{/each}}" | "$1" --execute --no-newline --helper-json "jsonArray=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_success
    assert_output "1|x|true|"

    run bash -c 'printf "%s" "{{#jsonArray}}body{{/jsonArray}}" | "$1" --execute --helper-json "jsonArray=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "does not support block invocation"
}

@test "--helper-json propagates protocol errors atomically" {
    skip_if_no_json
    run bash -c 'printf "%s" "before{{jsonError}}after" | "$1" --execute --helper-json "jsonError=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    refute_output --partial "before"
    assert_output --partial "fixture rejected the request"

    run bash -c 'printf "%s" "before{{jsonInvalid}}after" | "$1" --execute --helper-json "jsonInvalid=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    refute_output --partial "before"
    assert_output --partial "invalid JSON response"

    run bash -c 'printf "%s" "{{jsonVersion}}" | "$1" --execute --helper-json "jsonVersion=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "invalid JSON response envelope"

    run bash -c 'printf "%s" "{{jsonUnsafe}}" | "$1" --execute --helper-json "jsonUnsafe=$2" -' _ \
        "$HANDLEBARSC" "$TEST_DIR/helper_executable.sh"
    assert_failure
    assert_output --partial "marked a non-string value as safe"
}

@test "--helper-json rejects non-JSON parser extensions" {
    local helper_name
    local template_file="$BATS_TEST_TMPDIR/helper-json-extension.hbs"

    skip_if_no_json
    for helper_name in jsonNaN jsonInfinity jsonComment; do
        printf '{{%s}}' "$helper_name" > "$template_file"
        run "$HANDLEBARSC" --execute --no-newline \
            --helper-json "$helper_name=$TEST_DIR/helper_executable.sh" \
            "$template_file"

        assert_failure
        assert_output --partial "invalid JSON response"
    done
}

@test "--helper-json rejects non-JSON control whitespace" {
    local helper_name
    local template_file="$BATS_TEST_TMPDIR/helper-json-control-whitespace.hbs"

    skip_if_no_json
    for helper_name in jsonVerticalTab jsonFormFeed; do
        printf '{{%s}}' "$helper_name" > "$template_file"
        run "$HANDLEBARSC" --execute --no-newline \
            --helper-json "$helper_name=$TEST_DIR/helper_executable.sh" \
            "$template_file"

        assert_failure
        assert_output --partial "invalid JSON response"
    done
}

@test "--helper-json rejects malformed Unicode" {
    local helper_name
    local template_file="$BATS_TEST_TMPDIR/helper-json-unicode.hbs"

    skip_if_no_json
    for helper_name in jsonInvalidUtf8 jsonUnpairedSurrogate; do
        printf '{{%s}}' "$helper_name" > "$template_file"
        run "$HANDLEBARSC" --execute --no-newline \
            --helper-json "$helper_name=$TEST_DIR/helper_executable.sh" \
            "$template_file"

        assert_failure
        assert_output --partial "invalid JSON response"
    done
}

@test "--helper-json accepts valid UTF-8 and surrogate pairs" {
    local template_file="$BATS_TEST_TMPDIR/helper-json-valid-unicode.hbs"

    skip_if_no_json
    printf '%s' '{{jsonUnicode}}' > "$template_file"
    run "$HANDLEBARSC" --execute --no-newline \
        --helper-json "jsonUnicode=$TEST_DIR/helper_executable.sh" \
        "$template_file"

    assert_success
    assert_output "café 😀"
}

@test "array-each" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/array-each.json" "$BENCH_DIR/templates/array-each.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/array-each.expected")"
}

@test "array-each (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/array-each.json" --flags compat "$BENCH_DIR/templates/array-each.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/array-each.expected")"
}

@test "complex" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/complex.json" "$BENCH_DIR/templates/complex.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/complex.expected")"
}

@test "complex (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/complex.json" --flags compat "$BENCH_DIR/templates/complex.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/complex.expected")"
}

@test "data" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/data.json" "$BENCH_DIR/templates/data.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/data.expected")"
}

@test "depth-1" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/depth-1.json" "$BENCH_DIR/templates/depth-1.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/depth-1.expected")"
}

@test "depth-1 (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/depth-1.json" --flags compat "$BENCH_DIR/templates/depth-1.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/depth-1.expected")"
}

@test "depth-2" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/depth-2.json" "$BENCH_DIR/templates/depth-2.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/depth-2.expected")"
}

@test "depth-2 (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/depth-2.json" --flags compat "$BENCH_DIR/templates/depth-2.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/depth-2.expected")"
}

@test "object-mustache" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/object-mustache.json" "$BENCH_DIR/templates/object-mustache.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/object-mustache.expected")"
}

@test "object" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/object.json" "$BENCH_DIR/templates/object.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/object.expected")"
}

@test "object (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/object.json" --flags compat "$BENCH_DIR/templates/object.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/object.expected")"
}

@test "partial" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/partial.json" "${PARTIAL_FLAGS[@]}" "$BENCH_DIR/templates/partial.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/partial.expected")"
}

@test "partial (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/partial.json" "${PARTIAL_FLAGS[@]}" --flags compat "$BENCH_DIR/templates/partial.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/partial.expected")"
}

@test "partial-recursion" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/partial-recursion.json" "${PARTIAL_FLAGS[@]}" "$BENCH_DIR/templates/partial-recursion.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/partial-recursion.expected")"
}

@test "partial-recursion (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/partial-recursion.json" "${PARTIAL_FLAGS[@]}" --partial-ext .mustache --flags compat "$BENCH_DIR/templates/partial-recursion.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/partial-recursion.expected")"
}

@test "paths" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/paths.json" "$BENCH_DIR/templates/paths.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/paths.expected")"
}

@test "paths (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/paths.json" --flags compat "$BENCH_DIR/templates/paths.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/paths.expected")"
}

@test "string" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/string.json" "$BENCH_DIR/templates/string.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/string.expected")"
}

@test "string (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/string.json" --flags compat "$BENCH_DIR/templates/string.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/string.expected")"
}

@test "variables" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/variables.json" "$BENCH_DIR/templates/variables.handlebars"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/variables.expected")"
}

@test "variables (compat)" {
    skip_if_no_json
    run "$HANDLEBARSC" --data "$BENCH_DIR/templates/variables.json" --flags compat "$BENCH_DIR/templates/variables.mustache"
    assert_success
    assert_output "$(cat "$BENCH_DIR/templates/variables.expected")"
}

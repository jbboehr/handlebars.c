#!/usr/bin/env bats

if [ -z "$HANDLEBARSC" ]; then
    HANDLEBARSC=./bin/handlebarsc
fi

if [ -z "$TEST_DIR" ]; then
    TEST_DIR=./tests
fi

if [ -z "$BENCH_DIR" ]; then
    BENCH_DIR=./bench
fi

TEMPLATE=$TEST_DIR/fixture1.hbs
PARTIAL_FLAGS="--partial-loader --partial-path $BENCH_DIR/partials --partial-ext .handlebars"

if ! $HANDLEBARSC --debuginfo 2>&1 | grep -i 'JSON support: enabled' >/dev/null; then
    HAVE_JSON=true
else
    HAVE_JSON=false
fi
if ! $HANDLEBARSC --debuginfo 2>&1 | grep -i 'YAML support: enabled' >/dev/null; then
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

load "../vendor/bats-support/output"
load "../vendor/bats-support/error"
load "../vendor/bats-support/lang"
load "../vendor/bats-assert/assert"

@test "no arguments produces help" {
    run $HANDLEBARSC
    assert_failure
    assert_output --partial "Usage: handlebarsc"
}

@test "invalid option" {
    run $HANDLEBARSC --invalid
    assert_failure
    assert_output --partial "unrecognized option"
}

@test "--help" {
    run $HANDLEBARSC --help
    assert_success
    assert_output --partial "Usage: handlebarsc"
    assert_output --partial "Example: handlebarsc"
}

@test "--version" {
    run $HANDLEBARSC --version
    assert_success
    assert_output --partial "handlebarsc v"
    assert_output --partial "Affero"
}

@test "--debuginfo" {
    run $HANDLEBARSC --debuginfo
    assert_success
    assert_output --partial "JSON support"
    assert_output --partial "YAML support"
    assert_output --partial "XXHash version"
}

@test "--lex" {
    run $HANDLEBARSC --lex $TEMPLATE
    assert_output --partial "OPEN "
}

@test "--lex (invalid file)" {
    run $HANDLEBARSC --lex nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--lex (no file)" {
    run $HANDLEBARSC --lex
    assert_failure
    assert_output --partial "No input file"
}

@test "--lex (empty file)" {
    local empty_file=$(mktemp)
    run $HANDLEBARSC --lex $empty_file
    rm $empty_file
    assert_failure
    assert_output --partial "Failed to read file"
}

@test "--lex --template <TEMPLATE>" {
    run $HANDLEBARSC --lex --template $TEST_DIR/fixture1.hbs
    assert_success
    assert_output --partial "OPEN "
}

@test "--lex -" {
    run bash -c "cat $TEMPLATE | $HANDLEBARSC --lex -"
    assert_success
    assert_output --partial "OPEN "
}

@test "--parse" {
    run $HANDLEBARSC --parse $TEMPLATE
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--parse (invalid file)" {
    run $HANDLEBARSC --parse nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--parse (no file)" {
    run $HANDLEBARSC --parse
    assert_failure
    assert_output --partial "No input file"
}

@test "--parse (parse error)" {
    run $HANDLEBARSC --parse $TEST_DIR/fixture2.hbs
    assert_failure
    assert_output --partial "syntax error"
}

@test "--parse -" {
    run bash -c "cat $TEMPLATE | $HANDLEBARSC --parse -"
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--parse --template <TEMPLATE>" {
    run bash -c "$HANDLEBARSC --parse --template $TEMPLATE"
    assert_success
    assert_output --partial "PATH:foo"
}

@test "--compile" {
    run $HANDLEBARSC --compile $TEMPLATE
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile (invalid file)" {
    run $HANDLEBARSC --compile nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--compile (no file)" {
    run $HANDLEBARSC --compile
    assert_failure
    assert_output --partial "No input file"
}

@test "--compile (compile error)" {
    run $HANDLEBARSC --compile $TEST_DIR/fixture3.hbs
    assert_failure
    assert_output --partial "Unsupported number of partial arguments"
}

@test "--compile -" {
    run bash -c "cat $TEMPLATE | $HANDLEBARSC --compile -"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile --template <TEMPLATE>" {
    run $HANDLEBARSC --compile $TEMPLATE
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--compile --flags no_escape" {
    run $HANDLEBARSC --compile --flags no_escape $TEMPLATE
    assert_output --partial "append"
    refute_output --partial "appendEscaped"
}

@test "--module" {
    run $HANDLEBARSC --module $TEMPLATE
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module (invalid file)" {
    run $HANDLEBARSC --module nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--module (no file)" {
    run $HANDLEBARSC --module
    assert_failure
    assert_output --partial "No input file"
}

@test "--module (compile error)" {
    run $HANDLEBARSC --module $TEST_DIR/fixture3.hbs
    assert_failure
    assert_output --partial "Unsupported number of partial arguments"
}

@test "--module -" {
    run bash -c "cat $TEMPLATE | $HANDLEBARSC --module -"
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module --template <TEMPLATE>" {
    run $HANDLEBARSC --module $TEMPLATE
    assert_success
    assert_output --partial "appendEscaped"
}

@test "--module --flags no_escape" {
    run $HANDLEBARSC --module --flags no_escape $TEMPLATE
    assert_output --partial "append"
    refute_output --partial "appendEscaped"
}

@test "--execute" {
    skip_if_no_json
    run $HANDLEBARSC --execute --data $TEST_DIR/fixture1.json $TEMPLATE
    assert_success
    assert_output "|bar|"
}

@test "--execute (invalid file)" {
    run $HANDLEBARSC --execute nonexist
    assert_failure
    assert_output --partial "Failed to open file"
}

@test "--execute (no file)" {
    run $HANDLEBARSC --execute
    assert_failure
    assert_output --partial "No input file"
}

@test "--execute (empty file)" {
    local empty_file=$(mktemp)
    run $HANDLEBARSC --execute $empty_file
    rm $empty_file
    assert_failure
    assert_output --partial "Failed to read file"
}

@test "--execute (is default mode)" {
    skip_if_no_json
    run $HANDLEBARSC --data $TEST_DIR/fixture1.json $TEMPLATE
    assert_success
    assert_output "|bar|"
}

@test "--execute (yaml)" {
    skip_if_no_yaml
    run $HANDLEBARSC --execute --data $TEST_DIR/fixture1.yaml $TEMPLATE
    assert_success
    assert_output "|bar|"
}

@test "--execute --template <TEMPLATE>" {
    skip_if_no_json
    run $HANDLEBARSC --execute --data $TEST_DIR/fixture1.json --template $TEMPLATE
    assert_success
    assert_output "|bar|"
}

@test "--execute --flags strict (runtime error)" {
    run $HANDLEBARSC --execute --flags strict $TEMPLATE
    assert_failure
    assert_output --partial '"foo" not defined in object'
}

@test "--execute -n" {
    skip_if_no_json
    # wc on OSX outputs leading whitespace
    result1=$($HANDLEBARSC --execute --data $TEST_DIR/fixture1.yaml $TEMPLATE | wc -l | sed 's/ *//g')
    assert_equal "$result1" "1"
    result2=$($HANDLEBARSC --execute -n --data $TEST_DIR/fixture1.yaml $TEMPLATE | wc -l | sed 's/ *//g')
    assert_equal "$result2" "0"
    result3=$($HANDLEBARSC --execute --no-newline --data $TEST_DIR/fixture1.yaml $TEMPLATE | wc -l | sed 's/ *//g')
    assert_equal "$result3" "0"
}

@test "--execute --pool-size 0" {
    # not really any way to check if this works, just checking if nothing is broken when specified
    skip_if_no_yaml
    run $HANDLEBARSC --execute --pool-size 0 --data $TEST_DIR/fixture1.json $TEST_DIR/fixture1.hbs
    assert_success
    assert_output "|bar|"
}

@test "array-each" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/array-each.json $BENCH_DIR/templates/array-each.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/array-each.expected`"
}

@test "array-each (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/array-each.json --flags compat $BENCH_DIR/templates/array-each.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/array-each.expected`"
}

@test "complex" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/complex.json $BENCH_DIR/templates/complex.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/complex.expected`"
}

@test "complex (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/complex.json --flags compat $BENCH_DIR/templates/complex.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/complex.expected`"
}

@test "data" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/data.json $BENCH_DIR/templates/data.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/data.expected`"
}

@test "depth-1" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/depth-1.json $BENCH_DIR/templates/depth-1.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/depth-1.expected`"
}

@test "depth-1 (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/depth-1.json --flags compat $BENCH_DIR/templates/depth-1.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/depth-1.expected`"
}

@test "depth-2" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/depth-2.json $BENCH_DIR/templates/depth-2.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/depth-2.expected`"
}

@test "depth-2 (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/depth-2.json --flags compat $BENCH_DIR/templates/depth-2.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/depth-2.expected`"
}

@test "object-mustache" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/object-mustache.json $BENCH_DIR/templates/object-mustache.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/object-mustache.expected`"
}

@test "object" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/object.json $BENCH_DIR/templates/object.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/object.expected`"
}

@test "object (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/object.json --flags compat $BENCH_DIR/templates/object.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/object.expected`"
}

@test "partial" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/partial.json $PARTIAL_FLAGS $BENCH_DIR/templates/partial.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/partial.expected`"
}

@test "partial (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/partial.json $PARTIAL_FLAGS --flags compat $BENCH_DIR/templates/partial.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/partial.expected`"
}

@test "partial-recursion" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/partial-recursion.json $PARTIAL_FLAGS $BENCH_DIR/templates/partial-recursion.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/partial-recursion.expected`"
}

@test "partial-recursion (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/partial-recursion.json $PARTIAL_FLAGS --partial-ext .mustache --flags compat $BENCH_DIR/templates/partial-recursion.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/partial-recursion.expected`"
}

@test "paths" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/paths.json $BENCH_DIR/templates/paths.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/paths.expected`"
}

@test "paths (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/paths.json --flags compat $BENCH_DIR/templates/paths.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/paths.expected`"
}

@test "string" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/string.json $BENCH_DIR/templates/string.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/string.expected`"
}

@test "string (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/string.json --flags compat $BENCH_DIR/templates/string.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/string.expected`"
}

@test "variables" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/variables.json $BENCH_DIR/templates/variables.handlebars
    assert_success
    assert_output "`cat $BENCH_DIR/templates/variables.expected`"
}

@test "variables (compat)" {
    skip_if_no_json
    run $HANDLEBARSC --data $BENCH_DIR/templates/variables.json --flags compat $BENCH_DIR/templates/variables.mustache
    assert_success
    assert_output "`cat $BENCH_DIR/templates/variables.expected`"
}

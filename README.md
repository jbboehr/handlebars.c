# handlebars.c

[![GitHub Build Status](https://github.com/jbboehr/handlebars.c/workflows/ci/badge.svg)](https://github.com/jbboehr/handlebars.c/actions?query=workflow%3Aci)
[![Coverage Status](https://coveralls.io/repos/jbboehr/handlebars.c/badge.svg?branch=master&service=github)](https://coveralls.io/github/jbboehr/handlebars.c?branch=master)
[![License](https://img.shields.io/badge/license-LGPLv2.1-brightgreen.svg)](LICENSE.md)

C implementation of [handlebars.js](https://github.com/wycats/handlebars.js/),
developed in conjunction with [php-handlebars](https://github.com/jbboehr/php-handlebars)
and [handlebars.php](https://github.com/jbboehr/handlebars.php).

The opcode compiler is fully featured. The VM supports inline partials, but
does not implement general decorator execution.

## Installation

### Nix / NixOS

```bash
nix-env -i -f https://github.com/jbboehr/handlebars.c/archive/v1.0.0.tar.gz
```

or, in a `.nix` file:

```nix
(import <nixpkgs> {}).callPackage (import (fetchTarball {
  url = https://github.com/jbboehr/handlebars.c/archive/v1.0.0.tar.gz;
  sha256 = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
})) {}
```

or, to run as a flake:

```bash
nix run github:jbboehr/handlebars.c
```

### Alpine Linux

```bash
apk add handlebars handlebars-dev handlebars-utils
```

### Debian / Ubuntu

```bash
# Install dependencies
sudo apt-get install autoconf automake bison flex gcc libjson-c-dev liblmdb-dev \
                     libtalloc-dev libyaml-dev libtool m4 make pkg-config

# Install testing dependencies
sudo apt-get install check gdb lcov libpcre2-dev bats

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./configure --disable-Werror --disable-testing-exports
make all check
sudo make install
sudo ldconfig
```

### OSX via Homebrew

```bash
# Install dependencies
brew install autoconf automake bison flex gcc json-c libtool libyaml lmdb pkg-config talloc

# Install testing dependencies
brew install check lcov pcre2 bats

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./configure --disable-Werror --disable-testing-exports
make all check
make install
```

### CLI via Docker

```bash
# via Docker Hub
docker pull jbboehr/handlebars.c:latest

# via GitHub Packages
docker pull docker.pkg.github.com/jbboehr/handlebars.c/handlebarsc:latest
```

### Experimental no-refcount mode

Configuring with `--disable-refcounting` replaces per-object reference counting
with bounded-context arena ownership. This mode does not reclaim aliased or
cyclic values individually and is intended for experimentation and build
coverage, not as a drop-in ownership model.

Use a dedicated talloc context for each VM or request and destroy that context
after the work completes. Cache and partial-loader allocations follow their
cache or loader context instead of a VM context, so applications using this
mode must also bound those contexts rather than keep them for the lifetime of
the process.

### C API error handling

A VM is stateful and must not be used concurrently. Give each concurrent
request or worker its own context and VM. A parser, compiler, and VM may share
that context when used sequentially. Objects constructed directly from the
same context also share its mutable error state.

The core construction, parse, compile, serialize, render, cache,
partial-loader, and optional JSON/YAML conversion operations have additive
`_try` entrypoints for applications that do not want library errors to
`longjmp` into consumer code.
These functions return an `enum handlebars_error_type` and leave the error
message and location on the associated context:

```c
struct handlebars_string * output = NULL;
enum handlebars_error_type error = handlebars_vm_execute_try(
    vm,
    module,
    input,
    &output
);

if (error != HANDLEBARS_SUCCESS) {
    fprintf(stderr, "render failed: %s\n", handlebars_error_msg(context));
}
```

Pointer-producing `_try` functions set their output pointer to `NULL` on
failure. JSON and YAML conversion `_try` functions replace the supplied value
only on success, preserving its previous contents on failure. Partial-loader
construction and lookup follow the same value-preservation rule; VM rendering
also transfers built-in partial-loader errors to the VM context when the loader
uses a separate context. Cache lookup
reports a miss as success with a `NULL` module; cache statistics and garbage
collection publish their output values only on success.

Each `_try` call clears stale error state before it begins and restores any
previous internal jump target before returning. Existing entrypoints retain
their original behavior and ownership rules. Helper callbacks still use the
existing callback ABI internally; errors they raise during rendering are
caught by `handlebars_vm_execute_try` before it returns.

The `_try` APIs are workflow boundaries rather than replacements for every
allocation-bearing utility. Custom helpers do not need a separate
explicit-status callback ABI when invoked while rendering: they may use
`handlebars_throw`, and `handlebars_vm_execute_try` reports the resulting
error. Direct calls to helper callbacks, built-in helpers,
`handlebars_vm_call_helper_str`, lexer functions, container and string
mutators, or diagnostic printers retain the legacy error behavior and may
`longjmp` through an installed context error boundary.

## Usage

```console
$ handlebarsc --help
Usage: handlebarsc [OPTIONS]
Example: handlebarsc -t foo.hbs -D bar.json

Mode options:
  -h, --help            Show this message
  -V, --version         Print the version
  --execute             Execute the specified template (default)
  --lex                 Lex the specified template into tokens
  --parse               Parse the specified template into an AST
  --compile             Compile the specified template into opcodes

Input options:
  -t, --template=FILE   The template to operate on
  -D, --data=FILE       The input data file. Supports JSON and YAML.

Behavior options:
  -n, --no-newline      Do not print a newline after execution
  --flags=FLAGS         The flags to pass to the compiler separated by commas. One or more of:
                        compat, known_helpers_only, string_params, track_ids, no_escape,
                        ignore_standalone, alternate_decorators, strict, assume_objects,
                        mustache_style_lambdas
  --no-convert-input    Do not convert data to native types (use JSON wrapper)
  --partial-loader      Specify to enable loading partials dynamically
  --partial-path=DIR    The directory in which to look for partials
  --partial-ext=EXT     The file extension of partials, including the '.'
  --pool-size=SIZE      The size of the memory pool to use, 0 to disable (default 2 MB)
  --run-count=NUM       The number of times to execute (for benchmarking)
  --helper-exec=NAME=COMMAND
                        Register a helper backed by plain executable output
  --helper-json=NAME=COMMAND
                        Register a helper using the structured JSON protocol
  --helper-timeout-ms=MS
                        Per-call timeout; 0 disables it (default 5000)
  --helper-output-limit=BYTES
                        Maximum stdout size; 0 disables it (default 1048576)
  --allow-helper-override
                        Allow later registrations to replace helpers

The partial loader will concat the partial-path, given partial name in the template,
and the partial-extension to resolve the file from which to load the partial.

If a FILE is specified as '-', it will be read from STDIN.
```

### Executable-backed CLI helpers

`handlebarsc` can register a helper name to a separate executable for compile,
module, and execute modes. A command containing `/` is invoked directly; a
bare command is resolved through `PATH`. Commands are not interpreted by a
shell, and each helper invocation starts a new process. For example:

```console
$ printf '%s' '{{greet "world"}}' |
    handlebarsc --execute --no-newline \
      --helper-exec greet=/usr/local/bin/greet -
```

Plain helpers receive primitive positional arguments in `argv`, read
`/dev/null` on stdin, inherit stderr, and return their Handlebars string value
verbatim on stdout. Plain helpers reject block calls, hash arguments,
containers, and callable or opaque values.

JSON helpers receive one newline-terminated request on stdin:

```json
{"protocol":1,"helper":"greet","args":["world"],"hash":{},"scope":null,"data":null}
```

They must return exactly one JSON response on stdout. Unknown response fields
are ignored. A successful response is:

```json
{"protocol":1,"ok":true,"value":"hello, world","safe":false}
```

`value` may be any JSON value. `safe:true` is accepted only for strings and
suppresses normal Handlebars HTML escaping. A helper-reported error is:

```json
{"protocol":1,"ok":false,"error":"explanation"}
```

By default a helper has five seconds to complete and may emit at most 1 MiB.
`--helper-timeout-ms=0` and `--helper-output-limit=0` disable those respective
limits. Timeouts, oversized output, spawn failures, nonzero exits, signals,
and invalid JSON fail the render without publishing partial template output.

Configured helper executables are trusted code and run with `handlebarsc`'s
credentials and environment. Descendant cleanup is best-effort by process
group; processes that deliberately change their session or process group are
not contained and may survive helper completion or termination.

Duplicate registrations and names that collide with built-in helpers are
rejected. `--allow-helper-override` permits them, with the last registration
winning. Helper commands are runtime configuration: compile and module modes
record only the helper name and never execute or embed the command.

Executable-backed helpers currently require POSIX spawn, process-group, poll,
and monotonic-clock support. JSON helpers additionally require json-c. General
block-helper callbacks and persistent helper processes are not supported.

### via Docker

```bash
# via Docker Hub
DOCKER_IMAGE=jbboehr/handlebars.c:latest

# via GitHub Packages
DOCKER_IMAGE=docker.pkg.github.com/jbboehr/handlebars.c/handlebarsc:latest

docker pull ${DOCKER_IMAGE}

# relying on the workdir is probably not stable

docker run \
    --rm \
    -v "$PWD:/srv" \
    ${DOCKER_IMAGE} \
    --data bench/templates/variables.json \
    --template bench/templates/variables.handlebars

cat bench/templates/variables.json | docker run \
    --rm -i
    -v "$PWD:/srv"
    ${DOCKER_IMAGE} \
    --data - \
    --template bench/templates/variables.handlebars
```

## License

The library for this project is licensed under the [LGPLv2.1 or later](LICENSE.md).
The executable and the test suite are licensed under the [AGPLv3.0 or later](LICENSE-AGPL.md).
handlebars.js is licensed under the [MIT license](http://opensource.org/licenses/MIT).

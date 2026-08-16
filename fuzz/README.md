# Fuzzing

The `fuzz_template` target sends arbitrary templates through parsing,
compilation, serialization, and VM execution. The `fuzz_json` target parses
arbitrary JSON, exercises lazy and converted value traversal, and renders the
values through a fixed template. The `fuzz_yaml` target parses arbitrary
length-delimited YAML, traverses the resulting native values, and renders them
through a fixed template. The `fuzz_module` target verifies serialized
modules, reconstructs and normalizes their pointers, prints valid structures,
and executes unmodified valid corpus entries. The `fuzz_lmdb` target stores
arbitrary records and drives a stateful sequence of cache operations, including
valid module insertion, lookup, garbage collection, reset, and reopen. All
targets require Clang with libFuzzer, AddressSanitizer, and
UndefinedBehaviorSanitizer support. `fuzz_json` also requires json-c, and
`fuzz_yaml` requires libyaml, while `fuzz_lmdb` requires LMDB.

Build and run it with Autotools:

```sh
autoreconf -i
CC=clang ./configure --enable-fuzzing --disable-shared
make -j2
mkdir -p fuzz-corpus/template fuzz-corpus/json fuzz-corpus/lmdb fuzz-corpus/module fuzz-corpus/yaml
./fuzz/fuzz_template fuzz-corpus/template fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
./fuzz/fuzz_json fuzz-corpus/json fuzz/corpus/json \
    -dict=fuzz/json.dict -max_len=65536
./fuzz/fuzz_module fuzz-corpus/module fuzz/corpus/module \
    -dict=fuzz/module.dict -max_len=65536
./fuzz/fuzz_lmdb fuzz-corpus/lmdb fuzz/corpus/lmdb fuzz/corpus/module \
    -dict=fuzz/lmdb.dict -max_len=65536
./fuzz/fuzz_yaml fuzz-corpus/yaml fuzz/corpus/yaml \
    -dict=fuzz/yaml.dict -max_len=65536
```

Or with CMake:

```sh
CC=clang cmake -S . -B cmake-build-fuzz \
    -DHANDLEBARS_ENABLE_FUZZING=ON \
    -DHANDLEBARS_ENABLE_TESTS=OFF
cmake --build cmake-build-fuzz -j2
mkdir -p fuzz-corpus/template fuzz-corpus/json fuzz-corpus/lmdb fuzz-corpus/module fuzz-corpus/yaml
./cmake-build-fuzz/fuzz/fuzz_template fuzz-corpus/template fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
./cmake-build-fuzz/fuzz/fuzz_json fuzz-corpus/json fuzz/corpus/json \
    -dict=fuzz/json.dict -max_len=65536
./cmake-build-fuzz/fuzz/fuzz_module fuzz-corpus/module fuzz/corpus/module \
    -dict=fuzz/module.dict -max_len=65536
./cmake-build-fuzz/fuzz/fuzz_lmdb fuzz-corpus/lmdb fuzz/corpus/lmdb \
    fuzz/corpus/module -dict=fuzz/lmdb.dict -max_len=65536
./cmake-build-fuzz/fuzz/fuzz_yaml fuzz-corpus/yaml fuzz/corpus/yaml \
    -dict=fuzz/yaml.dict -max_len=65536
```

The first corpus directory passed to each target is writable and collects
newly discovered inputs; the checked-in seed corpus remains unchanged.
Preserve `fuzz-corpus` between runs to retain coverage gains.

Corpus inputs may begin with four hexadecimal digits and a newline. That
optional header selects compiler flags, masked by
`handlebars_compiler_flag_all`; the remaining bytes are the template. Inputs
without a valid header use no compiler flags.

JSON corpus inputs are passed directly to json-c and have no header.

YAML corpus inputs are passed directly to libyaml and have no header.

Module corpus inputs use the native serialized-module representation for the
build architecture. The harness independently exercises the original envelope
and a canonicalized envelope so mutations can reach structural validation
without having to reproduce the 64-bit module hash.

LMDB corpus inputs are both stored verbatim as a raw cache record and used as a
bounded sequence of cache operations. Passing the module corpus as an
additional seed directory lets the cache lookup and garbage-collection paths
exercise valid serialized records as well as malformed ones.

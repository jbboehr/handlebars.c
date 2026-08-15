# Fuzzing

The `fuzz_template` target sends arbitrary templates through parsing,
compilation, serialization, and VM execution. The `fuzz_json` target parses
arbitrary JSON, exercises lazy and converted value traversal, and renders the
values through a fixed template. Both require Clang with libFuzzer,
AddressSanitizer, and UndefinedBehaviorSanitizer support. `fuzz_json` also
requires json-c.

Build and run it with Autotools:

```sh
autoreconf -i
CC=clang ./configure --enable-fuzzing --disable-shared
make -j2
mkdir -p fuzz-corpus/template fuzz-corpus/json
./fuzz/fuzz_template fuzz-corpus/template fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
./fuzz/fuzz_json fuzz-corpus/json fuzz/corpus/json \
    -dict=fuzz/json.dict -max_len=65536
```

Or with CMake:

```sh
CC=clang cmake -S . -B cmake-build-fuzz \
    -DHANDLEBARS_ENABLE_FUZZING=ON \
    -DHANDLEBARS_ENABLE_TESTS=OFF
cmake --build cmake-build-fuzz -j2
mkdir -p fuzz-corpus/template fuzz-corpus/json
./cmake-build-fuzz/fuzz/fuzz_template fuzz-corpus/template fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
./cmake-build-fuzz/fuzz/fuzz_json fuzz-corpus/json fuzz/corpus/json \
    -dict=fuzz/json.dict -max_len=65536
```

The first corpus directory passed to each target is writable and collects
newly discovered inputs; the checked-in seed corpus remains unchanged.
Preserve `fuzz-corpus` between runs to retain coverage gains.

Corpus inputs may begin with four hexadecimal digits and a newline. That
optional header selects compiler flags, masked by
`handlebars_compiler_flag_all`; the remaining bytes are the template. Inputs
without a valid header use no compiler flags.

JSON corpus inputs are passed directly to json-c and have no header.

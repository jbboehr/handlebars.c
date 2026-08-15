# Fuzzing

The `fuzz_template` target sends arbitrary templates through parsing,
compilation, serialization, and VM execution. It requires Clang with
libFuzzer, AddressSanitizer, and UndefinedBehaviorSanitizer support.

Build and run it with Autotools:

```sh
autoreconf -i
CC=clang ./configure --enable-fuzzing --disable-shared
make -j2
mkdir -p fuzz-corpus
./fuzz/fuzz_template fuzz-corpus fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
```

Or with CMake:

```sh
CC=clang cmake -S . -B cmake-build-fuzz \
    -DHANDLEBARS_ENABLE_FUZZING=ON \
    -DHANDLEBARS_ENABLE_TESTS=OFF
cmake --build cmake-build-fuzz -j2
mkdir -p fuzz-corpus
./cmake-build-fuzz/fuzz/fuzz_template fuzz-corpus fuzz/corpus/template \
    -dict=fuzz/handlebars.dict -max_len=65536
```

The first corpus directory is writable and collects newly discovered inputs;
the checked-in seed corpus remains unchanged. Preserve `fuzz-corpus` between
runs to retain coverage gains.

Corpus inputs may begin with four hexadecimal digits and a newline. That
optional header selects compiler flags, masked by
`handlebars_compiler_flag_all`; the remaining bytes are the template. Inputs
without a valid header use no compiler flags.

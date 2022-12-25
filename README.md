# handlebars.c

[![GitHub Build Status](https://github.com/jbboehr/handlebars.c/workflows/ci/badge.svg)](https://github.com/jbboehr/handlebars.c/actions?query=workflow%3Aci)
[![Coverage Status](https://coveralls.io/repos/jbboehr/handlebars.c/badge.svg?branch=master&service=github)](https://coveralls.io/github/jbboehr/handlebars.c?branch=master)
[![License](https://img.shields.io/badge/license-LGPLv2.1-brightgreen.svg)](LICENSE.md)

C implementation of [handlebars.js](https://github.com/wycats/handlebars.js/),
developed in conjunction with [php-handlebars](https://github.com/jbboehr/php-handlebars)
and [handlebars.php](https://github.com/jbboehr/handlebars.php).

The opcode compiler is fully featured, however the VM currently does not implement decorators, and therefore inline
partials.

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
sudo apt-get install check gdb lcov libpcre3-dev bats

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
brew install check lcov pcre bats

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

## Usage

```
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

The partial loader will concat the partial-path, given partial name in the template,
and the partial-extension to resolve the file from which to load the partial.

If a FILE is specified as '-', it will be read from STDIN.
```

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

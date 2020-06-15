# handlebars.c

[![Build Status](https://travis-ci.org/jbboehr/handlebars.c.svg?branch=master)](https://travis-ci.org/jbboehr/handlebars.c)
[![Linux Build Status](https://github.com/jbboehr/handlebars.c/workflows/linux/badge.svg)](https://github.com/jbboehr/handlebars.c/actions?query=workflow%3Alinux)
[![OSX Build Status](https://github.com/jbboehr/handlebars.c/workflows/osx/badge.svg)](https://github.com/jbboehr/handlebars.c/actions?query=workflow%3Aosx)
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
nix-env -i -f https://github.com/jbboehr/handlebars.c/archive/v0.7.2.tar.gz
```

or, in a `.nix` file:

```nix
(import <nixpkgs> {}).callPackage (import (fetchTarball {
  url = https://github.com/jbboehr/handlebars.c/archive/v0.7.2.tar.gz;
  sha256 = "1rszprra8pavsw7aq7ixdn3jd00zy3hymmh2z4wcqc9lrw3h6hxb";
})) {}
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

# Install doc dependencies
sudo apt-get install doxygen

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./bootstrap && ./configure --disable-Werror --disable-testing-exports && make && sudo make install && sudo ldconfig
```

### OSX via Homebrew

```bash
# Install dependencies
brew install autoconf automake bison flex gcc json-c libtool libyaml lmdb pkg-config talloc

# Install testing dependencies
brew install check lcov pcre bats

# Install doc dependencies
brew install doxygen

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./bootstrap && ./configure --disable-Werror --disable-testing-exports && make install
```

## License

The library for this project is licensed under the [LGPLv2.1 or later](LICENSE.md).
The executable and the test suite are licensed under the [AGPLv3.0 or later](LICENSE-AGPL.md).
handlebars.js is licensed under the [MIT license](http://opensource.org/licenses/MIT).


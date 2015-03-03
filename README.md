# handlebars.c

[![Build Status](https://travis-ci.org/jbboehr/handlebars.c.svg?branch=master)](https://travis-ci.org/jbboehr/handlebars.c)

C implementation of the [handlebars.js](https://github.com/wycats/handlebars.js/)
lexer, parser, and compiler. Use with [php-handlebars](https://github.com/jbboehr/php-handlebars) and [handlebars.php](https://github.com/jbboehr/handlebars.php).


## Requirements

### Ubuntu

```bash
# Install dependencies
sudo apt-get install autoconf automake bison flex gawk gcc git-core \
                     libjson0-dev libtalloc-dev libtool m4 make pkg-config \
                     uthash-dev

# Install testing dependencies
sudo apt-get install check gdb lcov

# Install doc dependencies
sudo apt-get install doxygen
```

### OS X

```bash
# Install dependencies
brew install autoconf automake bison flex gcc json-c libtool pkg-config talloc
brew install https://raw.githubusercontent.com/caktux/homebrew-bitcoin/master/uthash.rb

# Install testing dependencies
sudo brew install check lcov

# Install doc dependencies
sudo brew install doxygen
```


## Installation

```bash
# Clone
git clone git://github.com/jbboehr/handlebars.c.git
cd handlebars.c
git submodule update --init --recursive

# Build
./bootstrap
./configure
make
make test
sudo make install
sudo ldconfig
```


## License

This project is licensed under the [LGPLv3](http://www.gnu.org/licenses/lgpl-3.0.txt).
handlebars.js is licensed under the [MIT license](http://opensource.org/licenses/MIT).

# handlebars.c

[![Build Status](https://travis-ci.org/jbboehr/handlebars.c.svg?branch=master)](https://travis-ci.org/jbboehr/handlebars.c)
[![Coverage Status](https://coveralls.io/repos/jbboehr/handlebars.c/badge.svg?branch=master&service=github)](https://coveralls.io/github/jbboehr/handlebars.c?branch=master)
[![License](https://img.shields.io/badge/license-LGPLv2.1-brightgreen.svg)](LICENSE.md)

C implementation of [handlebars.js](https://github.com/wycats/handlebars.js/),
developed in conjunction with [php-handlebars](https://github.com/jbboehr/php-handlebars)
and [handlebars.php](https://github.com/jbboehr/handlebars.php).

The opcode compiler is fully featured, however the VM currently does not implement decorators, and therefore inline 
partials.


## Installation


### PPA

```bash
sudo apt-add-repository ppa:jbboehr/handlebars
sudo apt-get update
sudo apt-get install handlebars libhandlebars-dev
```


### Source

Note: >= gcc 4.9 is required for `returns_nonnull`

#### Ubuntu 

```bash
# Install dependencies
sudo apt-get install autoconf automake bison flex gcc libjson-c-dev liblmdb-dev \
                     libtalloc-dev libyaml-dev libtool m4 make pkg-config

# Install testing dependencies
sudo apt-get install check gdb lcov libpcre3-dev

# Install doc dependencies
sudo apt-get install doxygen

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./bootstrap && ./configure && make && sudo make install && sudo ldconfig
```


#### OS X

```bash
# Install dependencies
brew install autoconf automake bison flex gcc json-c libtool libyaml lmdb pkg-config talloc

# Install testing dependencies
brew install check lcov pcre

# Install doc dependencies
brew install doxygen

# Compile
git clone https://github.com/jbboehr/handlebars.c.git --recursive
cd handlebars.c
./bootstrap && ./configure && make install
```


## License

This project is licensed under the [LGPLv2.1 or later](LICENSE.md).
handlebars.js is licensed under the [MIT license](http://opensource.org/licenses/MIT).

# handlebars.c

C implementation of the [handlebars.js](https://github.com/wycats/handlebars.js/)
lexer and parser using flex and bison.


## Requirements

### Ubuntu

```bash
# Install dependencies
sudo apt-get install autoconf automake bison flex gawk gcc git-core \
                     libjson0-devlibtalloc-dev libtool m4 make pkg-config \
                     uthash-dev

# Install testing dependencies
sudo apt-get install check gdb lcov

# Install doc dependencies
sudo apt-get install doxygen
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

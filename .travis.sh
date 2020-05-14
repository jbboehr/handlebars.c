#!/usr/bin/env bash

set -e -o pipefail

export CC="$MYCC"
export PREFIX="$HOME/build"
export PATH="$PREFIX/bin:$PATH"
export CFLAGS="-I$PREFIX/include -I$PREFIX/include/json-c"
export LDFLAGS="-L$PREFIX/lib $LDFLAGS"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:/usr/lib/$ARCH-linux-gnu/pkgconfig"
export LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
export SUDO="sudo"

mkdir -p $PREFIX/include $PREFIX/include/json-c $PREFIX/lib/pkgconfig

if [ "$ARCH" = "i386" ]; then
	export CFLAGS="$CFLAGS -m32"
fi

if [ "$HARDENING" = "true" ]; then
	export CFLAGS="$CFLAGS -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -Wp,-D_GLIBCXX_ASSERTIONS -fexceptions -fstack-protector-strong -grecord-gcc-switches"
	export CFLAGS="$CFLAGS -specs=`pwd`/tests/redhat-hardened-cc1 -specs=`pwd`/tests/redhat-hardened-ld"
	export CFLAGS="$CFLAGS -fasynchronous-unwind-tables -fstack-clash-protection -fPIC -DPIC"
	# this is not supported on travis: -fcf-protection
fi

function install_apt_packages() (
	set -e -o pipefail

	# we commit the generated files for these now:
	# apt-get install -y bison flex gperf re2c
	apt_packages_to_install="$MYCC automake pkg-config gcc-multilib cmake check:$ARCH liblmdb-dev:$ARCH libpcre3-dev:$ARCH libtalloc-dev:$ARCH libyaml-dev:$ARCH libsubunit-dev:$ARCH"
	if [ "$COVERAGE" = "true" ]; then
		apt_packages_to_install="$apt_packages_to_install lcov"
	fi
	$SUDO apt-add-repository -y ppa:ubuntu-toolchain-r/test
	$SUDO apt-get update -y
	$SUDO apt-get purge -y bison flex gperf re2c
	$SUDO apt-get install -y ${apt_packages_to_install}
)

function install_json_c() (
	set -e -o pipefail

	git clone -b json-c-0.14-20200419 https://github.com/json-c/json-c.git
	mkdir json-c-build
	cd json-c-build
	cmake -DCMAKE_INSTALL_PREFIX=$PREFIX ../json-c
	make install
)

function install_coveralls_lcov() (
	set -e -o pipefail

	gem install coveralls-lcov
)

function before_install() (
	set -e -o pipefail

	install_apt_packages
	install_coveralls_lcov
	# build json-c from source to work around: https://bugs.launchpad.net/ubuntu/+source/json-c/+bug/1878738
	# apt-get install -y libjson-c-dev:$ARCH
	install_json_c
)

function install() (
	set -e -o pipefail

	export CFLAGS="$CFLAGS -g -O2"

	if [ "$COVERAGE" = "true" ]; then
		export CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
		export LDFLAGS="$LDFLAGS --coverage"
	fi

	# json-c undeprecated json_object_object_get, but the version in xenial
	# is too old, so let's silence deprecated warnings. le sigh.
	export CFLAGS="$CFLAGS -Wno-deprecated-declarations -Wno-error=deprecated-declarations"

	# does gcc-4.9 not support "#pragma GCC diagnostic ignored"?
	if [ "$CC" = "gcc-4.9" ]; then
		export CFLAGS="$CFLAGS -Wno-shadow -Wno-error=shadow -Wno-pointer-sign -Wno-error=pointer-sign -Wno-switch-default -Wno-error=switch-default"
		export CFLAGS="$CFLAGS -Wno-unused-function -Wno-error=unused-function -Wno-inline -Wno-error=inline"
		# these are apparently just broken?
		export CFLAGS="$CFLAGS -Wno-missing-braces -Wno-error=missing-braces"
	fi

	./bootstrap

	trap "cat config.log" ERR
	./configure --build="$ARCH" --prefix="$PREFIX" --enable-handlebars-memory
	trap - ERR

	make clean all
)

function before_script() (
	set -e -o pipefail

	if [ "$COVERAGE" = "true" ]; then
		lcov --directory . --zerocounters
		lcov --directory . --capture --compat-libtool --initial --output-file coverage.info
	fi
)

function script() (
	set -e -o pipefail

	make check install
	./bench/run.sh
)

function after_success() (
	set -e -o pipefail

	if [ "$COVERAGE" = "true" ]; then
		lcov --no-checksum --directory . --capture --compat-libtool --output-file coverage.info
		lcov --remove coverage.info "/usr*" \
			--remove coverage.info "*/tests/*" \
			--remove coverage.info "$HOME/build/json-c/*" \
			--remove coverage.info "handlebars.tab.c" \
			--remove coverage.info "handlebars.lex.c" \
			--remove coverage.info "handlebars_scanners.c" \
			--compat-libtool \
			--output-file coverage.info
		coveralls-lcov coverage.info
	fi
)

function after_failure() (
	set -e -o pipefail

	if [ "$COVERAGE" = "true" ]; then
		for i in `find tests -name "*.log" 2>/dev/null`; do
			echo "-- START ${i}";
			cat $i;
			echo "-- END";
		done
	fi
	if [ -f tests/test-suite.log ]; then
			echo "-- START test-suite.log";
			cat tests/test-suite.log
			echo "-- END";
	fi
)

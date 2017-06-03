#!/usr/bin/env bash

set -ex

export CC="$MYCC"
export PREFIX="$HOME/build"
export PATH="$PREFIX/bin:$PATH"
export CFLAGS="-g -O3 -L$PREFIX/lib $CFLAGS"
export CPPFLAGS="-I$PREFIX/include"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:/usr/lib/$ARCH-linux-gnu/pkgconfig"

if [ "$COVERAGE" = "true" ]; then
	export CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	export LDFLAGS="$LDFLAGS --coverage"
fi

case $1 in
	before_install)
		apt-add-repository -y ppa:ubuntu-toolchain-r/test
		apt-add-repository -y ppa:jbboehr/build-deps
		apt-add-repository -y ppa:jbboehr/handlebars
		apt-get update -y
		apt-get install -y automake bison flex gperf pkg-config re2c gcc-multilib
		# install arch-specific packages
		apt-get install -y check:$ARCH libjson-c-dev:$ARCH liblmdb-dev:$ARCH libpcre3-dev:$ARCH libtalloc-dev:$ARCH libyaml-dev:$ARCH libsubunit-dev:$ARCH
		if [ "$COVERAGE" = "true" ]; then
			apt-get install -y lcov
			gem install coveralls-lcov
		fi
		;;

	install)
		./bootstrap
		./configure --build="$ARCH" --prefix="$PREFIX" --enable-handlebars-memory
		make clean all
		;;

	before_script)
		if [ "$COVERAGE" = "true" ]; then
			lcov --directory . --zerocounters
			lcov --directory . --capture --compat-libtool --initial --output-file coverage.info
		fi
		;;

	script)
		make check install
		;;

	after_success)
		if [ "$COVERAGE" = "true" ]; then
			lcov --no-checksum --directory . --capture --compat-libtool --output-file coverage.info
			lcov --remove coverage.info "/usr*" --remove coverage.info "*/tests/*" --remove coverage.info "handlebars.tab.c" --remove coverage.info "handlebars.lex.c" --remove coverage.info "handlebars_scanners.c" --compat-libtool --output-file coverage.info
			coveralls-lcov coverage.info
		fi
		;;

	after_failure)
		if [ "$COVERAGE" = "true" ]; then
			for i in `find tests -name "*.log" 2>/dev/null`; do
				echo "-- START ${i}";
				cat $i;
				echo "-- END";
			done
		fi
		;;
esac

exit 0


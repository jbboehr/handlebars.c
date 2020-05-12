#!/usr/bin/env bash

set -exE

export CC="$MYCC"
export PREFIX="$HOME/build"
export PATH="$PREFIX/bin:$PATH"
export CFLAGS="-g -O2 -L$PREFIX/lib $CFLAGS"
export CPPFLAGS="-I$PREFIX/include"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:/usr/lib/$ARCH-linux-gnu/pkgconfig"

if [ "$COVERAGE" = "true" ]; then
	export CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	export LDFLAGS="$LDFLAGS --coverage"
fi

if [ "$HARDENING" = "true" ]; then
	export CFLAGS="$CFLAGS -Werror=format-security -Wp,-D_FORTIFY_SOURCE=2 -Wp,-D_GLIBCXX_ASSERTIONS -fexceptions -fstack-protector-strong -grecord-gcc-switches"
	export CFLAGS="$CFLAGS -specs=`pwd`/tests/redhat-hardened-cc1 -specs=`pwd`/tests/redhat-hardened-ld"
	export CFLAGS="$CFLAGS -fasynchronous-unwind-tables -fstack-clash-protection -fPIC -DPIC"
	# this is not supported on travis: -fcf-protection
fi

if [ "$ARCH" = "i386" ]; then
	export CFLAGS="$CFLAGS -m32"
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

case $1 in
	before_install)
		mkdir -p $PREFIX/include $PREFIX/lib/pkgconfig
		apt-add-repository -y ppa:ubuntu-toolchain-r/test
		apt-get update -y
		apt-get install $MYCC
		apt-get install -y automake pkg-config gcc-multilib
		# we commit the generated files for these now:
		# apt-get install -y bison flex gperf re2c
		apt-get purge -y bison flex gperf re2c
		# install arch-specific packages
		apt-get install -y check:$ARCH libjson-c-dev:$ARCH liblmdb-dev:$ARCH libpcre3-dev:$ARCH libtalloc-dev:$ARCH libyaml-dev:$ARCH libsubunit-dev:$ARCH
		if [ "$COVERAGE" = "true" ]; then
			apt-get install -y lcov
			gem install coveralls-lcov
		fi
		;;

	install)
		./bootstrap

		trap "cat config.log" ERR
		./configure --build="$ARCH" --prefix="$PREFIX" --enable-handlebars-memory
		trap - ERR

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


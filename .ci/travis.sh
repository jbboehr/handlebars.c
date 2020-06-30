#!/usr/bin/env bash

set -o errexit -o pipefail

source .ci/fold.sh

export PS4=' \e[33m$(date +"%H:%M:%S"): $BASH_SOURCE@$LINENO ${FUNCNAME[0]} -> \e[0m'
export CC="$MYCC"
export PREFIX="$HOME/build"
export PATH="$PREFIX/bin:$PATH"
export CFLAGS="${CFLAGS} -I$PREFIX/include -I$PREFIX/include/json-c"
export LDFLAGS="${LDFLAGS} -L$PREFIX/lib $LDFLAGS"
export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}:$PREFIX/lib/pkgconfig:/usr/lib/$ARCH-linux-gnu/pkgconfig"
export LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
export SUDO="sudo"

# these mess up our configuration for LTO, make sure they are unset
unset RANLIB
unset AR
unset NM
unset LD

mkdir -p "${PREFIX}/include" "${PREFIX}/include/json-c" "${PREFIX}/lib/pkgconfig"

if [ -z "$ARCH" ]; then
	export ARCH=${TRAVIS_CPU_ARCH}
fi

if [ "$ARCH" = "i386" ]; then
	export HOST_ARCH=i686-linux-gnu
	export CFLAGS="$CFLAGS -m32"
	$SUDO dpkg --add-architecture i386
	unset CC
elif [ "$ARCH" = "armhf" ]; then
	export HOST_ARCH=arm-linux-gnueabihf
	$SUDO dpkg --add-architecture armhf
	unset CC
elif [ "$ARCH" = "ppc64le" ]; then
	# debian calls this differently
	export ARCH="ppc64el"
fi

if [ "$VALGRIND" == "true" ]; then
	export CHECK_TARGET=check-valgrind
else
	export CHECK_TARGET=check
fi

function install_apt_packages() (
    set -o errexit -o pipefail -o xtrace

	# we only need ubuntu-toolchain-r/test for gcc-10 now
	if [ "$MYCC" = "gcc-10" ]; then
		$SUDO apt-add-repository -y ppa:ubuntu-toolchain-r/test
	fi

	$SUDO apt-get update -y

	# we commit the generated files for these now, purge them
	$SUDO apt-get purge -y bison flex gperf re2c valgrind

	local apt_packages_to_install="${MYCC} autotools-dev autoconf automake libtool m4 make bats pkg-config:${ARCH} check:${ARCH} libpcre3-dev:${ARCH} libtalloc-dev:${ARCH} libsubunit-dev:${ARCH}"
	if [ "$ARCH" = "i386" ]; then
		apt_packages_to_install="${apt_packages_to_install} ${MYCC}-i686-linux-gnu crossbuild-essential-i386"
	elif [ "$ARCH" = "armhf" ]; then
		apt_packages_to_install="${apt_packages_to_install} ${MYCC}-arm-linux-gnueabihf crossbuild-essential-armhf"
		# this is brittle af
		# apt_packages_to_install="${apt_packages_to_install} libjson-c4:${ARCH} liblmdb0:${ARCH}"
	fi
	if [ ! -z "$GCOV" ]; then
		apt_packages_to_install="${apt_packages_to_install} lcov"
	fi
	if [ "$MINIMAL" != "true" ]; then
		# json-c might be having issues: https://bugs.launchpad.net/ubuntu/+source/json-c/+bug/1878738
		apt_packages_to_install="${apt_packages_to_install} libjson-c-dev:${ARCH} liblmdb-dev:${ARCH} libyaml-dev:${ARCH}"
	fi
	if [ "$VALGRIND" == "true" ]; then
		apt_packages_to_install="${apt_packages_to_install} valgrind:${ARCH}"
	fi

	$SUDO apt-get install -y ${apt_packages_to_install}
)

function install_coveralls_lcov() (
    set -o errexit -o pipefail -o xtrace

	if [ ! -z "$GCOV" ]; then
		gem install coveralls-lcov
	fi
)

function before_install() (
    set -o errexit -o pipefail

    if [[ ! -z "${TRAVIS}" ]] || [[ ! -z "${GITHUB_RUN_ID}" ]]; then
        cifold "install apt packages" install_apt_packages
	fi

	if [ ! -z "$GCOV" ]; then
        cifold "install coveralls-lcov" install_coveralls_lcov
    fi
)

function configure_handlebars() {
    set -o errexit -o pipefail -o xtrace

	# cflags
	export CFLAGS="$CFLAGS -g -O2"

	# json-c undeprecated json_object_object_get, but the version in xenial
	# is too old, so let's silence deprecated warnings. le sigh.
	export CFLAGS="$CFLAGS -Wno-deprecated-declarations -Wno-error=deprecated-declarations"

	# configure flags
	local extra_configure_flags="--prefix=${PREFIX}"

	if [ -n "$HOST_ARCH" ]; then
		extra_configure_flags="${extra_configure_flags} --host=${HOST_ARCH}"
	fi

	if [ ! -z "$GCOV" ]; then
		extra_configure_flags="$extra_configure_flags --enable-code-coverage --with-gcov=$GCOV"
	fi

	if [ "$DEBUG" == "true" ]; then
		extra_configure_flags="$extra_configure_flags --enable-debug"
	else
		extra_configure_flags="$extra_configure_flags --disable-debug"
	fi

	if [ "$HARDENING" != "false" ]; then
		extra_configure_flags="$extra_configure_flags --enable-hardening"
	else
		extra_configure_flags="$extra_configure_flags --disable-hardening"
	fi

	if [ "$LTO" = "true" ]; then
		extra_configure_flags="${extra_configure_flags} --enable-lto --disable-shared"
		# seems to break on Travis with: Error: no such instruction: `endbr64'
		export CFLAGS="$CFLAGS -fcf-protection=none"
	else
		extra_configure_flags="${extra_configure_flags} --disable-lto"
	fi

	if [ "$MINIMAL" = "true" ]; then
		extra_configure_flags="${extra_configure_flags} --disable-testing-exports --disable-handlebars-memory --enable-check --disable-json --disable-lmdb --enable-pcre --disable-pthread --enable-subunit --disable-yaml"
	else
		extra_configure_flags="${extra_configure_flags} --enable-testing-exports --enable-handlebars-memory --enable-check --enable-json --enable-lmdb --enable-pcre  --enable-pthread --enable-subunit --enable-yaml --enable-benchmark"
	fi

	if [ "$VALGRIND" = "true" ]; then
		extra_configure_flags="${extra_configure_flags} --enable-valgrind"
	else
		extra_configure_flags="${extra_configure_flags} --disable-valgrind"
	fi

	autoreconf -v

	# trap "cat config.log" ERR
	./configure ${extra_configure_flags}
	# trap - ERR
}

function make_handlebars() (
    set -o errexit -o pipefail -o xtrace

	make clean all
)

function install_handlebars() (
    set -o errexit -o pipefail -o xtrace

	make install
)

function install() (
    set -o errexit -o pipefail

    cifold "main configure step" configure_handlebars
    cifold "main build step" make_handlebars
    cifold "main install step" install_handlebars
)

function test_handlebars() (
    set -o errexit -o pipefail -o xtrace

	make ${CHECK_TARGET}

	if [ ! -z "$GCOV" ]; then
		make code-coverage-capture
	fi

	if [ -f ./bench/run.sh.log ]; then
		cat ./bench/run.sh.log
	fi
)

function script() (
    set -o errexit -o pipefail

    cifold "main test suite" test_handlebars
)

function upload_coverage() (
    set -o errexit -o pipefail -o xtrace

	if [ ! -z "$GCOV" ]; then
		coveralls-lcov handlebars-coverage.info
	fi
)

function after_success() (
    set -o errexit -o pipefail

	if [ ! -z "$GCOV" ]; then
        cifold "upload coverage" upload_coverage
	fi
)

function dump_logs() (
    #set -o errexit -o pipefail

	for i in `find bench tests -name "*.log" 2>/dev/null`; do
		echo "-- START ${i}";
		cat "${i}";
		echo "-- END";
	done
)

function after_failure() (
    set -o errexit -o pipefail

    cifold "dump logs" dump_logs
)

function run_all() (
    set -o errexit -o pipefail
    trap after_failure ERR
    before_install
    install
    # before_script
    script
    after_success
)

if [ "$1" == "run-all-now" ]; then
    run_all
fi

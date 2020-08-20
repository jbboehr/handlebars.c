
ARG BASE_IMAGE=fedora:latest

# image0
FROM ${BASE_IMAGE}
WORKDIR /build/handlebars.c

# handlebars.c
RUN dnf groupinstall 'Development Tools' -y
RUN dnf install \
    git-all \
    gcc \
    automake \
    autoconf \
    libtool \
    libyaml-devel \
    json-c-devel \
    libtalloc-devel \
    pcre-devel \
    check-devel \
    bats \
    -y
ADD . .
RUN autoreconf -fiv
RUN ./configure \
        --prefix /usr/local/ \
        --enable-lto \
        --enable-static \
        --enable-hardening \
        --disable-shared \
        --disable-debug \
        --disable-code-coverage \
        --disable-lmdb \
        --disable-pthread \
        --disable-valgrind \
        --disable-testing-exports \
        RANLIB=gcc-ranlib \
        AR=gcc-ar \
        NM=gcc-nm \
        LD=gcc \
        CFLAGS="-O3"
RUN make
RUN make check
RUN sudo make install
RUN sudo ldconfig

# image1
FROM ${BASE_IMAGE}
WORKDIR /srv/
RUN dnf install libyaml json-c libtalloc -y
COPY --from=0 /usr/local/bin/handlebarsc /usr/local/bin/handlebarsc
ENTRYPOINT ["/usr/local/bin/handlebarsc"]

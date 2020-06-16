FROM alpine:latest
WORKDIR /srv/handlebars.c
RUN apk update && \
    apk --no-cache add alpine-sdk automake autoconf libtool talloc-dev json-c-dev yaml-dev \
        pcre-dev check-dev bats
ADD . .
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
        LD=gcc
RUN make
RUN make check
RUN make install

FROM alpine:latest
WORKDIR /srv/
RUN apk --no-cache add talloc json-c yaml
COPY --from=0 /usr/local/bin/handlebarsc /usr/local/bin/handlebarsc
ENTRYPOINT ["/usr/local/bin/handlebarsc"]

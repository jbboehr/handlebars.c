{ lib
, stdenv
, fetchurl
, pkgconfig
, makeWrapper
, autoconf
, automake
, autoreconfHook
, libtool
, m4
, # autoconf
  cmake
, # cmake
  glib
, json_c
, lmdb
, talloc
, libyaml
, # deps
  doxygen
, # doc deps
  check
, subunit
, bats
, pcre
, # testing deps
  autoconf-archive
, bison
, flex
, gperf
, lcov
, re2c
, valgrind
, kcachegrind
, bc
, # dev deps
  handlebars_spec
, mustache_spec
, # my special stuff
  gitignoreSource
, # source options
  handlebarscVersion ? null
, handlebarscSrc ? null
, handlebarscSha256 ? null
, # configure options
  checkSupport ? true
, cmakeSupport ? false
, debugSupport ? false
, devSupport ? false
, doxygenSupport ? false
, hardeningSupport ? (!debugSupport && !devSupport)
, jsonSupport ? true
, lmdbSupport ? true
, ltoSupport ? false
, noRefcountingSupport ? false
, pthreadSupport ? true
, sharedSupport ? true
, staticSupport ? true
, WerrorSupport ? (debugSupport || devSupport)
, valgrindSupport ? (debugSupport || devSupport)
, yamlSupport ? true
}:

stdenv.mkDerivation rec {
  name = "handlebars.c-${version}";
  version = "v1.0.0";

  src = lib.cleanSourceWith {
    filter = (path: type: (builtins.all (x: x != baseNameOf path)
      [ ".idea" ".git" "nix" "ci.nix" ".travis.sh" ".travis.yml" ".github" "flake.nix" "flake.lock" ]));
    src = gitignoreSource ../.;
  };

  outputs = [ "out" "dev" "bin" ]
    ++ lib.optionals doxygenSupport [ "man" "doc" ]
  ;

  enableParallelBuilding = true;

  buildInputs = [ glib ]
    ++ lib.optional checkSupport pcre
    ++ lib.optional jsonSupport json_c
    ++ lib.optional lmdbSupport lmdb
    ++ lib.optional yamlSupport libyaml
  ;

  propagatedBuildInputs = [ talloc ];

  nativeBuildInputs = [ makeWrapper ]
    ++ lib.optionals checkSupport [ handlebars_spec mustache_spec check subunit bats ]
    ++ lib.optional cmakeSupport cmake
    ++ lib.optionals (!cmakeSupport) [ autoreconfHook autoconf automake libtool m4 ]
    ++ lib.optionals devSupport [ autoconf-archive bc bison gperf flex kcachegrind lcov re2c valgrind ]
    ++ lib.optional doxygenSupport doxygen
    ++ lib.optional valgrindSupport valgrind
  ;

  propagatedNativeBuildInputs = [ pkgconfig ];

  configureFlags = [ "--bindir=$(bin)/bin" ]
    ++ lib.optionals checkSupport [
    "--with-handlebars-spec=${handlebars_spec}/share/handlebars-spec/"
    "--with-mustache-spec=${mustache_spec}/share/mustache-spec/"
    "--enable-check"
    "--enable-pcre"
    "--enable-subunit"
    "--enable-testing-exports"
  ]
    ++ lib.optionals (!checkSupport) [
    "--disable-check"
    "--disable-pcre"
    "--disable-subunit"
    "--disable-testing-exports"
  ]
    ++ lib.optional debugSupport "--enable-debug"
    ++ lib.optionals doxygenSupport [ "--mandir=$(man)/share/man" "--docdir=$(doc)/share/doc" ]
    ++ lib.optional hardeningSupport "--enable-hardening"
    ++ lib.optional (!hardeningSupport) "--disable-hardening"
    ++ lib.optional jsonSupport "--enable-json"
    ++ lib.optional (!jsonSupport) "--disable-json"
    ++ lib.optional lmdbSupport "--enable-lmdb"
    ++ lib.optional (!lmdbSupport) "--disable-lmdb"
    ++ lib.optional ltoSupport "--enable-lto"
    # not sure if this is a good idea, but I don't think it'll work without it
    ++ lib.optionals ltoSupport [ "RANLIB=" "AR=" "NM=" "LD=" ]
    ++ lib.optional (!ltoSupport) "--disable-lto"
    ++ lib.optional noRefcountingSupport "--disable-refcounting"
    ++ lib.optional pthreadSupport "--enable-pthread"
    ++ lib.optional (!pthreadSupport) "--disable-pthread"
    ++ lib.optional (sharedSupport && !ltoSupport) "--enable-shared"
    ++ lib.optional (!sharedSupport || ltoSupport) "--disable-shared"
    ++ lib.optional staticSupport "--enable-static"
    ++ lib.optional (!staticSupport) "--disable-static"
    ++ lib.optional valgrindSupport "--enable-valgrind"
    ++ lib.optional (!valgrindSupport) "--disable-valgrind"
    ++ lib.optional WerrorSupport "--enable-compile-warnings=error"
    ++ lib.optionals (!WerrorSupport) [ "--enable-compile-warnings=yes" "--disable-Werror" ]
    ++ lib.optional yamlSupport "--enable-yaml"
    ++ lib.optional (!yamlSupport) "--disable-yaml"
  ;

  cmakeFlags = [ ]
    ++ lib.optional debugSupport "-DCMAKE_BUILD_TYPE=Debug"
    ++ lib.optional (!debugSupport) "-DCMAKE_BUILD_TYPE=Release"
    ++ lib.optional checkSupport "-DHANDLEBARS_ENABLE_TESTS=1"
  ;

  preConfigure = lib.optionalString checkSupport ''
    patchShebangs ./bench/run.sh
    patchShebangs ./tests/test_executable.bats
    export handlebars_export_dir=${handlebars_spec}/share/handlebars-spec/export/
    export handlebars_spec_dir=${handlebars_spec}/share/handlebars-spec/spec/
    export handlebars_tokenizer_spec=${handlebars_spec}/share/handlebars-spec/spec/tokenizer.json
    export handlebars_parser_spec=${handlebars_spec}/share/handlebars-spec/spec/parser.json
    export mustache_spec_dir=${mustache_spec}/share/mustache-spec/specs
  '';

  doCheck = checkSupport;
  checkTarget = if cmakeSupport then "test" else (if valgrindSupport then "check-valgrind" else "check");

  meta = with lib; {
    description = "C implementation of handlebars.js";
    homepage = https://github.com/jbboehr/handlebars.c;
    license = "LGPLv2.1+";
    maintainers = [ "John Boehr <jbboehr@gmail.com>" ];
    mainProgram = "handlebarsc";
  };
}

{
  stdenv, fetchurl, pkgconfig,
  glib, json_c, lmdb, talloc, libyaml, pcre, # deps
  check, # doc deps
  handlebars_spec, mustache_spec, # my special stuff
  autoconf-archive, bison, flex, gperf, re2c, valgrind, kcachegrind, # debug
  autoconf, automake, autoreconfHook, libtool, m4, # autoconf
  cmake, # cmake
  handlebarscDebug ? false,
  handlebarscDev ? false,
  handlebarscWithCmake ? false,
  handlebarscRefcounting ? true,
  handlebarscWerror ? false,
  handlebarscVersion ? null,
  handlebarscSrc ? null,
  handlebarscSha256 ? null
}:

let
  debug = false;
  orDefault = x: y: (if (!isNull x) then x else y);
in

stdenv.mkDerivation rec {
  name = "handlebars.c-${version}";
  version = orDefault handlebarscVersion "v0.7.1";

  src = orDefault handlebarscSrc (fetchurl {
    url = "https://github.com/jbboehr/handlebars.c/archive/${version}.tar.gz";
    sha256 = orDefault handlebarscSha256 "1irss3zbpjshmlp0ng1pr796hqzs151yhj8y6gp2jwgin9ha2dr8";
  });

  outputs = [ "out" "dev" "bin" ];

  enableParallelBuilding = true;
  buildInputs = [ glib json_c lmdb pcre libyaml ];
  propagatedBuildInputs = [ talloc pkgconfig ];
  nativeBuildInputs = [ handlebars_spec mustache_spec check ]
    ++ (if handlebarscWithCmake then [ cmake ] else [ autoreconfHook autoconf automake libtool m4 ])
    ++ (if handlebarscDev then [ valgrind kcachegrind autoconf-archive bison gperf flex re2c ] else []);

  doCheck = true;
  configureFlags = [
      "--with-handlebars-spec=${handlebars_spec}/share/handlebars-spec/"
      "--with-mustache-spec=${mustache_spec}/share/mustache-spec/"
      "--bindir=$(bin)/bin"
    ]
    ++ (if handlebarscRefcounting then [] else ["--disable-refcounting"])
    ++ (if handlebarscWerror then ["--enable-compile-warnings=error"] else ["--enable-compile-warnings=yes --disable-Werror"])
    ++ (if handlebarscDebug then ["--enable-debug"] else []);

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"}"
    "-DHANDLEBARS_ENABLE_TESTS=1"
  ];

  hardeningDisable = if handlebarscDebug then [ "fortify" ] else [];

  preConfigure = ''
      export handlebars_export_dir=${handlebars_spec}/share/handlebars-spec/export/
      export handlebars_spec_dir=${handlebars_spec}/share/handlebars-spec/spec/
      export handlebars_tokenizer_spec=${handlebars_spec}/share/handlebars-spec/spec/tokenizer.json
      export handlebars_parser_spec=${handlebars_spec}/share/handlebars-spec/spec/parser.json
      export mustache_spec_dir=${mustache_spec}/share/mustache-spec/specs
    '';

  meta = with stdenv.lib; {
    description = "C implementation of handlebars.js";
    homepage = https://github.com/jbboehr/handlebars.c;
    license = "LGPLv2.1+";
    maintainers = [ "John Boehr <jbboehr@gmail.com>" ];
  };
}


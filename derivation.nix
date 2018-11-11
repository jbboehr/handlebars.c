{ stdenv, fetchurl, pkgconfig, glib, autoconf, automake, json_c, lmdb,
  talloc, libyaml, libtool, m4, pcre, check, handlebars_spec, mustache_spec, autoreconfHook,
  handlebarscVersion ? null, handlebarscSrc ? null, handlebarscSha256 ? null }:

let
  orDefault = x: y: (if (!isNull x) then x else y);
in

stdenv.mkDerivation rec {
  name = "handlebars.c-${version}";
  version = orDefault handlebarscVersion "v0.6.4";

  src = orDefault handlebarscSrc (fetchurl {
    url = "https://github.com/jbboehr/handlebars.c/archive/${version}.tar.gz";
    sha256 = orDefault handlebarscSha256 "0vmq3dxgbpx3yba0nvnxmcmc902yl7a7s49iv4cbb0m7jz0zbd8q";
  });

  outputs = [ "bin" "dev" "out" ];

  enableParallelBuilding = true;
  buildInputs = [ glib json_c lmdb pcre libyaml ];
  propagatedBuildInputs = [ talloc pkgconfig ];
  nativeBuildInputs = [ autoreconfHook handlebars_spec mustache_spec check libtool m4 autoconf automake ];

  doCheck = true;
  configureFlags = [
    "--with-handlebars-spec=${handlebars_spec}/share/handlebars-spec/"
    "--with-mustache-spec=${mustache_spec}/share/mustache-spec/"
    "--bindir=$(bin)/bin"
  ];

  meta = with stdenv.lib; {
    description = "C implementation of handlebars.js";
    homepage = https://github.com/jbboehr/handlebars.c;
    license = "LGPLv2.1+";
    maintainers = [ "John Boehr <jbboehr@gmail.com>" ];
  };
}


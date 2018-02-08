{ stdenv, fetchurl, pkgconfig, glib, autoconf, automake, json_c, lmdb,
  talloc, libyaml, libtool, m4, pcre, check, handlebars_spec, mustache_spec, autoreconfHook }:

stdenv.mkDerivation rec {
  name = "handlebars-0.6.4";

  src = fetchurl {
    url = https://github.com/jbboehr/handlebars.c/archive/v0.6.4.tar.gz;
    sha256 = "18b5f5c197a782b518d931117dd4a15e80c42aabdd6e0bd4f2a3dff57a1bb86e";
  };

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

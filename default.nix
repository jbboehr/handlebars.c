{
  pkgs ? (import <nixpkgs> {}),
  stdenv ? pkgs.stdenv,

  handlebarscVersion ? "v0.7.2",
  handlebarscSrc ? ./.,
  handlebarscSha256 ? null,

  mustache_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/mustache-spec/archive/5b85c1b58309e241a6f7c09fa57bd1c7b16fa9be.tar.gz;
    sha256 = "1h9zsnj4h8qdnzji5l9f9zmdy1nyxnf8by9869plyn7qlk71gdyv";
  }))) {},

  handlebars_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/handlebars-spec/archive/2dedb7ab0bb0088f2a8ea588759b1016ed37c82d.tar.gz;
    sha256 = "0c4f0aydy5ni3skbyvsrg6yskvljmsrqhhpx54lk0jlwblqziah4";
  }))) {},

  checkSupport ? true,
  cmakeSupport ? false,
  debugSupport ? false,
  devSupport ? false,
  doxygenSupport ? false,
  hardeningSupport ? true,
  jsonSupport ? true,
  lmdbSupport ? true,
  ltoSupport ? true,
  noRefcountingSupport ? false,
  pthreadSupport ? true,
  WerrorSupport ? false,
  valgrindSupport ? false,
  yamlSupport ? true
}:

pkgs.callPackage ./derivation.nix {
  inherit stdenv;
  inherit handlebarscVersion handlebarscSrc handlebarscSha256;
  inherit mustache_spec handlebars_spec;
  inherit checkSupport cmakeSupport debugSupport devSupport doxygenSupport hardeningSupport jsonSupport lmdbSupport ltoSupport noRefcountingSupport pthreadSupport WerrorSupport valgrindSupport yamlSupport;
}

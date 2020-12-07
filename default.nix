{
  pkgs ? (import <nixpkgs> {}),
  stdenv ? pkgs.stdenv,

  gitignoreSource ? (import (pkgs.fetchFromGitHub {
      owner = "hercules-ci";
      repo = "gitignore";
      rev = "00b237fb1813c48e20ee2021deb6f3f03843e9e4";
      sha256 = "sha256:186pvp1y5fid8mm8c7ycjzwzhv7i6s3hh33rbi05ggrs7r3as3yy";
  }) { inherit (pkgs) lib; }).gitignoreSource,

  handlebarscVersion ? "v0.7.3",
  handlebarscSha256 ? null,
  handlebarscSrc ? pkgs.lib.cleanSourceWith {
    filter = (path: type: (builtins.all (x: x != baseNameOf path) [".idea" ".git" "ci.nix" ".travis.sh" ".travis.yml" ".github"]));
    src = gitignoreSource ./.;
  },

  mustache_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/mustache-spec/archive/5b85c1b58309e241a6f7c09fa57bd1c7b16fa9be.tar.gz;
    sha256 = "1h9zsnj4h8qdnzji5l9f9zmdy1nyxnf8by9869plyn7qlk71gdyv";
  }))) {},

  handlebars_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/handlebars-spec/archive/2dedb7ab0bb0088f2a8ea588759b1016ed37c82d.tar.gz;
    sha256 = "0c4f0aydy5ni3skbyvsrg6yskvljmsrqhhpx54lk0jlwblqziah4";
  }))) {},

  check ? pkgs.callPackage ./nix/check.nix {},

  checkSupport ? true,
  cmakeSupport ? false,
  debugSupport ? false,
  devSupport ? false,
  doxygenSupport ? false,
  hardeningSupport ? true,
  jsonSupport ? true,
  lmdbSupport ? true,
  ltoSupport ? false,
  noRefcountingSupport ? false,
  pthreadSupport ? true,
  sharedSupport ? true,
  staticSupport ? true,
  WerrorSupport ? false,
  valgrindSupport ? false,
  yamlSupport ? true
}:

pkgs.callPackage ./nix/derivation.nix {
  inherit stdenv check;
  inherit handlebarscVersion handlebarscSrc handlebarscSha256;
  inherit mustache_spec handlebars_spec;
  inherit checkSupport cmakeSupport debugSupport devSupport doxygenSupport hardeningSupport jsonSupport lmdbSupport ltoSupport noRefcountingSupport pthreadSupport sharedSupport staticSupport WerrorSupport valgrindSupport yamlSupport;
}

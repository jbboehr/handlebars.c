{
  pkgs ? (import <nixpkgs> {}),
  stdenv ? pkgs.stdenv,
  gitignoreSource ?
    (import
      (pkgs.fetchFromGitHub {
        owner = "hercules-ci";
        repo = "gitignore";
        rev = "00b237fb1813c48e20ee2021deb6f3f03843e9e4";
        sha256 = "sha256:186pvp1y5fid8mm8c7ycjzwzhv7i6s3hh33rbi05ggrs7r3as3yy";
      })
      {inherit (pkgs) lib;})
    .gitignoreSource,
  mustache_spec ?
    pkgs.callPackage
    (import (fetchTarball {
      url = "https://github.com/jbboehr/mustache-spec/archive/5b85c1b58309e241a6f7c09fa57bd1c7b16fa9be.tar.gz";
      sha256 = "1h9zsnj4h8qdnzji5l9f9zmdy1nyxnf8by9869plyn7qlk71gdyv";
    }))
    {},
  handlebars_spec ?
    pkgs.callPackage
    (import (fetchTarball {
      url = "https://github.com/jbboehr/handlebars-spec/archive/3eb919f19988f37a539779c08342d2ce50aa75d0.tar.gz";
      sha256 = "088qzggkgl1v1a15l1plxdwiphh773q50k3w4pj0v45qc1cgyr7c";
    }))
    {},
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
  yamlSupport ? true,
}:
pkgs.callPackage ./nix/derivation.nix {
  inherit stdenv gitignoreSource;
  inherit mustache_spec handlebars_spec;
  inherit checkSupport cmakeSupport debugSupport devSupport doxygenSupport hardeningSupport jsonSupport lmdbSupport ltoSupport noRefcountingSupport pthreadSupport sharedSupport staticSupport WerrorSupport valgrindSupport yamlSupport;
}

# nix-shell --arg stdenv '(import <nixpkgs> {}).clangStdenv'
{ pkgs ? (import <nixpkgs> { })
, stdenv ? pkgs.stdenv
, checkSupport ? true
, cmakeSupport ? false
, debugSupport ? true
, # different than default.nix
  devSupport ? true
, # different than default.nix
  doxygenSupport ? false
, hardeningSupport ? true
, jsonSupport ? true
, lmdbSupport ? true
, ltoSupport ? false
, noRefcountingSupport ? false
, pthreadSupport ? true
, sharedSupport ? true
, staticSupport ? true
, WerrorSupport ? true
, # different than default.nix
  valgrindSupport ? true
, # different than default.nix
  yamlSupport ? true
}:

let
  args = {
    inherit stdenv;
    inherit checkSupport cmakeSupport debugSupport devSupport doxygenSupport hardeningSupport jsonSupport lmdbSupport ltoSupport noRefcountingSupport pthreadSupport sharedSupport staticSupport WerrorSupport valgrindSupport yamlSupport;
  };
in

import ./default.nix args

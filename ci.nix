let
  generateHandlebarsCTestsForPlatform = { pkgs, stdenv, handlebarscWithCmake ? false, handlebarscRefcounting ? true, handlebarscDebug ? false }:
    pkgs.callPackage ./default.nix {
      inherit stdenv handlebarscWithCmake handlebarscRefcounting handlebarscDebug;
      handlebarscWerror = true;

      handlebarscSrc = builtins.filterSource
        (path: type: baseNameOf path != ".idea" && baseNameOf path != ".git" && baseNameOf path != "ci.nix")
        ./.;
    };

  generateHandlebarsCTestsForPlatform2 = { pkgs, ... }@args:
    pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform (args // { stdenv = pkgs.stdenv; });
      clang = generateHandlebarsCTestsForPlatform (args // { stdenv = pkgs.clangStdenv; });
    };
in
builtins.mapAttrs (k: _v:
  let
    path = builtins.fetchTarball {
        url = https://github.com/NixOS/nixpkgs/archive/release-19.09.tar.gz;
        name = "nixpkgs-19.09";
    };
    pkgs = import (path) { system = k; };
  in
  pkgs.recurseIntoAttrs {
    n1809 = let
        path = builtins.fetchTarball {
           url = https://github.com/NixOS/nixpkgs/archive/release-18.09.tar.gz;
           name = "nixpkgs-18.09";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2  {
      inherit pkgs;
    };

    n1903 = let
      path = builtins.fetchTarball {
        url = https://github.com/NixOS/nixpkgs/archive/release-19.03.tar.gz;
        name = "nixpkgs-19.03";
      };
      pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2  {
      inherit pkgs;
    };

    n1909 = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
    };

    # cmake
    n1909-cmake = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
      handlebarscWithCmake = true;
    };

    # 32bit (gcc only)
   n1909-32bit = pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform {
        pkgs = pkgs.pkgsi686Linux;
        stdenv = pkgs.pkgsi686Linux.stdenv;
      };
    };

    # refcounting disabled
    n1909-norc = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
      handlebarscRefcounting = false;
    };

    # debug/dev
    n1909-debug = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
      handlebarscDebug = true;
    };

    n2003 = let
        path = builtins.fetchTarball {
           url = https://github.com/NixOS/nixpkgs/archive/release-20.03.tar.gz;
           name = "nixpkgs-20.03";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
    };

    unstable = let
       path = builtins.fetchTarball {
          url = https://github.com/NixOS/nixpkgs/archive/master.tar.gz;
          name = "nixpkgs-unstable";
       };
       pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
    };
  }
) {
  x86_64-linux = {};
  # Uncomment to test build on macOS too
  # x86_64-darwin = {};
}

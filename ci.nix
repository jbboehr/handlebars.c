let
  generateHandlebarsCTestsForPlatform = { pkgs, stdenv, ... }@args:
    pkgs.callPackage ./default.nix (args // {
      inherit stdenv;
      WerrorSupport = true;
    });

  generateHandlebarsCTestsForPlatform2 = { pkgs, ... }@args:
    pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform (args // { stdenv = pkgs.stdenv; });
      # we need this or llvm-ar isn't found
      clang = generateHandlebarsCTestsForPlatform (args // {
        stdenv = with pkgs; overrideCC clangStdenv [ clang llvm lld ];
      });
      #clang = generateHandlebarsCTestsForPlatform (args // { stdenv = pkgs.clangStdenv; });
    };
in
builtins.mapAttrs (k: _v:
  let
    path = builtins.fetchTarball {
        url = https://github.com/NixOS/nixpkgs/archive/release-19.09.tar.gz;
        name = "nixpkgs-19.09";
    };
    pkgs = import (path) { system = k; };
    gitignoreSrc = pkgs.fetchFromGitHub {
      owner = "hercules-ci";
      repo = "gitignore";
      rev = "00b237fb1813c48e20ee2021deb6f3f03843e9e4";
      sha256 = "sha256:186pvp1y5fid8mm8c7ycjzwzhv7i6s3hh33rbi05ggrs7r3as3yy";
    };
    inherit (import gitignoreSrc { inherit (pkgs) lib; }) gitignoreSource;
    handlebarscSrc = pkgs.lib.cleanSourceWith {
      filter = (path: type: (builtins.all (x: x != baseNameOf path) [".idea" ".git" "ci.nix" ".travis.sh" ".travis.yml"]));
      src = gitignoreSource ./.;
    };
  in
  pkgs.recurseIntoAttrs {
    n1809 = let
        path = builtins.fetchTarball {
           url = https://github.com/NixOS/nixpkgs/archive/release-18.09.tar.gz;
           name = "nixpkgs-18.09";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2  {
      inherit pkgs handlebarscSrc;
    };

    n1903 = let
      path = builtins.fetchTarball {
        url = https://github.com/NixOS/nixpkgs/archive/release-19.03.tar.gz;
        name = "nixpkgs-19.03";
      };
      pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
    };

    n1909 = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
    };

    # cmake
    n1909-cmake = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      cmakeSupport = true;
    };

    # 32bit (gcc only)
   n1909-32bit = pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform {
        inherit handlebarscSrc;
        pkgs = pkgs.pkgsi686Linux;
        stdenv = pkgs.pkgsi686Linux.stdenv;
      };
    };

    # refcounting disabled
    n1909-norc = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      noRefcountingSupport = true;
    };

    # debug
    n1909-debug = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      debugSupport = true;
      hardeningSupport = false;
    };

    # lto
    n1909-lto = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      ltoSupport = true;
      sharedSupport = false;
    };

    # minimal
    n1909-minimal = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      debugSupport = false;
      hardeningSupport = false;
      jsonSupport = false;
      lmdbSupport = false;
      pthreadSupport = false;
      yamlSupport = false;
      valgrindSupport = false;
    };

    # static only
    n1909-static = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      sharedSupport = false;
    };

    # shared only
    n1909-shared = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      staticSupport = false;
    };

    # valgrind
    n1909-valgrind = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
      valgrindSupport = true;
    };

    n2003 = let
        path = builtins.fetchTarball {
           url = https://github.com/NixOS/nixpkgs/archive/release-20.03.tar.gz;
           name = "nixpkgs-20.03";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
    };

    unstable = let
       path = builtins.fetchTarball {
          url = https://github.com/NixOS/nixpkgs/archive/master.tar.gz;
          name = "nixpkgs-unstable";
       };
       pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs handlebarscSrc;
    };
  }
) {
  x86_64-linux = {};
  # Uncomment to test build on macOS too
  # x86_64-darwin = {};
}

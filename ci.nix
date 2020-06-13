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
      url = https://github.com/NixOS/nixpkgs-channels/archive/nixos-20.03.tar.gz;
      name = "nixos-20.03";
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
    generateHandlebarsCTestsForPlatform3 = { ... }@args:
      generateHandlebarsCTestsForPlatform2 { inherit pkgs handlebarscSrc; } // args;
  in
  pkgs.recurseIntoAttrs {
    n1909 = let
        path = builtins.fetchTarball {
          url = https://github.com/NixOS/nixpkgs-channels/archive/nixos-19.09.tar.gz;
          name = "nixos-19.09";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform3 {
      inherit pkgs;
    };

    n2003 = generateHandlebarsCTestsForPlatform3 {};

    # cmake
    n2003-cmake = generateHandlebarsCTestsForPlatform3 {
      cmakeSupport = true;
    };

    # 32bit (gcc only)
    n2003-32bit = pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform {
        inherit handlebarscSrc;
        pkgs = pkgs.pkgsi686Linux;
        stdenv = pkgs.pkgsi686Linux.stdenv;
      };
    };

    # refcounting disabled
    n2003-norc = generateHandlebarsCTestsForPlatform3 {
      noRefcountingSupport = true;
    };

    # debug
    n2003-debug = generateHandlebarsCTestsForPlatform3 {
      debugSupport = true;
      hardeningSupport = false;
    };

    # lto
    n2003-lto = generateHandlebarsCTestsForPlatform3 {
      ltoSupport = true;
      sharedSupport = false;
    };

    # minimal
    n2003-minimal = generateHandlebarsCTestsForPlatform3 {
      debugSupport = false;
      hardeningSupport = false;
      jsonSupport = false;
      lmdbSupport = false;
      pthreadSupport = false;
      yamlSupport = false;
      valgrindSupport = false;
    };

    # static only
    n2003-static = generateHandlebarsCTestsForPlatform3 {
      sharedSupport = false;
    };

    # shared only
    n2003-shared = generateHandlebarsCTestsForPlatform3 {
      staticSupport = false;
    };

    # valgrind
    n2003-valgrind = generateHandlebarsCTestsForPlatform3 {
      valgrindSupport = true;
    };

    # Uncomment to test build against unstable nixpkgs too
    # unstable = let
    #    path = builtins.fetchTarball {
    #       url = https://github.com/NixOS/nixpkgs/archive/master.tar.gz;
    #       name = "nixpkgs-unstable";
    #    };
    #    pkgs = import (path) { system = k; };
    # in generateHandlebarsCTestsForPlatform3 {
    #  inherit pkgs;
    # };
  }
) {
  x86_64-linux = {};
  # Uncomment to test build on macOS too
  # x86_64-darwin = {};
}

let
  generateHandlebarsCTestsForPlatform = { pkgs, stdenv, ... }@args:
    pkgs.callPackage ../default.nix (args // {
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
builtins.mapAttrs
  (k: _v:
  let
    path = builtins.fetchTarball {
      url = https://github.com/NixOS/nixpkgs/archive/nixos-22.11.tar.gz;
      name = "nixos-22.11";
    };
    pkgs = import (path) { system = k; };
    generateHandlebarsCTestsForPlatform3 = { ... }@args:
      generateHandlebarsCTestsForPlatform2 { inherit pkgs; } // args;
  in
  pkgs.recurseIntoAttrs {
    std = generateHandlebarsCTestsForPlatform3 { };

    # cmake
    cmake = generateHandlebarsCTestsForPlatform3 {
      cmakeSupport = true;
    };

    # 32bit (gcc only)
    # i686 = pkgs.recurseIntoAttrs {
    #   gcc = generateHandlebarsCTestsForPlatform {
    #     pkgs = pkgs.pkgsi686Linux;
    #     stdenv = pkgs.pkgsi686Linux.stdenv;
    #   };
    # };

    # refcounting disabled
    norc = generateHandlebarsCTestsForPlatform3 {
      noRefcountingSupport = true;
    };

    # debug
    debug = generateHandlebarsCTestsForPlatform3 {
      debugSupport = true;
      hardeningSupport = false;
    };

    # lto
    lto = generateHandlebarsCTestsForPlatform3 {
      ltoSupport = true;
      sharedSupport = false;
    };

    # minimal
    minimal = generateHandlebarsCTestsForPlatform3 {
      debugSupport = false;
      hardeningSupport = false;
      jsonSupport = false;
      lmdbSupport = false;
      pthreadSupport = false;
      yamlSupport = false;
      valgrindSupport = false;
    };

    # static only
    static = generateHandlebarsCTestsForPlatform3 {
      sharedSupport = false;
    };

    # shared only
    shared = generateHandlebarsCTestsForPlatform3 {
      staticSupport = false;
    };

    # valgrind
    valgrind = generateHandlebarsCTestsForPlatform3 {
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
  )
{
  x86_64-linux = { };
  # Uncomment to test build on macOS too
  # x86_64-darwin = {};
}

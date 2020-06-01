let
  generateHandlebarsCTestsForPlatform = { pkgs, stdenv, handlebarscWithCmake ? false }:
    pkgs.callPackage ./default.nix {
      inherit stdenv handlebarscWithCmake;

      handlebarscSrc = builtins.filterSource
        (path: type: baseNameOf path != ".idea" && baseNameOf path != ".git" && baseNameOf path != "ci.nix")
        ./.;
    };

  generateHandlebarsCTestsForPlatform2 = { pkgs, handlebarscWithCmake ? false }:
    pkgs.recurseIntoAttrs {
      gcc = generateHandlebarsCTestsForPlatform { inherit pkgs handlebarscWithCmake; stdenv = pkgs.stdenv; };
      clang = generateHandlebarsCTestsForPlatform { inherit pkgs handlebarscWithCmake; stdenv = pkgs.clangStdenv; };
    };
in
builtins.mapAttrs (k: _v:
  let
    path = builtins.fetchTarball {
      url = https://github.com/NixOS/nixpkgs/archive/release-20.03.tar.gz;
      name = "nixpkgs-20.03";
    };
    pkgs = import (path) { system = k; };
  in
  pkgs.recurseIntoAttrs {
    n1909 = let
        path = builtins.fetchTarball {
          url = https://github.com/NixOS/nixpkgs/archive/release-19.09.tar.gz;
          name = "nixpkgs-19.09";
        };
        pkgs = import (path) { system = k; };
    in generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
    };

    n2003 = generateHandlebarsCTestsForPlatform2 {
      inherit pkgs;
    };

    # test once with cmake
    n2003cmake = generateHandlebarsCTestsForPlatform2  {
      inherit pkgs;
      handlebarscWithCmake = true;
    };

    # unstable = let
    #    path = builtins.fetchTarball {
    #       url = https://github.com/NixOS/nixpkgs/archive/master.tar.gz;
    #       name = "nixpkgs-unstable";
    #    };
    #    pkgs = import (path) { system = k; };
    # in generateHandlebarsCTestsForPlatform2 {
    #  inherit pkgs;
    # };
  }
) {
  x86_64-linux = {};
  # Uncomment to test build on macOS too
  # x86_64-darwin = {};
}

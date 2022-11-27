{
  description = "handlebars.c";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils = {
      url = "github:numtide/flake-utils";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    mustache_spec = {
      url = "github:jbboehr/mustache-spec";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    handlebars_spec = {
      url = "github:jbboehr/handlebars-spec";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    gitignore = {
      url = "github:hercules-ci/gitignore.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, mustache_spec, handlebars_spec, gitignore }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      rec {
        packages = flake-utils.lib.flattenTree rec {
          handlebars-c = pkgs.callPackage ./nix/derivation.nix {
            handlebars_spec = handlebars_spec.packages.${system}.handlebars-spec;
            mustache_spec = mustache_spec.packages.${system}.mustache-spec;
            inherit (gitignore.lib) gitignoreSource;
          };
          default = handlebars-c;
        };

        devShells.default = packages.default;

        apps.default = {
          type = "app";
          program = "${packages.default.bin}/bin/handlebarsc";
        };
      }
    );
}

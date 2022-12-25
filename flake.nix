{
  description = "handlebars.c";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs";
    flake-utils = {
      url = "github:numtide/flake-utils";
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
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, mustache_spec, handlebars_spec, gitignore, pre-commit-hooks }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        src = pkgs.lib.cleanSourceWith {
          filter = (path: type: (builtins.all (x: x != baseNameOf path)
            [ ".idea" ".git" "nix" "ci.nix" ".travis.sh" ".travis.yml" ".github" "flake.nix" "flake.lock" ]));
          src = gitignore.lib.gitignoreSource ./.;
        };

        pre-commit-check = pre-commit-hooks.lib.${system}.run {
          inherit src;
          hooks = {
            #editorconfig-checker.enable = true;
            #markdownlint.enable = true;
            nixpkgs-fmt.enable = true;
          };
        };
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

        devShells.default = pkgs.mkShell {
          inputsFrom = [ packages.default ];
          buildInputs = with pkgs; [ nixpkgs-fmt pre-commit editorconfig-checker ];
          shellHook = ''
            ${pre-commit-check.shellHook}
          '';
        };

        checks = {
          # @todo we could move/copy everything in nix/ci.nix here
          inherit pre-commit-check;
          inherit (packages) default;
        };

        apps.default = {
          type = "app";
          program = "${packages.default.bin}/bin/handlebarsc";
        };

        formatter = pkgs.nixpkgs-fmt;
      }
    );
}

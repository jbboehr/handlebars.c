let
  defaultPkgs = import <nixpkgs> {};
  fetchurl = defaultPkgs.fetchurl;
  stdenv = defaultPkgs.stdenv;

  defaultMustacheSpec = stdenv.mkDerivation rec {
    name = "mustache-spec-1.1.2";

    builder = defaultPkgs.writeText "builder.sh" ''
        source $stdenv/setup
        
        buildPhase() {
            echo do nothing
        }
        
        installPhase() {
            mkdir -p $out/share/mustache-spec
            cp -prvd specs $out/share/mustache-spec/
        }
        
        genericBuild
      '';

    src = fetchurl {
      url = https://github.com/mustache/spec/archive/v1.1.2.tar.gz;
      sha256 = "477552869cf4a8d3cadb74f0d297988dfa9edddbc818ee8f56bae0a097dc657c";
    };

    meta = {
      description = "The mustache spec";
      homepage = https://github.com/mustache/spec;
      maintainers = [  ];
    };
  };

  handlebarsSpecTarball = builtins.fetchTarball "https://github.com/jbboehr/handlebars-spec/archive/b4f9deed481e370de8e8b8cef23cee30999c5c30.tar.gz";
  defaultHandlebarsSpec = defaultPkgs.callPackage (handlebarsSpecTarball + "/default.nix") {};
in
  { pkgs ? import <nixpkgs> {}, handlebars_spec ? defaultHandlebarsSpec, mustache_spec ? defaultMustacheSpec }:

  pkgs.callPackage ./derivation.nix {
    inherit mustache_spec handlebars_spec;
  }

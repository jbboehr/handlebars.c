{
  pkgs ? import <nixpkgs> {},

  handlebarscWithCmake ? false,
  handlebarscVersion ? "v0.7.2",
  handlebarscSrc ? ./.,
  handlebarscSha256 ? null,

  stdenv ? pkgs.stdenv,

  mustache_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/mustache-spec/archive/5b85c1b58309e241a6f7c09fa57bd1c7b16fa9be.tar.gz;
    sha256 = "1h9zsnj4h8qdnzji5l9f9zmdy1nyxnf8by9869plyn7qlk71gdyv";
  }))) {},

  handlebars_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/handlebars-spec/archive/v104.7.6.tar.gz;
    sha256 = "08dvx3s8j6i3npvh65halv18f5ilm0iisbrqxxv9gpfcav0m3hi6";
  }))) {}
}:

pkgs.callPackage ./derivation.nix {
  inherit mustache_spec handlebars_spec handlebarscVersion handlebarscSrc handlebarscSha256 handlebarscWithCmake;
}


{
  pkgs ? (import <nixpkgs> {}),

  handlebarscVersion ? "v0.7.2",
  handlebarscSrc ? ./.,
  handlebarscSha256 ? null,

  handlebarscDebug ? false,
  handlebarscDev ? false,
  handlebarscRefcounting ? true,
  handlebarscWerror ? false,
  handlebarscWithCmake ? false,

  stdenv ? pkgs.stdenv,

  mustache_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/mustache-spec/archive/5b85c1b58309e241a6f7c09fa57bd1c7b16fa9be.tar.gz;
    sha256 = "1h9zsnj4h8qdnzji5l9f9zmdy1nyxnf8by9869plyn7qlk71gdyv";
  }))) {},

  handlebars_spec ? pkgs.callPackage (import ((fetchTarball {
    url = https://github.com/jbboehr/handlebars-spec/archive/v104.7.6.tar.gz;
    sha256 = "0i2czm6yhiv5xbq93yj249xjxqrfv70mk1qgl0abkbm0qmmkc4vk";
  }))) {}
}:

pkgs.callPackage ./derivation.nix {
  inherit stdenv;
  inherit mustache_spec handlebars_spec;
  inherit handlebarscVersion handlebarscSrc handlebarscSha256;
  inherit handlebarscDebug handlebarscDev handlebarscWithCmake handlebarscRefcounting handlebarscWerror;
}

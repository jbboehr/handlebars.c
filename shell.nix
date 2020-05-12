{
  pkgs ? import <nixpkgs> {},
  ...
}@args:

pkgs.mkShell {
  inputsFrom = [ (import ./default.nix args) ];
  buildInputs = with pkgs; [ valgrind bison flex re2c ];
}

{
  pkgs ? import <nixpkgs> {},
}:

pkgs.stdenv.mkDerivation {
  name = "process-tree";
  src = ./src;
  phases = [ "unpackPhase" "buildPhase" "installPhase" ];
  installPhase = ''
    make install PREFIX=$out
  '';
}

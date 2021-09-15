with import <nixpkgs> {};
stdenv.mkDerivation {
  name = "sled";
  buildInputs = [ gnumake SDL2 ];
}

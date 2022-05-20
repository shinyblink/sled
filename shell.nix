with import <nixpkgs> {};
pkgs.mkShell {
  buildInputs = [ SDL2 ncurses ];
}

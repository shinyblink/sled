with import <nixpkgs> {};
stdenv.mkDerivation {
  src = ./.;
  name = "sled";
  buildInputs = [ SDL2 ncurses ];
  enableParallelBuilding = true;

  configurePhase = ''
      cat >sledconf <<EOF
      PROJECT := sled
      DEBUG := 0
      STATIC := 0
      PLATFORM := unix
      DEFAULT_OUTMOD := sdl2
      DEFAULT_MODULEDIR := "$out/libexec/sled"
      MODULES := out_\$(DEFAULT_OUTMOD)
      MODULES += \$(BGMMODS_DEFAULT)
      MODULES += \$(FLTMODS_DEFAULT)
      MODULES += \$(GFXMODS_DEFAULT)
      MATRIX_X := 64
      MATRIX_Y := 64
      SDL_SCALE_FACTOR := 4
      EOF
    '';

  installPhase = ''
      mkdir -p $out/bin
      install -m755 ./sled $out/bin/sled
      mkdir -p $out/libexec/sled
      install -m755 ./modules/* $out/libexec/sled
    '';
}

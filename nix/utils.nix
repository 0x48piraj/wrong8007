{
  lib,
  stdenv,
  makeWrapper,
  coreutils,
  gawk,
  gnugrep,
  gnused,
  kmod,
}:

stdenv.mkDerivation {
  pname = "wrong8007-utils";
  version = import ./version.nix { inherit lib; };

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../configs/wrong8007.conf.example
      ../tools/Makefile
      ../tools/params.sh
      ../tools/wrong8007ctl.c
    ];
  };

  nativeBuildInputs = [ makeWrapper ];

  buildPhase = ''
    runHook preBuild
    make -C tools
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    make -C tools install PREFIX=$out
    install -Dm555 tools/params.sh $out/bin/wrong8007-params
    install -Dm444 configs/wrong8007.conf.example $out/share/wrong8007/wrong8007.conf.example
    runHook postInstall
  '';

  postFixup = ''
    wrapProgram $out/bin/wrong8007-params --prefix PATH : ${
      lib.makeBinPath [
        coreutils
        gawk
        gnugrep
        gnused
        kmod
      ]
    }
  '';

  meta = {
    description = "Parameter check, control and heartbeat utilities for wrong8007";
    homepage = "https://github.com/0x48piraj/wrong8007";
    license = lib.licenses.gpl2Only;
    platforms = lib.platforms.linux;
    mainProgram = "wrong8007ctl";
  };
}

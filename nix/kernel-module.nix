{
  lib,
  stdenv,
  kernel,
  kernelModuleMakeFlags,
  # Build profile in Kconfig format; every trigger when null
  profile ? null,
}:

stdenv.mkDerivation {
  pname = "wrong8007";
  version = "${import ./version.nix { inherit lib; }}-${kernel.version}";

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions [
      ../Kbuild
      ../Kconfig
      ../Makefile
      ../dkms.conf
      ../core.c
      ../configs
      ../include
      ../lib
      ../trigger
      ../tools/validate-config.sh
    ];
  };

  nativeBuildInputs = kernel.moduleBuildDependencies;
  hardeningDisable = [ "pic" ];

  makeFlags =
    kernelModuleMakeFlags
    ++ [ "KDIR=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build" ]
    ++ lib.optional (profile != null) "PROFILE=${profile}";

  buildFlags = [ "build" ];

  installPhase = ''
    runHook preInstall
    install -Dm444 wrong8007.ko $out/lib/modules/${kernel.modDirVersion}/extra/wrong8007.ko
    runHook postInstall
  '';

  meta = {
    description = "Programmable kernel-resident dead man's switch for last-resort execution";
    homepage = "https://github.com/0x48piraj/wrong8007";
    license = lib.licenses.gpl2Only;
    platforms = lib.platforms.linux;
  };
}

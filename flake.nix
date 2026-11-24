{
  description = "Programmable kernel-resident dead man's switch for last-resort execution";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";

  outputs =
    { self, nixpkgs }:
    let
      inherit (nixpkgs) lib;
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = f: lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      overlays.default = final: prev: {
        wrong8007-utils = final.callPackage ./nix/utils.nix { };
        kernelPackagesExtensions = prev.kernelPackagesExtensions ++ [
          (kfinal: _: { wrong8007 = kfinal.callPackage ./nix/kernel-module.nix { }; })
        ];
      };

      nixosModules.default = ./nix/nixos-module.nix;

      packages = forAllSystems (
        pkgs:
        let
          wrong8007-utils = pkgs.callPackage ./nix/utils.nix { };
        in
        {
          default = wrong8007-utils;
          inherit wrong8007-utils;
          wrong8007 = pkgs.linuxPackages.callPackage ./nix/kernel-module.nix { };
        }
      );

      checks = forAllSystems (pkgs: {
        inherit (self.packages.${pkgs.stdenv.hostPlatform.system}) wrong8007 wrong8007-utils;
        nixos = pkgs.testers.runNixOSTest (import ./nix/test.nix self);
      });

      formatter = forAllSystems (pkgs: pkgs.nixfmt);
    };
}

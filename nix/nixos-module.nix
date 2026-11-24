{
  config,
  lib,
  pkgs,
  ...
}:

let
  cfg = config.services.wrong8007;
  inherit (lib)
    mkEnableOption
    mkIf
    mkOption
    types
    ;

  option =
    name: on: if on then "CONFIG_WRONG8007_${name}=y" else "# CONFIG_WRONG8007_${name} is not set";

  profile = pkgs.writeText "wrong8007-profile" ''
    CONFIG_WRONG8007=y
    ${option "KEYBOARD" cfg.triggers.keyboard}
    ${option "USB" cfg.triggers.usb}
    ${option "WHITELIST" cfg.triggers.whitelist}
    ${option "NETWORK" cfg.triggers.network}
    ${option "DEBUG" cfg.debug}
  '';

  module = config.boot.kernelPackages.callPackage ./kernel-module.nix { inherit profile; };
  utils = pkgs.callPackage ./utils.nix { };
in
{
  options.services.wrong8007 = {
    enable = mkEnableOption "Wrong Boot";

    triggers = {
      keyboard = mkOption {
        type = types.bool;
        default = true;
        description = "Compile in the keyboard phrase trigger.";
      };
      usb = mkOption {
        type = types.bool;
        default = true;
        description = "Compile in the USB trigger.";
      };
      whitelist = mkOption {
        type = types.bool;
        default = false;
        description = "Make whitelist mode the USB default when the whitelist parameter is absent.";
      };
      network = mkOption {
        type = types.bool;
        default = true;
        description = "Compile in the network trigger (magic packet and heartbeat).";
      };
    };

    debug = mkOption {
      type = types.bool;
      default = false;
      description = "Compile in verbose diagnostic logging.";
    };

    parameterFile = mkOption {
      type = types.str;
      default = "/etc/wrong8007/wrong8007.conf";
      description = ''
        Root-only file with the module parameters in modprobe.d syntax.
        It holds the trigger values and must stay out of the Nix store where
        every user can read it. Create it yourself or with a secrets manager.
      '';
    };

    afterNetwork = mkOption {
      type = types.bool;
      default = false;
      description = ''
        Load the module after network-online.target. Turn it on when you use
        the heartbeat. Its timeout starts counting at load.
      '';
    };
  };

  config = mkIf cfg.enable {
    assertions = [
      {
        assertion = cfg.triggers.keyboard || cfg.triggers.usb || cfg.triggers.network;
        message = "services.wrong8007 needs at least one trigger.";
      }
      {
        assertion = !cfg.triggers.whitelist || cfg.triggers.usb;
        message = "services.wrong8007.triggers.whitelist needs the USB trigger.";
      }
      {
        assertion = !lib.hasPrefix builtins.storeDir cfg.parameterFile;
        message = "services.wrong8007.parameterFile must stay out of the world-readable Nix store.";
      }
    ];

    boot.extraModulePackages = [ module ];
    environment.systemPackages = [ utils ];

    systemd.tmpfiles.rules = [ "d /etc/wrong8007 0700 root root -" ];

    systemd.services.wrong8007 = {
      description = "Wrong Boot dead man's switch";
      documentation = [ "https://raj.foundation/wrong8007/" ];
      wantedBy = [ "multi-user.target" ];
      wants = lib.optional cfg.afterNetwork "network-online.target";
      after = lib.optional cfg.afterNetwork "network-online.target";
      serviceConfig = {
        Type = "oneshot";
        RemainAfterExit = true;
        ExecStartPre = "${utils}/bin/wrong8007-params check wrong8007 ${cfg.parameterFile}";
        ExecStart = "${pkgs.kmod}/bin/modprobe -C ${cfg.parameterFile} wrong8007";
        ExecStop = "${pkgs.kmod}/bin/modprobe -r wrong8007";
      };
    };
  };
}

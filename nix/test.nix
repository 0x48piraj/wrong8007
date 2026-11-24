self:
{ pkgs, ... }:

let
  version = import ./version.nix { inherit (pkgs) lib; };
  payload = pkgs.writeShellScript "wb-payload" ''
    echo fired >> /run/wb-fired
  '';
  params = "options wrong8007 exec=${payload} match_port=4242";
  writeParams = "install -m 0600 /dev/null /etc/wrong8007/wrong8007.conf && echo '${params}' > /etc/wrong8007/wrong8007.conf";
in
{
  name = "wrong8007";

  nodes = {
    machine = {
      imports = [ self.nixosModules.default ];
      services.wrong8007.enable = true;
    };

    nonetwork = {
      imports = [ self.nixosModules.default ];
      services.wrong8007 = {
        enable = true;
        triggers.network = false;
      };
    };
  };

  testScript = ''
    start_all()

    with subtest("refuses to load without a parameter file"):
        machine.wait_for_unit("multi-user.target")
        machine.fail("systemctl is-active wrong8007.service")
        machine.succeed("test $(stat -c %a /etc/wrong8007) = 700")

    with subtest("loads with valid parameters"):
        machine.succeed("${writeParams}")
        machine.succeed("wrong8007-params check")
        machine.succeed("systemctl restart wrong8007.service")
        machine.succeed("grep -q '^wrong8007 ' /proc/modules")
        machine.succeed("test \"$(modinfo -F version wrong8007)\" = ${version}")

    with subtest("a matching packet runs the payload once"):
        machine.succeed("test ! -e /run/wb-fired")
        machine.succeed("echo x > /dev/udp/127.0.0.1/4242")
        machine.wait_until_succeeds("test \"$(wc -l < /run/wb-fired)\" = 1")
        machine.succeed("echo x > /dev/udp/127.0.0.1/4242")
        machine.sleep(1)
        machine.succeed("test \"$(wc -l < /run/wb-fired)\" = 1")

    with subtest("loads again at boot"):
        machine.shutdown()
        machine.start()
        machine.wait_for_unit("wrong8007.service")
        machine.succeed("grep -q '^wrong8007 ' /proc/modules")
        machine.succeed("test ! -e /run/wb-fired")

    with subtest("stopping the service unloads the module"):
        machine.succeed("systemctl stop wrong8007.service")
        machine.fail("grep -q '^wrong8007 ' /proc/modules")

    with subtest("a profile without the network trigger"):
        nonetwork.wait_for_unit("multi-user.target")
        nonetwork.succeed("modinfo -F parm wrong8007 | grep -q '^phrase:'")
        nonetwork.fail("modinfo -F parm wrong8007 | grep -q '^match_port:'")
        nonetwork.succeed("${writeParams}")
        nonetwork.fail("wrong8007-params check")
  '';
}

# The release version, read from dkms.conf
{ lib }:
lib.pipe ../dkms.conf [
  builtins.readFile
  (lib.splitString "\n")
  (map (builtins.match ''PACKAGE_VERSION="(.*)"''))
  (lib.findFirst (m: m != null) (throw "dkms.conf has no PACKAGE_VERSION"))
  builtins.head
]

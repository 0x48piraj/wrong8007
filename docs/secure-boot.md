# Secure Boot

With Secure Boot on, Debian, Ubuntu and Fedora kernels load only modules signed by a key the firmware trusts. Loading Wrong Boot then fails with:

```
modprobe: ERROR: could not insert 'wrong8007': Key was rejected by service
```

DKMS signs every module it builds with a Machine Owner Key (MOK). Enroll that key once and every later build loads, including the ones DKMS makes after a kernel upgrade.

Check whether Secure Boot is on:

```bash
mokutil --sb-state
```

## Debian

DKMS creates its key on the first build. Enroll it:

```bash
sudo mokutil --import /var/lib/dkms/mok.pub
```

`mokutil` asks for a one-time password. Reboot and the blue MOK Manager screen appears before the system starts:

1. Press a key within 10 seconds.
2. Choose **Enroll MOK**, then **Continue** and **Yes**.
3. Enter the one-time password.
4. Choose **Reboot**.

The module loads on the next boot. Confirm the enrollment:

```bash
mokutil --test-key /var/lib/dkms/mok.pub
```

## Ubuntu

DKMS on Ubuntu signs with the key of the `shim-signed` package. An interactive `apt` installation offers to enroll it and asks for the one-time password. Otherwise enroll it yourself:

```bash
sudo mokutil --import /var/lib/shim-signed/mok/MOK.der
```

Reboot and follow the MOK Manager steps under [Debian](#debian).

## Fedora

DKMS on Fedora creates `/var/lib/dkms/mok.pub` like Debian. Enroll it the same way.

## Arch Linux and NixOS

Their kernels load unsigned modules even with Secure Boot on, unless you enforce module signatures yourself (`module.sig_enforce=1` or `lockdown=integrity`). Sign the module with your own key in that case.

## Lost the MOK Manager screen?

`mokutil --import` only queues the key. The prompt appears on the next boot alone. Run the import again and reboot when you're at the machine.

## Summary

<!-- What this pull request changes for users, in one or two sentences. -->

## Type of change

- [ ] Bug fix
- [ ] New trigger or feature
- [ ] Breaking change (parameters, build profile or behavior of an existing trigger)
- [ ] Documentation
- [ ] Refactoring or cleanup

## Affected triggers

- [ ] Keyboard
- [ ] USB
- [ ] Network
- [ ] New trigger
- [ ] Core or tooling only

## Kernel versions tested

<!-- For example: 6.12.23, 7.2.11 -->

## Testing

- [ ] Every build profile this change touches builds without warnings (`make PROFILE=<file>`)
- [ ] The module loads with the change (`make load PARAMS=<file>`) and unloads cleanly (`make remove`)
- [ ] The trigger fires on the condition it should and stays quiet on the ones it shouldn't
- [ ] `dmesg` shows no new warnings and no trigger values
- [ ] New behavior has an automated test (CI or `nix/test.nix`)
- [ ] CI passes

### Evidence

<!--
The commands you ran and what they printed. Leave out real phrases, USB rules and addresses. For example:

make PROFILE=configs/defconfig
make load PARAMS=test.conf
sudo dmesg | tail -20
make remove
-->

## Checklist

- [ ] Code matches the existing style (kernel conventions, 4-space indentation)
- [ ] New parameters are validated in `init()` and checked by `wrong8007-params`
- [ ] README and `configs/wrong8007.conf.example` cover new or changed parameters
- [ ] No trigger values or payload path in the kernel log

## Breaking changes

<!-- What breaks and how users migrate. Delete this section if nothing breaks. -->

## Notes

<!-- Anything else a reviewer should know. -->

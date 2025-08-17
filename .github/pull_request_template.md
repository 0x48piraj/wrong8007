# Pull Request

## Summary
<!-- Provide a brief description of what this PR accomplishes -->
Example: Add heartbeat-based network trigger, upgrade USB whitelist management

## Changes Made
<!-- List the main changes in this PR -->
- Added support for heartbeat-based network trigger
- Refactored USB trigger parameter parsing
- Updated Makefile to support new trigger options
- Minor code cleanup in core module

## Type of Change
<!-- Mark the relevant option with an "x" -->
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (changes module behavior in existing triggers)
- [ ] Documentation
- [ ] Performance
- [ ] Code refactoring

## Kernel Versions Tested
<!-- List kernel versions this PR has been tested on -->
Example: 5.4.236, 6.1.47, 6.6.14

## Affected Triggers
<!-- Mark all triggers affected by this PR -->
- [ ] Keyboard Trigger
- [ ] USB Trigger
- [ ] Network Trigger
- [ ] Other / Custom

## Testing
<!-- Describe how you tested your changes -->
- [ ] Load/unload module with new trigger(s)
- [ ] Verified `/tmp/trigger_test.log` logs correct execution
- [ ] Ran `make test` / runtime smoke tests
- [ ] Nightly CI builds passed

### Evidence
<!-- Provide commands run and results -->
```bash
# Example: sudo insmod wrong8007.ko PHRASE="nuke" EXEC="/tmp/trigger_test.sh"
# Example: sudo rmmod wrong8007
# Example: sudo dmesg | tail -20
```

## Checklist

<!-- Mark completed items with an "x" -->

* [ ] The code follows Linux kernel coding style
* [ ] All new and existing tests pass locally
* [ ] The proposed changes generate no new warnings in `dmesg`
* [ ] Documentation for new or changed triggers
* [ ] No regressions in existing trigger functionality

## Breaking

<!-- If this PR introduces breaking changes, describe them here -->

<!-- Include migration steps if applicable -->

## Notes

<!-- Any additional information reviewers should know -->

<!-- Include screenshots, performance metrics, dependencies, etc. -->

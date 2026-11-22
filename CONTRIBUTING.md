# Contributing

Thank you for looking into contributing!

Read these first:

* [Development guide](docs/development.md) for the architecture, the trigger contract and how to build a new trigger.
* [Security model](docs/security-model.md) for what Wrong Boot guarantees and what it leaves out.
* [Design philosophy](docs/manifesto.md) for the reasoning behind both.

## Before you open a pull request

* Build every trigger profile your change touches. CI builds each profile against kernels from 5.4 on with warnings as errors.
* Load and unload the module with your change and check `dmesg` for warnings.
* Keep trigger values and the payload path out of the kernel log.
* Follow the Linux kernel coding style.
* Write commit messages in the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) form.

## Security issues

Report vulnerabilities privately as [SECURITY.md](SECURITY.md) describes.

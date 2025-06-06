# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.12.0-2]

This release builds with the Linux distribution kernels shipped with:

- RHEL 8.2 to 8.10, and 9.0 to 9.5.
- Fedora 40 and 41.
- Ubuntu 20.04, 22.04, and 24.04.

### Added

- backport: constify call to device_find_child() with Linux 6.14 ([ee6c4df](https://github.com/OFS/linux-dfl-backport/commit/ee6c4dffc34e7e7217a0395c0bd07e0c590316b9)).
- Updating SECURITY.md ([4d08cb4](https://github.com/OFS/linux-dfl-backport/commit/4d08cb488f6855d7633755169c9171d32ca7bf6b)).

[1.12.0-2]: https://github.com/OFS/linux-dfl-backport/compare/intel-1.12.0-1...intel-1.12.0-2

## [1.12.0-1]

This release builds with the Linux distribution kernels shipped with:

- RHEL 8.2 to 8.10, and 9.0 to 9.5.
- Fedora 40 and 41.
- Ubuntu 20.04, 22.04, and 24.04.

### Fixed

- Fix regression querying platform device in `power_hwmon_write()` ([a98607b](https://github.com/OFS/linux-dfl-backport/commit/a98607b9ba9fa12e82e7d1696dd838700dd53a90)).
- Fix warnings about missing prototypes in `qsfp-mem-core` driver ([9d84f4d](https://github.com/OFS/linux-dfl-backport/commit/9d84f4d9d6832915c71e4e71c3b66470bead023b)).
- Consistently use CVL in MAX 10 BMC sensor labels ([cb86311](https://github.com/OFS/linux-dfl-backport/commit/cb863118d5997fc964109fbf728eb42767cb0179)).
- Fix missing `free()` of type in `binfo_create_feature_dev_data()` ([7035881](https://github.com/OFS/linux-dfl-backport/commit/7035881a7660c01f21c92afd5384aa36b42ef59e)).
- Fix dfl MODULE_DEVICE_TABLE built on big-endian host ([6e26c62](https://github.com/OFS/linux-dfl-backport/commit/6e26c623e1a9b765d72cb3c66a03186dc84ba7a3)).
- Fix leaking device id for released ports ([7f68320](https://github.com/OFS/linux-dfl-backport/commit/7f68320df3675c7e55fc081ff48be29297e70996)).

[1.12.0-1]: https://github.com/OFS/linux-dfl-backport/compare/intel-1.11.0-2...intel-1.12.0-1

## [1.11.0-2]

### Added

- Document parent/child AFU port relationship ([55a4090](https://github.com/OFS/linux-dfl-backport/commit/55a40900aaafa88f43f5c5f5b65f3dc747e4c67e)).

### Fixed

- Fix automatic loading of 8250_dfl driver ([c955739](https://github.com/OFS/linux-dfl-backport/commit/c955739bb74695d75bdf3eee6dd1a8ab438c865d)).

[1.11.0-2]: https://github.com/OFS/linux-dfl-backport/compare/intel-1.11.0-1...intel-1.11.0-2

## [1.11.0-1]

This release builds with the Linux distribution kernels shipped with:

- RHEL 8.2 to 8.10.
- Fedora 39 and 40.
- Ubuntu 20.04 and 22.04.

### Added

- Add support for CMC card ([be7ca0b](https://github.com/OFS/linux-dfl-backport/commit/be7ca0beda444111e788939561f48ff7c371e504), [97cea2b](https://github.com/OFS/linux-dfl-backport/commit/97cea2bad40199ec1b54e2c790d82468007a7fb2), [d5b4999](https://github.com/OFS/linux-dfl-backport/commit/d5b4999d57687c26fa1d7541b36c979b24717f33), [93b79e4](https://github.com/OFS/linux-dfl-backport/commit/93b79e416032365ba8cb5f6a0e2ef38cab02a89d), [2b94c9e](https://github.com/OFS/linux-dfl-backport/commit/2b94c9eb05119c6cf61d61feca1b114b58843941), [1f5515f](https://github.com/OFS/linux-dfl-backport/commit/1f5515fed63da527584873b3a96d45b1b84f6eec), [3ca4b96](https://github.com/OFS/linux-dfl-backport/commit/3ca4b96e5d1bdee769380ee94703d9dbf825a061)).
- Add support for binding a PASID to an FPGA port ([e0dec9d](https://github.com/OFS/linux-dfl-backport/commit/e0dec9d4a286c28ecbd77185df036edb54f11827)).
- Add CXL Cache IP block driver ([2ab241a](https://github.com/OFS/linux-dfl-backport/commit/2ab241a3b6fa2b893ca8585c540e96bea6bf2330), [c41b7d8](https://github.com/OFS/linux-dfl-backport/commit/c41b7d8c157b87e2195f86970fd43a23498e5614), [cf1af08](https://github.com/OFS/linux-dfl-backport/commit/cf1af08e2c427129eb1603110b987512242f3156), [7f328e1](https://github.com/OFS/linux-dfl-backport/commit/7f328e171970edd472050f58c3aec619b525e735), [1b9bd02](https://github.com/OFS/linux-dfl-backport/commit/1b9bd027ad515133f9c2660825b54833e639c3dd), [75ddbd8](https://github.com/OFS/linux-dfl-backport/commit/75ddbd81191faa5c797eea618954631941e1e61f), [cb19826](https://github.com/OFS/linux-dfl-backport/commit/cb19826b72fb10a315f79469a7f6bba9d98dddfe)).
- Add platform driver for DFL device tree ([6a5e47c](https://github.com/OFS/linux-dfl-backport/commit/6a5e47cf83a2900482554a14d238f8d648a17645), [47c9c30](https://github.com/OFS/linux-dfl-backport/commit/47c9c30a43909c02539230d8d74d5f220202f51c)).
- Add support for DFH Private feature type ([8197b70](https://github.com/OFS/linux-dfl-backport/commit/8197b705ac482e5879b5413405c6407e9a5b8b56), [c392888](https://github.com/OFS/linux-dfl-backport/commit/c3928880e991cc4a05704818e9588eddb744a07f), [5afb903](https://github.com/OFS/linux-dfl-backport/commit/5afb90359afc8b24e0f8c98b866eb3726937a82c), [51e675c](https://github.com/OFS/linux-dfl-backport/commit/51e675caca3de1de3759123b86b562e577b05fd9)).
- Add support for Revision 2 of the DFL Port feature ([365fcc6](https://github.com/OFS/linux-dfl-backport/commit/365fcc64d9bd0801272154a29800e121e07f9035), [1c82071](https://github.com/OFS/linux-dfl-backport/commit/1c820711015fc23dfdf77ce229015f2823bd601d)).

### Fixed

- Fix handling of CSR-relative addresses ([c6fb0fb](https://github.com/OFS/linux-dfl-backport/commit/c6fb0fbab07d15a7b89686a34ce5119fa4e65eff)).
- Fix resource claim failure ([605182a](https://github.com/OFS/linux-dfl-backport/commit/605182ab8675fb31dc69576f98f114c49c00e9be)).
- Handle NACK from QSFP module ([cb1a638](https://github.com/OFS/linux-dfl-backport/commit/cb1a638ed47863100cb0d27aca61cf1b43d1dd83)).
- Fix multiplier for N6000 board power sensor ([2fd7bfa](https://github.com/OFS/linux-dfl-backport/commit/2fd7bfa558d257dd90a9f58355c5bc6326dcc2fd)).
- Fix excessive QSFP cable polling ([5e592f2](https://github.com/OFS/linux-dfl-backport/commit/5e592f26c2641eacba29cace196541c7ed984ef2), [3a12784](https://github.com/OFS/linux-dfl-backport/commit/3a12784821b1829e25d92ac6fae9eee537aae85b)).
- Add PCI subdevice ID for Intel D5005 card ([68adaf0](https://github.com/OFS/linux-dfl-backport/commit/68adaf05a42bfa22e33501269fbc1e35f44cbf80)).

[1.11.0-1]: https://github.com/OFS/linux-dfl-backport/compare/intel-1.10.0-1...intel-1.11.0-1

# Changelog

All Notable changes to `handlebars.c` will be documented in this file.

Updates should follow the [Keep a CHANGELOG](http://keepachangelog.com/) principles.

## [1.0.0]

### Changed
- *Various improvements and cleanup*
- Updated handlebars-spec to v4.7.7
- The executable and the test suite are now licensed under the AGPLv3 or later. The
  library remains licensed under the LGPLv2.1 or later.

### Fixed
- Segmentation fault when attempting to use unimplemented inline partials in the VM
- Empty raw block no longer has a parse error
- Access of uninitialized memory in partials related to indentation

### Added
- Partial blocks support
- Improved mustache compatibility

## [0.7.3] - 2020-12-06

### Fixed
- `-Wformat-security` failures (@remicollet)
- Link issues on NixOS 20.09

## [0.7.2] - 2020-04-21

### Fixed
- Determinism issue with `handlebars_module_normalize_pointers`

## [0.7.1] - 2020-04-18

### Added
- `handlebars_module_normalize_pointers` that is the inverse of `handlebars_module_patch_pointers`

### Fixed
- Test failures with certain hardening options
- Various compiler warnings

## [0.7.0] - 2020-04-17

### Added
- Mustache-style lambda support
- The executable now supports loading partials from files via the new options
`--partial-loader`, `--partial-path=DIR`, and `--partial-ext=EXT`.

### Changed
- The executable's default mode is now `--execute`. Compiler flags are now
specified through a single `--flags=FLAGS` option, and there is an improved `--help` message.

## [0.6.4] - 2017-07-17

### Added
- Mustache delimiter preprocessing support - implementing libraries will need to run `handlebars_preprocess_delimiters`
function to receive a new template with converted delimiters

### Fixed
- Segmentation fault when lookup built-in used with a non-string parameter

## [0.6.3] - 2017-06-07

### Fixed
- Test when lmdb is not available

## [0.6.2] - 2017-06-04

### Fixed
- Test failure on 32-bit systems

## [0.6.1] - 2017-05-31

### Fixed
- memcpy causes SIGILL in `handlebars_str_reduce` on alpine linux

## [0.6.0] - 2016-11-07

### Added
- `strict` and `assumeObjects` flags
- `handlebars_cache_reset()` to completely flush the cache

## [0.5.2] - 2016-08-12

### Fixed
- `MAP_ANONYMOUS` undefined on OS X, called `MAP_ANON`
- pthread spinlocks unavailable on OS X, use regular mutex

## [0.5.1] - 2016-05-05

### Changed
- Changed the license from `LGPLv3` to `LGPLv2.1 or later`

[Unreleased]: https://github.com/jbboehr/handlebars.c/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/jbboehr/handlebars.c/compare/v0.7.3...v1.0.0
[0.7.3]: https://github.com/jbboehr/handlebars.c/compare/v0.7.2...v0.7.3
[0.7.2]: https://github.com/jbboehr/handlebars.c/compare/v0.7.1...v0.7.2
[0.7.1]: https://github.com/jbboehr/handlebars.c/compare/v0.7.0...v0.7.1
[0.7.0]: https://github.com/jbboehr/handlebars.c/compare/v0.6.4...v0.7.0
[0.6.4]: https://github.com/jbboehr/handlebars.c/compare/v0.6.3...v0.6.4
[0.6.3]: https://github.com/jbboehr/handlebars.c/compare/v0.6.2...v0.6.3
[0.6.2]: https://github.com/jbboehr/handlebars.c/compare/v0.6.1...v0.6.2
[0.6.1]: https://github.com/jbboehr/handlebars.c/compare/v0.6.0...v0.6.1
[0.6.0]: https://github.com/jbboehr/handlebars.c/compare/v0.5.2...v0.6.0
[0.5.2]: https://github.com/jbboehr/handlebars.c/compare/v0.5.1...v0.5.2
[0.5.1]: https://github.com/jbboehr/handlebars.c/compare/v0.5.0...v0.5.1

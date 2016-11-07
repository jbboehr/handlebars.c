# Changelog

All Notable changes to `handlebars.c` will be documented in this file.

Updates should follow the [Keep a CHANGELOG](http://keepachangelog.com/) principles.

## [Unreleased]

### Added
- `handlebars_cache_reset()` to completely flush the cache

## [0.5.2] - 2016-08-12

### Fixed
- `MAP_ANONYMOUS` undefined on OS X, called `MAP_ANON`
- pthread spinlocks unavailable on OS X, use regular mutex

## [0.5.1] - 2016-05-05

### Changed
- Changed the license from `LGPLv3` to `LGPLv2.1 or later`

[Unreleased]: https://github.com/jbboehr/handlebars.c/compare/v0.5.2...HEAD
[0.5.2]: https://github.com/jbboehr/handlebars.c/compare/v0.5.1...v0.5.2
[0.5.1]: https://github.com/jbboehr/handlebars.c/compare/v0.5.0...v0.5.1

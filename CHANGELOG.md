# Changelog

All Notable changes to `handlebars.c` will be documented in this file.

Updates should follow the [Keep a CHANGELOG](http://keepachangelog.com/) principles.

## [Unreleased]

### Added
- Core construction, parsing, compilation, serialization, and rendering now
  have `_try` entry points that return errors without allowing the library's
  internal `longjmp` handling to escape the call.
- Optional JSON and YAML conversions now have transactional `_try` entry points
  that preserve the destination value on failure.
- Cache constructors and operations now have `_try` entry points that report
  errors without allowing library `longjmp` handling to escape the call.
- Filesystem partial-loader construction and lookup now have transactional
  `_try` entry points that preserve caller-owned output values on failure.
- `handlebarsc` can register bounded plain-text or structured-JSON helpers
  backed by external executables.
- String and array values expose an emulated `length` property during template
  lookup.
- Native array/map mutations and typed-pointer retrieval now have checked entry
  points for callers that need recoverable type and operation failures.
- Maps now have a closeable iterator API that keeps their backing vector stable
  and participates in library error unwinding.

### Changed
- Documented the talloc and reference-count lifetime model, including uniform
  cleanup for value conversions and VM render results.
- Documented VM setter ownership and made passing NULL to the cache setter an
  explicit way to disable caching.
- Documented that every cache hit must be paired with a cache release, along
  with the mmap backend's existing reset behavior while hits remain active.
- Value iterators now retain their backing storage, support nested map
  iteration, and are closed when an error unwinds through the library.
- The legacy map foreach macro now uses the closeable map iterator, so library
  error unwinds and, on supported compilers, early lexical exits no longer
  leave the map locked against rehashing. Its existing in-place mutation
  behavior is preserved for shared maps; mutation is rejected only when a
  value-iterator snapshot of the same map is simultaneously active.
- Value iterators must now be declared with
  `HANDLEBARS_VALUE_ITERATOR_DECL` and initialized with
  `HANDLEBARS_VALUE_ITERATOR_INIT`; the unsafe public initializer that assumed
  hidden trailing storage was removed.
- `hbs_str_val()` now returns a read-only byte buffer. Callers that modified
  string storage directly must use the public string mutation APIs so
  copy-on-write state and cached hashes remain valid. String trimming now
  invalidates cached hashes whenever it changes the string.
- The public value iterator layout and initialization contract changed, and the
  shared-library ABI is now version 10. Downstream binaries must be rebuilt.
- Serialized programs now retain block-parameter counts, extending the public
  module-table and `handlebars_options` layouts.
- VM rendering now transfers filesystem partial-loader errors from the loader
  context to the VM context.

### Fixed
- Stable installed headers now compile independently as C and C++; generated
  Flex/Bison headers remain unchanged.
- JSON `null` elements no longer reuse the preceding iterator value.
- Recursive value conversion, expression rendering, and diagnostic dumping now
  reject cyclic and excessively deep container graphs.
- Block parameters preserve lexical precedence through nested helpers and
  partial blocks.
- Subexpressions reject truthy context values that are not callable.
- LMDB cache statistics report the correct backend name.
- Failed map copy-on-write mutations now release unpublished replacement maps
  and temporary string keys while preserving the original map.
- Cached runtime templates are released through the cache that produced them
  when a helper changes the VM's cache during rendering.
- Runtime-template cache errors now remain inside VM `_try` boundaries when
  the borrowed cache has an independent error context.

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

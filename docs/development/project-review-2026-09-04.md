# Project review: 2026-09-04

Reviewed commit: 7d256818b75e925484f85f31ccd8c4cdd9a8ab64.

This report covers the library, CLI, build and installation workflows, tests, fuzz harnesses, documentation, and bundled C dependencies. The findings and baseline verification describe the reviewed commit and have experimental evidence. The review itself changed no production code or repository tests; subsequent fixes are recorded in status notes.

P1 means fix before the next release because the defect affects packaging or a substantial runtime contract. P2 means a correctness or ownership defect in ordinary use. P3 means a narrower API, diagnostic, test, or maintenance issue. These are remediation priorities, not vulnerability severity ratings.

R01 through R04 are addressed. The remaining priorities include optional JSON and YAML dependencies in CMake, inline-partial handling, cache compilation settings, and the test-runner gaps.

## Verification and coverage limits

| Check | Observed result |
| --- | --- |
| Existing Autotools configuration, make -j4 all check | 2,266 checks passed, no failures or skips |
| Fresh GCC build with AddressSanitizer, UndefinedBehaviorSanitizer, and allocation-failure testing | 3,529 checks passed, no sanitizer diagnostics |
| Fresh CMake Release build and CTest | All 26 test programs passed |
| CMake Debug CLI build | Built successfully. A normal inline-partial module-print operation then hit an assertion |
| Installed CMake headers and exported-target consumer | Both focused consumer checks failed, as detailed below |
| CMake with LMDB unavailable and tests disabled | Configuration failed despite LMDB being optional |
| distcheck from the allocation-failure-enabled build | Failed linking the newly configured distribution build |
| Distribution control omitting the generated public configuration header | Fresh configuration and build succeeded |
| Normal-input CLI, C API, allocation-count, dependency, and test-oracle probes | Results recorded under the relevant findings |

The sanitizer build used GCC 15.2.0, -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined, CK_FORK=no, ASAN_OPTIONS=detect_leaks=1:halt_on_error=1, and UBSAN_OPTIONS=halt_on_error=1. It enabled HANDLEBARS_MEMORY and disabled hardening, Valgrind, and source-generator regeneration. Build and test targets were run sequentially. CMake was 4.1.6. The environment supplied Check 0.15.2, json-c 0.18, libyaml 0.2.5, LMDB 0.9.35, and talloc 2.4.4.

The source review used 14 location assignments and two final cross-cutting sweeps. Enumeration found 120 C/C++ files, 2,117 units, and 73,104 lines, with no unreadable files or explicit scope exclusions reported. This includes generated code, vendored headers, and a local generated CMake compiler-identification file. Build scripts, grammar sources, CI configuration, and documentation were inspected separately.

**Coverage remains unverified.** The parser reported degraded parses in 102 files. All assignments produced artifacts, but the ledger gate accepted 4,443 of 4,497 required checks and rejected 63 accounting violations. Twenty-nine sweep rows did not match enumerated unit IDs. Another 240 units, containing 16,104 lines, had no generated questions and therefore cannot be assessed by that gate. These counts are not a percentage of proven-correct code. The ledger is only a consistency check, and reviewer shell access was not technically restricted.

No separate false-positive judging pass ran over the raw C-review artifacts. Those artifacts include source-only candidates and automatically promoted pointers, so their finding count must not be read as the number of confirmed defects. This report reconciles the candidates against the experiments below.

No new fuzzing campaign, ThreadSanitizer run, 32-bit build, Windows/macOS run, or no-refcount build was performed. No exploitable memory-corruption defect was experimentally established. Successful sanitizer runs do not establish safety for untested inputs or platforms.

The upstream runtime runner also deliberately excludes 11 runtime cases and 40 AST-inapplicable cases. Its passing result does not imply complete Handlebars compatibility.

## Build and distribution

### R01. P1: release archives contain configuration from the machine that created them

Sources: [src/Makefile.am](../../src/Makefile.am), [src/handlebars.h:35](../../src/handlebars.h#L35).

At the reviewed commit, the generated src/handlebars_config.h was listed among distributed public headers. A distribution created with allocation-failure testing enabled therefore contained HANDLEBARS_MEMORY=1. A subsequent out-of-tree configuration could disable that feature, but the quoted include in handlebars.h found the archived source-side header before the new build-side header.

Experiment:

- Created the archive from the sanitizer/allocation-failure build.
- The archived header defined HANDLEBARS_MEMORY=1.
- The distribution's newly generated header and config.h both left it undefined.
- distcheck failed with unresolved _handlebars_talloc_* and handlebars_memory_fail_counter_incr symbols.
- Extracted the same archive into a separate control directory, omitting only its generated public configuration header. Configuration with memory testing disabled and make -j4 all then succeeded.

This can make a release depend on the maintainer's build settings. ABI-sensitive configuration switches deserve particular care, although cross-ABI memory behavior was not tested.

**Status: addressed.** [src/Makefile.am:77](../../src/Makefile.am#L77) now lists the generated header in nodist_pkginclude_HEADERS, and src/Makefile.in has been regenerated. The header remains installed under include/handlebars, while release archives retain only its configuration template. The Linux CI job enables allocation-failure testing for archive creation and explicitly disables it for distcheck.

The regression failed before the fix with unresolved allocation-wrapper symbols and passed afterward. The producer build passed 3,529 checks; distcheck passed 2,266 checks plus installation and cleanup checks. An external consumer compiled and ran against the installed headers and library. These checks ran locally with GCC; the full platform/compiler CI matrix was not run locally.

### R02. P1: CMake publishes incorrect version and feature information

Sources: [CMakeLists.txt](../../CMakeLists.txt), [cmake/handlebars_config.h.in](../../cmake/handlebars_config.h.in), [configure.ac:20](../../configure.ac#L20).

At the reviewed commit, CMake reported version 0.7.2 and specification version 4.7.6, while Autotools reported 1.0.0 and 4.7.7. CMake also enabled features through compiler definitions without setting the variables used by the installed header's configuration directives.

The fresh CMake CLI reported JSON and YAML support enabled, but its installed header left the JSON, YAML, LMDB, pthread, PCRE, and testing-export macros undefined. An installed-header consumer calling handlebars_cache_lmdb_ctor failed compilation with an implicit-function-declaration error even though the library was built with LMDB.

The CMake tests inherit build-wide definitions, which explains why all 26 programs can pass while the installed interface is wrong.

**Status: addressed.** CMake now declares the correct release and specification versions explicitly. Releases must update these constants and the installed-consumer expectations alongside configure.ac. The compiler definitions and installed header use the same resolved feature settings. Enabled feature macros have the value 1, and the header includes both specification-version strings.

The new [installed consumer test](../../tests/installed_config/main.c) failed before the fix because feature macros, specification versions, and cache declarations were missing. It now compiles and runs using a temporary installation. Local GCC verification passed all 27 Release CTest programs, all 28 programs with allocation-failure testing enabled and no explicit build type, and all 28 programs with AddressSanitizer and UndefinedBehaviorSanitizer. All three CMake configurations also built and passed their tests with configure.ac absent from an isolated source copy. The existing Autotools suite passed 2,266 checks. A separate tests-disabled installation correctly omitted the PCRE feature macro.

These checks cover the public configuration header; R03's exported-target requirements and R04's missing optional dependencies remain separate. Windows, macOS, and the full platform/compiler matrix were not tested locally.

Independent correctness and test reviews of the feature and header changes found no actionable defects. Additional checks passed from paths containing spaces and after toggling allocation-failure testing off, on, and off in the same build. The consumer also checks that enabled feature macros equal 1 and that a memory-enabled installation links its allocation-failure implementation. Older CMake versions and multi-config generators remain untested.

### R03. P2: the exported CMake target lacks its public include requirements

Sources: [src/CMakeLists.txt](../../src/CMakeLists.txt), [CMakeLists.txt](../../CMakeLists.txt), [tests/CMakeLists.txt](../../tests/CMakeLists.txt).

At the reviewed commit, a consumer included the installed handlebars.cmake export and linked its executable to the handlebars target. Configuration succeeded, but compiling a source file containing #include <handlebars.h> failed because the target exported no include directory.

The generated interface also carried test dependencies such as Check and PCRE into consumer link requirements when tests were enabled. That dependency leakage was observed in the installed export; failure on a machine without those libraries was not separately tested during the original review.

**Status: addressed.** Both exported targets now publish their build and installation header directories and the talloc include directory required by handlebars_memory.h. Required runtime libraries remain transitive. Check and PCRE include paths and link libraries are scoped to the tests.

The consumer first failed because handlebars.h was missing from its include path. After the include fix, a separate check rejected Check in the imported target's link interface. The corrected exports passed both checks for shared and static consumers. Independent consumer builds also passed with ambient dependency paths removed from the environment, including probes that link JSON, LMDB, YAML, and allocation functions.

The existing installed-header probe is retained. New [target probes](../../tests/installed_config/target.c) compile and run using only each imported target's usage requirements. A temporary mutation removing json-c from the static target's link requirements failed with unresolved JSON symbols, confirming the probe exercises transitive linkage.

Absolute installation directories require care in the test harness: CMake records those fixed destinations in its export, so the export cannot be loaded directly from a DESTDIR staging tree. The harness keeps the header probe for all installation layouts and adds the imported-target probes when include and library destinations are relative. Absolute include-only, library-only, and combined cases passed without creating the configured destination. Both targets also passed a separate direct-install check at configured absolute paths under the temporary test directory.

Fresh GCC verification passed 27 Release CTest programs, 28 with allocation-failure testing enabled and no explicit build type, and 28 with AddressSanitizer and UndefinedBehaviorSanitizer. All three consumers passed against a tests-disabled installation. Both build-tree target consumers passed, and the existing Autotools suite passed 2,266 checks. Independent review identified the absolute-path staging issue described above; the corrected harness passed its focused regressions. Runtime dependency paths still come from configure-time discovery; rediscovering dependencies on another machine is outside this slice. Older CMake versions, Windows, macOS, and multi-config generators were not tested locally.

### R04. P2: missing optional LMDB prevents CMake configuration

Sources: [CMakeLists.txt](../../CMakeLists.txt), [src/CMakeLists.txt](../../src/CMakeLists.txt), [fuzz/CMakeLists.txt](../../fuzz/CMakeLists.txt), [cmake/FindLMDB.cmake](../../cmake/FindLMDB.cmake).

At the reviewed commit, with only the LMDB include and library directories excluded from discovery and HANDLEBARS_ENABLE_TESTS=OFF, configuration failed because LMDB_INCLUDE_DIR-NOTFOUND and LMDB_LIBRARIES-NOTFOUND were added to targets unconditionally.

**Status: addressed.** LMDB include paths, link libraries, feature metadata, backend source selection, and the LMDB fuzz target now depend on successful discovery of both its header and library. The installed-header test also uses the discovery result, so finding a library without its header does not falsely imply that LMDB support is enabled.

The new handlebars-c-cmake-no-lmdb configuration in [flake.nix](../../flake.nix) omits LMDB from its build dependencies and runs the existing CMake suite. It is included in the generated CI matrix. Running nix build .#handlebars-c-cmake-no-lmdb --no-link first reproduced the configuration error. Guarding the paths exposed a second failure: CMake still compiled handlebars_cache_lmdb.c without LMDB's types. Conditional source selection, matching Autotools, fixed that build failure as well. The same Nix check then passed all 27 CTest programs and installed the package.

Local GCC verification passed all 27 Release CTest programs with LMDB available, all 28 programs with allocation-failure testing enabled and LMDB absent, and all 28 under AddressSanitizer and UndefinedBehaviorSanitizer with LMDB absent. A tests-disabled build without LMDB also passed the three installed-header and shared/static target consumers. Separate configurations with only the header absent or only the library absent passed builds and the cache, public-header, and installed-consumer checks. Disabling LMDB discovery with cached paths present, then re-enabling it in the same build directory, passed those checks and updated the feature macro correctly. Other platforms and older CMake versions were not tested locally.

Review identified a missed consumer of the discovery result: the LMDB fuzz target still depended on the cached library path. A Clang build with fuzzing enabled succeeded initially, then failed to compile fuzz_lmdb after disabling LMDB discovery because the backend constructor was no longer declared. Gating that target on LMDB_FOUND fixed the regression. The [fuzz workflow](../../.github/workflows/fuzz.yml) now checks CMake builds with discovery enabled, disabled, and re-enabled in the same directory. This sequence passed locally with Clang 21.1.8; the workflow uses Clang 18. No fuzz inputs were needed to reproduce the build failure.

Related follow-up: separately excluding json-c or libyaml reproduced analogous NOTFOUND configuration errors. Their source files and tests are also selected unconditionally. Making those dependencies optional requires its own slice; this change addresses LMDB only.

### R05. P3: Autotools release archives omit CMake support

Sources: [Makefile.am:33](../../Makefile.am#L33), the root and subdirectory CMakeLists.txt files.

The generated handlebars-1.0.0.tar.gz contained 383 members and no CMakeLists.txt or cmake/ files. Consequently, the repository's CMake workflow cannot be used from that source archive.

Include the CMake build definitions and helper modules in the distribution, and configure CMake against an extracted release archive in CI.

## Rendering, compilation, and parsing

Unless stated otherwise, rendering experiments used the shown template on stdin, a JSON data file, and handlebarsc --no-newline. All examples use ordinary template/data inputs. Handlebars' expected conditional, inverse, and context behavior is described in its [built-in helper documentation](https://handlebarsjs.com/guide/builtin-helpers.html).

### R06. P1: an expression before an inline partial can change the partial's identity

Sources: [src/handlebars_opcode_serializer.c:681](../../src/handlebars_opcode_serializer.c#L681), [src/handlebars_vm.c:1251](../../src/handlebars_vm.c#L1251), [src/handlebars_vm.c:1447](../../src/handlebars_vm.c#L1447).

This template rendered P:

~~~handlebars
{{#*inline "p"}}P{{/inline}}{{>p}}
~~~

Adding an ordinary preceding conditional caused rendering to fail with “The partial p could not be found”:

~~~handlebars
prefix{{#if true}}T{{/if}}{{#*inline "p"}}P{{/inline}}{{>p}}
~~~

The expected output is prefixTP. The serializer scans forward from an earlier literal-producing opcode to a later inline registration and treats the entire range as one declaration. The VM repeats that inference and can derive the declaration name from the earlier expression.

Represent an inline declaration's boundaries explicitly, or validate its exact opcode structure. Keep serializer, verifier, and VM recognition consistent. Add a regression with ordinary expressions before, between, and after inline declarations.

### R07. P2: printing a valid inline-partial module aborts in a debug build

Sources: [src/handlebars_opcodes.c:317](../../src/handlebars_opcodes.c#L317), [src/handlebars_opcode_serializer.c:620](../../src/handlebars_opcode_serializer.c#L620), [src/handlebars_opcode_printer.c:144](../../src/handlebars_opcode_printer.c#L144).

Serializing an inline declaration sets a third operand on registerDecorator, but its operand-count metadata still says two. Running --module on the simple inline template from R06 in the fresh CMake Debug build terminated with SIGABRT at the printer's assertion that operand three is null.

The Release build completed but omitted that operand from the printed representation.

Update the opcode metadata and its consumers together. Test serialized module printing with assertions enabled. A shared opcode-description table would reduce drift between operand counts, printers, and name mappings.

### R08. P1: cached partials reuse bytecode compiled with different flags

Source: [src/handlebars_vm.c:919](../../src/handlebars_vm.c#L919).

The runtime cache uses template text as its key, although compilation also depends on VM flags.

A C API probe rendered a partial containing {{value}} with the ordinary value “a & b”:

| Step | Configuration | Actual output |
| --- | --- | --- |
| 1 | Enable no_escape and populate cache | a & b |
| 2 | Disable no_escape, retain cache | a & b |
| 3 | Keep no_escape disabled, reset cache | a &amp; b |

Thus changing compilation settings does not reliably change rendering when a cache entry already exists. A strict-mode comparison did behave correctly and is not evidence for a strict-mode failure.

Include compilation-affecting settings in cache identity, or explicitly enforce one immutable configuration per cache. Test shared-cache use across different VM configurations. Any impact on an application's handling of untrusted content depends on that application's configuration and was not tested.

### R09. P2: some numeric literals reach helpers as strings

Sources: [src/handlebars_compiler.c:1404](../../src/handlebars_compiler.c#L1404), [src/handlebars_vm.c:2747](../../src/handlebars_vm.c#L2747).

| Template | Actual | Expected |
| --- | --- | --- |
| {{#if 0}}T{{else}}F{{/if}} | F | F |
| {{#if 0.0}}T{{else}}F{{/if}} | T | F |
| {{#if -0}}T{{else}}F{{/if}} | T | F |

Some numeric spellings are stored as string operands and pushed into the VM as string values. This changes helper-visible types and truthiness.

Preserve numeric type information through compilation and execution. Restore meaningful type assertions in the ported helper fixtures, including the disabled checks in tests/fixtures.c, and cover equivalent zero spellings.

### R10. P2: conditional helpers mishandle includeZero

Source: [src/handlebars_helpers.c:446](../../src/handlebars_helpers.c#L446), [src/handlebars_helpers.c:499](../../src/handlebars_helpers.c#L499).

With data {"n":0}:

| Template | Actual | Expected |
| --- | --- | --- |
| {{#if n includeZero=false}}yes{{else}}no{{/if}} | yes | no |
| {{#if n includeZero=true}}yes{{else}}no{{/if}} | yes | yes |
| {{#unless n includeZero=true}}yes{{else}}no{{/unless}} | yes | no |

The if implementation tests whether the option exists rather than its value. Unless converts its argument to a boolean before delegating, losing the zero-specific behavior.

Evaluate the option's value and implement unless by reversing the branch selection of the same conditional evaluation. Callable-argument behavior was a source-review concern but was not experimentally checked here.

### R11. P2: with uses the wrong empty-value and inverse-context rules

Source: [src/handlebars_helpers.c:568](../../src/handlebars_helpers.c#L568).

The template {{#with n}}yes{{else}}no{{/with}} rendered yes for n=false, n="", and n=[].

For {"n":null,"name":"outer"}, this template rendered fallback= instead of fallback=outer:

~~~handlebars
{{#with n}}{{name}}{{else}}fallback={{name}}{{/with}}
~~~

Only null selects the inverse branch, and that branch executes with the null value rather than the original scope.

Use the intended emptiness predicate and preserve the caller's scope for the inverse branch. Add tests for each empty type and for an inverse that actually reads its context.

### R12. P2: root and parent data references resolve to the current frame

Source: [src/handlebars_vm.c:2564](../../src/handlebars_vm.c#L2564).

Two independent normal-input checks failed:

- With {"name":"ROOT","child":{"name":"CHILD"}}, {{#with child}}{{@root.name}}{{/with}} rendered CHILD instead of ROOT.
- With {"outer":[[1,2],[3]]}, nested iteration rendered the wrong parent indices:

~~~handlebars
{{#each outer}}{{#each this}}[{{@../index}},{{@index}}]{{/each}}{{/each}}
~~~

Actual: `[0,0][1,1][0,0]`. Expected: `[0,0][0,1][1,0]`.

The fallback for root uses the current context-stack top. Parent-data traversal can reuse the current metadata when the parent link is missing.

Establish an explicit root reference and parent-linked data frames, with tests where parent and child values differ. Repeated equal values would mask these errors.

### R13. P2: partial blocks bypass whitespace processing

Source: [src/handlebars_whitespace.c:520](../../src/handlebars_whitespace.c#L520).

The partial-block case is ignored by the whitespace visitor. This fallback template retained both standalone tag lines as blank lines:

~~~handlebars
A
{{#> missing}}
B
{{/missing}}
C
~~~

Actual bytes: A\n\nB\n\nC. Expected standalone-line handling: A\nB\nC.

Explicit trim markers were also ineffective in the tested fallback block: A {{~#> missing~}} B {{~/missing~}} C rendered A  B  C.

Apply whitespace handling to partial blocks, including their opening and closing strip flags. Cover both a supplied partial and fallback execution.

### R14. P2: whitespace changes inverse syntax into a different construct

Source: [src/handlebars.l:288](../../src/handlebars.l#L288), [src/handlebars.l:308](../../src/handlebars.l#L308), and the generated lexer.

| Template | Actual |
| --- | --- |
| {{#if false}}A{{^}}B{{/if}} | B |
| {{#if false}}A{{^ }}B{{/if}} | Syntax error |
| {{#if false}}A{{else if true}}B{{/if}} | B |
| {{#if false}}A{{ else if true}}B{{/if}} | Empty output |

These rules use \s where the Flex grammar needs its existing WHITESPACE definition. The resulting scanner does not implement the intended whitespace match.

Correct the grammar and regenerate the committed lexer. Keep these comparisons in both tokenizer and render tests.

### R15. P2: a final backslash breaks compatibility-mode rendering

Source: [src/handlebars_delimiters.c:94](../../src/handlebars_delimiters.c#L94).

Rendering ordinary text ending in a backslash with --flags=compat failed with “Template contains an embedded NUL byte.” The delimiter preprocessor advances past the final backslash and appends the terminator as content.

Handle a final backslash explicitly without appending a terminator byte. Add a literal-text regression and a preprocessor output-length assertion.

### R16. P2: AST reconstruction changes literal-key lookup meaning

Sources: [src/handlebars_ast_helpers.c:278](../../src/handlebars_ast_helpers.c#L278), [src/handlebars_ast_printer.c:924](../../src/handlebars_ast_printer.c#L924).

A parse → handlebars_ast_to_string → render experiment used:

~~~json
{"a.b":"LITERAL","a":{"b":"NESTED"}}
~~~

The original template {{[a.b]}} rendered LITERAL. Reconstruction produced {{a.b}}, which rendered NESTED.

The original path loses the information needed to reconstruct its literal segment. Preserve or recreate literal-segment quoting and compare rendering semantics in round-trip tests. String-literal backslash reconstruction remains unverified and is not included as a confirmed defect.

### R17. P2: this recognition misclassifies ordinary identifiers

Sources: [src/handlebars_ast_helpers.c:492](../../src/handlebars_ast_helpers.c#L492), [src/handlebars_compiler.c:667](../../src/handlebars_compiler.c#L667).

With {"items":["A","B"]}, an each block parameter named item rendered AB, while the same template using mythis rendered an empty string:

~~~handlebars
{{#each items as |mythis|}}{{mythis}}{{/each}}
~~~

The scoped-path predicate uses a substring match and incorrectly bypasses block-parameter lookup.

A separate tracked-ID check compiled {{h thisName}} with track_ids. The lookup retained thisName, but pushId contained Name.

Recognize the this path component and its permitted separators, rather than substrings or unbounded prefixes. Test identifiers that begin with, end with, or contain those letters.

### R18. P3: tracked numeric IDs are truncated

Source: [src/handlebars_compiler.c:682](../../src/handlebars_compiler.c#L682).

Compiling {{h 12345678901}} with track_ids produced:

~~~text
pushId[STRING:NumberLiteral][LONG:1234567890][NULL]
pushLiteral[LONG:12345678901]
~~~

The helper argument and its tracked ID disagree because the numeric scan uses %10ld, which consumes at most ten characters.

Use a checked conversion covering the supported integer type, or carry the numeric operand directly. Test IDs against their associated argument values.

## Data conversion, containers, and ownership

### R19. P2: recursive conversion of native containers discards converted children

Source: [src/handlebars_value.c:410](../../src/handlebars_value.c#L410).

A C API probe inserted a JSON-backed object into a native array and, separately, a native map. After handlebars_value_convert on each container, retrieving its child still reported real type USER. Converting the same JSON wrapper directly reported MAP.

The traversal converts the iterator's copied current value without writing the replacement back to the container.

Convert actual stored slots, or publish converted children through the appropriate container mutation operation while respecting iterator and copy-on-write rules. Test mixed native/wrapped containers, not only a wrapped root.

### R20. P2: JSON scalar termination and null membership are inconsistent

Sources: [src/handlebars_json.c:424](../../src/handlebars_json.c#L424), [src/handlebars_json.c:222](../../src/handlebars_json.c#L222), [src/handlebars_json.c:233](../../src/handlebars_json.c#L233).

Two separate behaviors were confirmed:

| Input or operation | Actual result | Control |
| --- | --- | --- |
| Data file containing exactly 1, true, or null | JSON “continue” error | Adding one trailing newline succeeds |
| Strict lookup of known in {"known":null}, using --no-convert-input | “known” not defined | Native conversion succeeds and renders empty |

The length-delimited parser does not finish scalar parsing at the supplied document boundary. Lazy object/array lookup also uses a null pointer as both “absent” and “present with JSON null.”

Finalize parsing without requiring callers to add whitespace. Use explicit membership/index validity checks independently of the returned JSON value. Test scalar documents at exact length and native/lazy parity for null members.

### R21. P2: YAML quoted scalars are converted to non-string values

Source: [src/handlebars_yaml.c:343](../../src/handlebars_yaml.c#L343).

For the template {{#if value}}truthy{{else}}falsy{{/if}}:{{value}}:

| YAML | Actual output |
| --- | --- |
| value: "false" | falsy:false |
| value: "0012" | truthy:12 |

The first should remain a nonempty string, and the second should retain its leading zeros. Scalar conversion ignores the quoting style when inferring types.

Respect scalar style and explicit type information when resolving YAML values. Add paired quoted/unquoted tests that inspect both type and output. Explicit-tag behavior was inspected in source, but the !!str 12 rendering probe alone did not distinguish its runtime type.

### R22. P2: CLI YAML dispatch depends on filename length and parsed truthiness

Source: [bin/handlebarsc.c:809](../../bin/handlebarsc.c#L809).

The same file contents, name: world, rendered successfully as aa.yml but failed as a.yml with a JSON parse error. The shared suffix guard requires more than five characters, incorrectly excluding a one-character .yml basename.

After successful YAML conversion, the CLI uses handlebars_value_is_empty to decide whether to parse again as JSON. Files containing a YAML document marker followed by false, 0, or [] all failed with JSON errors. The corresponding true document succeeded.

Select the parser once from the input format. Track parse success separately from the parsed value. Test short relative filenames and each valid falsy YAML root.

### R23. P2: duplicate cache keys have three different contracts

Sources: [src/handlebars_cache.h:165](../../src/handlebars_cache.h#L165), [src/handlebars_cache_simple.c:192](../../src/handlebars_cache_simple.c#L192), [src/handlebars_cache_mmap.c:480](../../src/handlebars_cache_mmap.c#L480), [src/handlebars_cache_lmdb.c:313](../../src/handlebars_cache_lmdb.c#L313).

A normal C API probe added a module rendering first, then another module rendering second, under the same key:

| Backend | Second add status | Module found afterward |
| --- | --- | --- |
| Simple | Error | first |
| mmap | Success | first |
| LMDB | Success | second |

The public header says duplicate insertion is an error. Backend changes therefore alter observable behavior.

Choose and document one duplicate policy, or expose the difference explicitly. Run the same contract tests against every backend rather than using backend-specific expectations alone.

### R24. P2: compatibility-mode partial rendering retains temporary strings

Source: [src/handlebars_vm.c:2424](../../src/handlebars_vm.c#L2424).

A C API probe repeatedly rendered a cached partial using the same VM and released every returned output. In compatibility mode, allocations owned by the VM grew as follows:

| Renders | Live VM blocks | Live VM bytes |
| --- | --- | --- |
| 1 | 5 | 481 |
| 10 | 14 | 706 |
| 100 | 104 | 2,956 |

The ordinary-mode control stayed at four blocks and 456 bytes. The compatibility append path copies the expression buffer but does not release it. The ordinary indentation path consumes that buffer.

This is retained memory during VM reuse, not a process-exit leak. Context destruction still frees it, so LeakSanitizer's passing result is consistent with this experiment.

Release the temporary in the compatibility path with appropriate cleanup on errors. Add a warmed-up, repeated-render allocation assertion. The existing compatibility regression in tests/test_cache.c renders twice without measuring retention and overwrites its first output pointer.

### R25. P2: string APIs have inconsistent result and input ownership

Sources: [src/handlebars_string.h:154](../../src/handlebars_string.h#L154), [src/handlebars_string.c:565](../../src/handlebars_string.c#L565), [src/handlebars_string.h:294](../../src/handlebars_string.h#L294), [src/handlebars_string.c:996](../../src/handlebars_string.c#L996).

Two benign ownership probes established:

- handlebars_str_replace promises a newly allocated result. Empty input and empty search returned the original pointer, while an ordinary replacement returned a distinct pointer.
- An input destructor installed solely as an observation hook ran during handlebars_string_indent, before that function returned. The header describes a new result but does not describe consuming the input reference.

The probes did not free aliases twice or access released input. They establish contract differences, not an experimentally triggered memory-corruption claim.

Give replacement results a consistent ownership rule and document consuming parameters explicitly. Add ownership tests for no-op and empty cases. API names and documentation should make consumption differences between append operations visible to callers.

### R26. P3: public value contracts and a convenience macro need correction

Sources: [src/handlebars_value.h:317](../../src/handlebars_value.h#L317), [src/handlebars_value.h:335](../../src/handlebars_value.h#L335), [src/handlebars_value.h:424](../../src/handlebars_value.h#L424), [src/handlebars_value.c:298](../../src/handlebars_value.c#L298).

The getter experiments found:

| Documented behavior | Observed behavior |
| --- | --- |
| get_strval returns an empty string for non-string values | An integer value returned NULL |
| get_boolval follows JavaScript boolean conversion | String "0" and an empty native array returned false; NaN returned true |

Either align these functions with their stated contracts or document the actual conversion policy. Avoid quietly changing established truthiness behavior without considering callers.

The convert convenience macro also ends with a semicolon. This ordinary consumer code failed to compile with “else without a previous if”:

~~~c
if (condition)
    handlebars_value_convert(value);
else
    handlebars_value_null(value);
~~~

Remove the macro's trailing semicolon, or use a suitable inline wrapper. Header syntax tests alone do not exercise macro composition.

A separate benign state check showed that setting SAFE_STRING, replacing the value with an integer, and then replacing it with a different ordinary string leaves the flag byte equal to one. This is a **potential lifecycle hardening issue**, not an established affected application: define whether flags belong to the payload or the value slot, then reset or preserve them deliberately. No unsafe-content rendering experiment was performed.

## CLI and diagnostics

### R27. P2: CLI validation accepts invalid options or silently changes their meaning

Source: [bin/handlebarsc.c:140](../../bin/handlebarsc.c#L140).

Confirmed Release-build results:

| Arguments | Actual |
| --- | --- |
| --invalid --version | Printed an option error, then version output, and exited zero |
| --flags=typo_flag | Rendered successfully with no diagnostic |
| --flags=not_strict | Enabled strict behavior |
| --run-count=potato | Rendered successfully |
| --run-count=2oops | Rendered successfully |
| --run-count=0 | Still performed a render |

Unknown options fall through an assertion that disappears in Release builds. Flag parsing uses substring matches. Numeric options ignore conversion success and complete-input validation.

Return explicit errors from option parsing. Parse comma-separated flags by exact token, and reuse checked integer parsing like that already used for external-helper limits. Define the zero-run behavior instead of accepting it accidentally.

### R28. P2: output failure is reported as success

Sources: [bin/handlebarsc.c:876](../../bin/handlebarsc.c#L876), [bin/handlebarsc.c:880](../../bin/handlebarsc.c#L880).

Rendering hello with stdout redirected to /dev/full exited zero and emitted no diagnostic, despite the output sink being unable to accept data.

Check write and final flush errors and propagate failure to the process exit status. Include a failing-output-stream test, including buffered output that fails only at flush time.

### R29. P3: diagnostic locations depend on preceding text, and copies truncate silently

Sources: [src/handlebars.l:88](../../src/handlebars.l#L88), [src/handlebars.c:139](../../src/handlebars.c#L139), [src/handlebars.c:166](../../src/handlebars.c#L166).

The same syntax error on line two was reported at different columns:

| Template bytes | Reported location |
| --- | --- |
| a\n{{) }} | Line 2, column 6 |
| abcdefghij\n{{) }} | Line 2, column 15 |

Lexer column accounting adds the whole token length after newline handling, carrying preceding-line length into the next line.

Separately, a 600-character ordinary error message remained 600 characters through handlebars_error_msg, but the copied diagnostic APIs returned lengths 255 and 511. The fixed formatting buffers silently truncate the message, and the normal form can lose its location suffix.

Track positions relative to the last newline. Allocate formatted diagnostics to the required length, or explicitly report truncation. Test locations with different preceding-line lengths and long messages whose distinguishing information is near the end.

### R30. P3: opcode utility APIs do not honor their advertised mappings or flags

Sources: [src/handlebars_opcodes.c:274](../../src/handlebars_opcodes.c#L274), [src/handlebars_opcode_printer.c:183](../../src/handlebars_opcode_printer.c#L183).

Two direct API probes found:

- The valid return opcode mapped from enum value 27 to the name return, but reverse mapping returned -1.
- Printing the same program with and without handlebars_opcode_printer_flag_no_newlines produced identical output containing a newline.

Complete the reverse mapping and implement or retire the ineffective option. Add round-trip checks over every supported opcode name and direct assertions on printer flags.

## Test comprehensiveness and dependency maintenance

### R31. P2: three specification runners omit their final fixture

Sources: [tests/test_spec_handlebars_compiler.c:728](../../tests/test_spec_handlebars_compiler.c#L728), [tests/test_spec_handlebars_parser.c:280](../../tests/test_spec_handlebars_parser.c#L280), [tests/test_spec_handlebars_tokenizer.c:250](../../tests/test_spec_handlebars_tokenizer.c#L250).

Each runner supplies tests_len - 1 as the exclusive loop-test end.

Fresh baseline logs reported:

| Runner | Loaded | Executed |
| --- | --- | --- |
| Compiler | 461 | 460 |
| Parser | 79 | 78 |
| Tokenizer | 78 | 77 |

For a stronger control, a temporary copy of the compiler exports changed the last fixture's expected appendContent operand to REVIEW_SENTINEL. All 460 checks still passed. Moving that same modified fixture one position earlier produced one failure.

Use tests_len as the exclusive end and assert registered/executed counts against loaded fixtures. The omitted parser and tokenizer entries were “should fail if directives have inverse” and the last “tokenizes raw blocks” case.

### R32. P2: a missing runtime fixture file silently reduces coverage

Source: [tests/test_spec_handlebars.c:597](../../tests/test_spec_handlebars.c#L597).

Removing only whitespace-control.json from a temporary fixture directory produced:

~~~text
Failed to read spec file: .../whitespace-control.json
Loaded 413 test cases
Handlebars spec exclusions: 11 runtime, 40 AST-inapplicable
100%: Checks: 776, Failures: 0, Errors: 0
~~~

The process exited zero. The baseline loaded 449 fixtures and executed 848 checks, so 72 checks disappeared while the exclusion budget remained satisfied.

Require every expected fixture file to load successfully and validate the fixture inventory. The existing exclusion budget catches some missing-fixture scenarios, including broad losses, but it does not catch this one.

### R33. P2: floating-point tests discard fractional precision

Sources: [tests/test_json.c:91](../../tests/test_json.c#L91), [tests/test_yaml.c:109](../../tests/test_yaml.c#L109), [tests/test_value.c:2599](../../tests/test_value.c#L2599).

These tests compare fractional values using ck_assert_int_eq. Check 0.15.2 converts the operands to integers.

A standalone Check control using ck_assert_int_eq(1234.0, 1234.4321) passed. The same values compared with ck_assert_double_eq_tol and a tolerance of 0.000001 failed.

Use floating-point assertions with intentional tolerances, and verify that changing the fractional part makes the test fail. A test's API choice must preserve the property it is supposed to check.

### R34. P2: the token-print allocation-failure test never attempts the operation

Source: [tests/test_token.c:266](../../tests/test_token.c#L266).

The first setjmp return is zero. The test negates it and immediately returns, before enabling failure injection or calling token_print.

A temporary instrumented copy, linked to the sanitizer/memory-testing build, ran the isolated test and reported one passing check. It printed a marker in the early-return branch and never printed the marker immediately before enabling injection.

Correct the branch and require evidence that the intended allocation failure was reached. Merely compiling the allocation-failure variant does not establish that its failure path ran.

### R35. P3: bundled XXH3 hashing depends on update partitioning

Sources: [vendor/xxhash/xxh3.h:1598](../../vendor/xxhash/xxh3.h#L1598), [vendor/xxhash/xxh3.h:1652](../../vendor/xxhash/xxh3.h#L1652), [src/handlebars_string.c:172](../../src/handlebars_string.c#L172).

The bundled header documents streaming equivalence to its one-shot operation. A dependency-only probe hashed ordinary alphabetic patterned data using one-shot, one update, and one-byte updates.

For 257 bytes:

~~~text
one-shot:       973507107031e78f
one update:     7b30fb4e88faab9d
one-byte calls: 973507107031e78f
~~~

Lengths 300 and 513 also disagreed. Controls at 256, 320, and 512 agreed.

The project uses the one-update streaming path for string hashing. This experiment demonstrates an algorithm-equivalence defect, not a cryptographic or collision-security claim. The project's consistent use of one update can hide the dependency inconsistency.

Do not patch the vendored implementation locally. Consider updating to a newer upstream release, with equivalence tests across chunk boundaries. Consider persisted hash/cache compatibility when changing the algorithm.

## Improvements supported by these results

The useful cleanup work follows from the defects above:

- Put build features and versions in one authoritative representation, then test installed consumers and extracted source archives.
- Reduce duplicated opcode and inline-partial inference. Explicit metadata is easier to validate than multiple scanners trying to rediscover declaration boundaries.
- Separate format selection, parse status, value truthiness, membership, and ownership. Several defects result from using one of these concepts as a proxy for another.
- Add native/lazy representation parity tests and render-equivalence AST tests.
- Exercise debug diagnostics as well as Release rendering.
- Measure retained allocations during VM reuse, in addition to checking eventual cleanup.
- Validate test selection and assertion sensitivity, not only a successful test-process exit.

The code already has substantial ownership, error-boundary, cache, API-header, sanitizer, and allocation-failure coverage. The experiments show where that coverage does not yet establish the claimed behavior.

No style-only changes are recommended without a concrete maintenance benefit. In particular, a passing local test with a weak assertion was not treated as proof that the entire suite lacks coverage.

## Unresolved questions and rejected broader claims

These were not promoted to experimentally confirmed findings:

- Ordinary-mode partial rendering remained allocation-stable. The retained-memory finding is restricted to the tested compatibility path.
- Strict-mode behavior changed correctly with a warmed cache. R08 uses the separately confirmed compilation/escaping flag case.
- Partial-block trim markers disappeared during AST reconstruction, but the original renderer already ignored their effect. This was not counted as a second demonstrated rendering change.
- Explicit YAML tags, string-literal backslash reconstruction, sparse-iterator completion assertions, and compiler allocation loops need additional focused controls before separate findings are justified.
- Very-large-input integer bounds, formatting cleanup across allocation failure, alternative pthread mutex setup, read-only cache/string contracts, custom iterator unwinding, and mmap reset/release latency remain source-review follow-ups. No special triggering inputs, exploitability results, or platform validation are claimed.
- The lexer reallocates its token-pointer array for every token, but no performance experiment established the practical cost. A growth-policy change should be benchmarked before being recommended as a measured improvement.

## Reproduction notes and retained evidence

CLI examples can be run from a configured checkout, for example:

~~~sh
printf '%s' '{{#if 0.0}}T{{else}}F{{/if}}' |
    ./bin/handlebarsc --no-newline -
~~~

For examples with data, write the table's JSON or YAML to a temporary file and pass --data with that filename. Filename-length experiments require using the short relative names exactly. C API probes used fresh contexts, valid modules produced by the project's compiler, and balanced returned-output releases unless testing an explicitly described ownership observation.

The local experiment directory is /tmp/handlebars-project-review/. It contains the probe sources and measured outputs, including:

- api-probe.c, api-results.txt, and api-results-compat.txt for conversion, cache contracts/settings, and allocation retention.
- contracts-probe.c, contracts-results.txt, macro-probe.c, and macro-results.txt.
- ast-probe.c and ast-results.txt.
- utility-probe.c, utility-results.txt, hash-probe.c, and hash-results.txt.
- float-assert-probe.c, float-assert-results.txt, token-test-reachability.txt, and test-gap-results.json.
- cli/ JSON result files with exact inputs, stdout, stderr, and statuses.
- CMake build, installation, consumer, optional-dependency, and debug-module logs.
- asan-build.log, asan-tests.log, distcheck.log, dist-control.log, and archive-inspection.txt.

The baseline log is /tmp/handlebars-review-baseline.log.

Raw review artifacts are under /tmp/handlebars-project-review/.c-review-results/20260904/: findings.json, REPORT.md, REPORT.sarif, units.json, ledger-gate.json, parts/, and assignments/. They retain rejected coverage accounting and unverified candidates. They are local working artifacts, not additional confirmed findings, and may be removed by temporary-directory cleanup.

All confirmed observations and their interpretation are included above so the report remains useful after those temporary artifacts disappear.

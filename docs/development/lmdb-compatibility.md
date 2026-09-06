# LMDB compatibility checks

The September 5, 2026 [macOS CI failure](https://github.com/jbboehr/handlebars.c/actions/runs/33994554104/job/101382668610)
used LMDB 1.0.1. The [next passing job](https://github.com/jbboehr/handlebars.c/actions/runs/33998815691/job/101393910500)
used 0.9.35 on a different runner image. The intervening CMake changes
did not fix the cache failures.

The existing cache suite reproduced all three failures on Linux with
LMDB 1.0.1. The oversized-key test assumed that 1,024 bytes exceeded the
database limit. Locally, `mdb_env_get_maxkeysize()` reported 511 for
0.9.35 and 1,978 for 1.0.1. The test now queries an opened environment and
checks the largest supported cache key and the first unsupported size,
including the trailing NUL stored by the cache.

The two GC tests aborted in LMDB's write-map transaction commit path.
Removing `MDB_MAPASYNC` alone did not help; disabling `MDB_WRITEMAP` did.
The backend and raw-record test helpers now open environments with
`MDB_NOSUBDIR`, using LMDB's default write mode and synchronous commits.
The GC test also checks that deletion survives reopening and that another
add/find/GC cycle succeeds. This is a compatibility workaround for the
observed failure; the underlying LMDB assertion has not been independently
established as an upstream defect.

Synchronous commits may increase cache-write latency; storage performance
has not been benchmarked. Applications sharing a cache file should stop
all users of the old library before starting users of the new library.
LMDB documents that processes using different `MDB_WRITEMAP` settings
must not share an environment concurrently. See the
[LMDB environment flag documentation](https://github.com/LMDB/lmdb/blob/LMDB_1.0.1/libraries/liblmdb/lmdb.h).

The CI matrix builds upstream LMDB 0.9.35 and 1.0.1 explicitly and runs
the CMake suite with allocation-failure testing enabled for each version.
Both configurations passed all 28 CTest programs locally with GCC 15.2.0
on Linux. With LMDB 1.0.1, the project also passed all 28 programs under
AddressSanitizer and UndefinedBehaviorSanitizer, and Autotools `make check`
passed all 3,529 checks. The LMDB dependency itself was built with assertions
enabled but without sanitizer instrumentation. Workflow validation with
`actionlint` also passed. The changed workflow has not yet run on GitHub,
and the patch has not been tested locally on macOS.

Independent correctness review found no actionable defects. The test review
added focused checks for the largest valid key surviving reopening and for
both GC cycles persisting their deletions. All of the checks above were rerun
after incorporating those assertions. The reliability verdict is
`PASS_WITH_RESIDUAL_RISK`, with the platform and performance limits described
above.

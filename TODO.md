# TODO

* Benchmark custom known-helper lookup and replace the compiler's linear list
  with an indexed container if it is a meaningful bottleneck.
* Provide a public API option that reports errors without requiring consumers
  to use `setjmp`/`longjmp`. Internal use of `longjmp` may remain.
* Implement decorator execution and inline partials, then enable their excluded
  upstream specification tests.
* Define how `handlebarsc` should load and register custom helpers, such as
  through a shared-library plugin interface.

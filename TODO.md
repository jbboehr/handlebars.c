# TODO

* Benchmark custom known-helper lookup and replace the compiler's linear list
  with an indexed container if it is a meaningful bottleneck.
* Extend the explicit-status `_try` API beyond the core pipeline to optional
  JSON/YAML conversion, caches, and loaders, and define whether custom helper
  callbacks need an explicit-status ABI.
* Implement general decorator execution and enable its excluded upstream
  specification tests. Inline partials are supported without executing general
  decorators.
* Define how `handlebarsc` should load and register custom helpers, such as
  through a shared-library plugin interface.

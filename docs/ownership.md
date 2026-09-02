# Ownership and lifetimes

`handlebars.c` uses talloc ownership and, in normal builds, reference counts.
They solve different problems:

- The talloc parent sets the maximum lifetime of an allocation.
- Reference counts retain shared values and support copy-on-write within that
  lifetime.

An `addref` call does not detach an object from its talloc parent. Destroying
the parent context frees its children even when one of those children has a
nonzero reference count.

## Talloc contexts

Most constructors take a context. The returned object is allocated beneath
that context, directly or through another object owned by it. Destroying the
context releases the complete allocation tree.

This means the following is invalid:

```c
struct handlebars_context *owner = handlebars_context_ctor();
struct handlebars_string *name = handlebars_string_ctor(
    owner,
    HBS_STRL("name")
);

handlebars_string_addref(name);
handlebars_context_dtor(owner);

/* name no longer exists. */
use_string(name);
```

To keep a string after its current owner is destroyed, copy it into a
longer-lived context before destroying the old one:

```c
struct handlebars_string *saved = handlebars_string_copy_ctor(
    longer_lived_context,
    name
);
```

The public map and stack copy constructors do not perform this transfer. They
allocate under the source container's context and share referenced payloads
with the source elements. Keep the source context alive, or reconstruct the
container and its payloads under the destination context using
application-specific knowledge. The library does not currently provide a
general cross-context deep copy for containers.

Do not treat `addref` or a same-context copy as a way to move an object to a
different context. `talloc_steal` is not a container deep copy either: the
container's stored context remains unchanged and its nested payloads can still
belong to the original ownership tree.

## Reference counts

Strings, maps, stacks, USER values, pointer wrappers, and closures use internal
reference counts in the default build. Assigning one of these objects to a
`struct handlebars_value` retains it automatically. Destroying or replacing
the value releases that reference.

Call `addref` when native code keeps an additional raw reference. Balance that
reference with `delref` when it is no longer needed. A final `delref` may free
the object before its talloc parent is destroyed. Avoid calling `talloc_free`
on an individual reference-counted object while aliases may still exist.

Reference counts also tell mutable strings and containers when they need a
copy-on-write replacement. They do not change the object's talloc parent.

When configured with `--disable-refcounting`, the `addref` and `delref`
operations are no-ops. Objects then remain allocated until their talloc context
is destroyed. Use a bounded context for each request or VM and destroy it after
the work is complete.

## VM configuration

The VM setters use two different ownership models:

| Setter | VM ownership | Caller responsibility |
| --- | --- | --- |
| `handlebars_vm_set_helpers()` | Copies the value and shares its reference-backed payloads. | Keep the payloads' original talloc context alive while the VM may use them. |
| `handlebars_vm_set_partials()` | Copies the value and shares its reference-backed payloads. | Keep the payloads' original talloc context alive while the VM may use them. |
| `handlebars_vm_set_data()` | Copies the value and shares its reference-backed payloads. | Keep the payloads' original talloc context alive while the VM may use them. |
| `handlebars_vm_set_cache()` | Borrows without adopting or reparenting. | Keep it alive until it is replaced or cleared and every VM call that may have used it has returned. Pass `NULL` to disable caching. |
| `handlebars_vm_set_logger()` | Borrows the callback context without adopting or reparenting. | A non-NULL context must remain valid while the callback can run. |

These setters do not explicitly destroy the cache or logger context. Existing
talloc ownership still applies: an object allocated beneath the VM is freed
recursively when the VM is destroyed, while an object owned elsewhere is not.
Do not destroy an object again after its talloc parent has freed it.

Clear an externally owned cache before destroying it if the VM will remain in
use:

```c
handlebars_vm_set_cache(vm, cache);
/* Render using cache. */

handlebars_vm_set_cache(vm, NULL);
handlebars_cache_dtor(cache);
```

Changing the cache during a VM call does not end an active cache lookup. The
previous cache must remain alive until that call returns so the VM can release
the lookup through the backend that produced it.

Copying a helpers, partials, or data value into the VM is not a cross-context
deep copy. In a refcounted build the shared payload is retained, but its talloc
parent remains its lifetime ceiling. In a no-refcount build the VM's copied
value instead relies entirely on that ownership tree.

## Common return values

| Value | Lifetime ceiling | Caller action |
| --- | --- | --- |
| `handlebars_vm_execute*()` result | VM context | Call `handlebars_string_delref()` after use. Copy it first if it must outlive the VM. |
| `handlebars_value_to_string()` result | Existing string owner or the supplied conversion context | Always call `handlebars_string_delref()` after use. |
| `hbs_str_val()` buffer | Source string | Do not free it. Stop using it when the string is modified or released. |
| Map, stack, and value accessors | Source container or value | Treat the result as borrowed unless that function documents a retained result. |
| `handlebars_cache_find*()` hit | Cache backend | Call `handlebars_cache_release()` or `handlebars_cache_release_try()` exactly once with the same cache and key. Do not use the module afterward. |
| `handlebars_ptr_try_get()` result | Wrapped pointee | Retrieval transfers no ownership. With `nofree=true`, the caller remains responsible for the pointee's lifetime. |

Calling `handlebars_string_delref()` uniformly for
`handlebars_value_to_string()` results works for both cases. A string input is
retained before it is returned. Other primitive inputs produce a new string
whose final `delref` frees it in a refcounted build. In a no-refcount build,
`delref` is a no-op and the owning context performs the cleanup.

For example:

```c
struct handlebars_string *text = handlebars_value_to_string(value, context);

consume(hbs_str_val(text), hbs_str_len(text));
handlebars_string_delref(text);
```

## Cache lookup results

Cache lookup has one release rule even though its implementation differs by
backend. The simple cache returns a cache-owned module and release is currently
a no-op. LMDB returns an allocated copy that release frees. The mmap cache
returns a view into shared memory and release drops its active-reader count.

Always pair a non-NULL result with one release call:

```c
struct handlebars_module *module = handlebars_cache_find(cache, key);

if (module != NULL) {
    use_module(module);
    handlebars_cache_release(cache, key, module);
}
```

A NULL result is a cache miss and must not be released. Keep the key and cache
alive until the matching release call, and do not use the module afterward.

The existing reset API has one backend-specific limitation. An mmap cache reset
may leave the cache unchanged while lookup results are active. Neither
`handlebars_cache_reset()` nor `handlebars_cache_reset_try()` reports that
condition. Release every hit before resetting when the caller requires the
cache to be empty.

## Immortal strings

`handlebars_string_immortalize()` disables destruction through the string's
reference count. It does not make the string outlive its talloc parent. The
serializer uses this state for strings embedded in serialized modules. It is
not a general way to transfer ownership or extend a string across contexts.

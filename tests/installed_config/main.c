#include <handlebars.h>
#include <handlebars_cache.h>
#include <stdio.h>
#include <string.h>

#if EXPECT_JSON != defined(HANDLEBARS_HAVE_JSON)
#error Installed JSON feature does not match the build
#endif
#if EXPECT_LMDB != defined(HANDLEBARS_HAVE_LMDB)
#error Installed LMDB feature does not match the build
#endif
#if EXPECT_PCRE != defined(HANDLEBARS_HAVE_PCRE)
#error Installed PCRE feature does not match the build
#endif
#if EXPECT_PTHREAD != defined(HANDLEBARS_HAVE_PTHREAD)
#error Installed pthread feature does not match the build
#endif
#if EXPECT_YAML != defined(HANDLEBARS_HAVE_YAML)
#error Installed YAML feature does not match the build
#endif
#if EXPECT_MEMORY != defined(HANDLEBARS_MEMORY)
#error Installed memory-testing feature does not match the build
#endif
#if EXPECT_TESTING_EXPORTS != defined(HANDLEBARS_TESTING_EXPORTS)
#error Installed testing-exports feature does not match the build
#endif

#if EXPECT_JSON
#if HANDLEBARS_HAVE_JSON != 1
#error Installed JSON feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_LMDB
#if HANDLEBARS_HAVE_LMDB != 1
#error Installed LMDB feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_PCRE
#if HANDLEBARS_HAVE_PCRE != 1
#error Installed PCRE feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_PTHREAD
#if HANDLEBARS_HAVE_PTHREAD != 1
#error Installed pthread feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_YAML
#if HANDLEBARS_HAVE_YAML != 1
#error Installed YAML feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_MEMORY
#if HANDLEBARS_MEMORY != 1
#error Installed memory-testing feature macro must equal 1 when enabled
#endif
#endif
#if EXPECT_TESTING_EXPORTS
#if HANDLEBARS_TESTING_EXPORTS != 1
#error Installed testing-exports feature macro must equal 1 when enabled
#endif
#endif

#if EXPECT_MEMORY
/* Check the enabled implementation without importing talloc's header here. */
extern int handlebars_memory_fail_get_state(void);
#endif

int main(void)
{
    /* Pin the published versions independently of CMake's configured values.
     * Update these expectations when changing the versions in configure.ac. */
    if (HANDLEBARS_VERSION_MAJOR != 1 || HANDLEBARS_VERSION_MINOR != 0
            || HANDLEBARS_VERSION_PATCH != 0 || HANDLEBARS_VERSION_INT != 10000
            || strcmp(HANDLEBARS_VERSION_STRING, "1.0.0")
            || handlebars_version() != HANDLEBARS_VERSION_INT
            || strcmp(handlebars_version_string(), HANDLEBARS_VERSION_STRING)
            || strcmp(HANDLEBARS_SPEC_VERSION_STRING, "4.7.7")
            || strcmp(handlebars_spec_version_string(), HANDLEBARS_SPEC_VERSION_STRING)
            || strcmp(MUSTACHE_SPEC_VERSION_STRING, "1.1.3")
            || strcmp(handlebars_mustache_spec_version_string(), MUSTACHE_SPEC_VERSION_STRING)) {
        fputs("Installed version information disagrees with the release or library\n", stderr);
        return 1;
    }

#if EXPECT_LMDB
    {
        struct handlebars_cache *(* volatile ctor)(struct handlebars_context *, const char *)
            = handlebars_cache_lmdb_ctor;
        if (!ctor) return 1;
    }
#endif
#if EXPECT_PTHREAD
    {
        struct handlebars_cache *(* volatile ctor)(struct handlebars_context *, size_t, size_t)
            = handlebars_cache_mmap_ctor;
        if (!ctor) return 1;
    }
#endif
#if EXPECT_MEMORY
    if (handlebars_memory_fail_get_state() != 0) return 1;
#endif
    return 0;
}

#include <handlebars.h>
#include <handlebars_cache.h>
#include <handlebars_json.h>
#include <handlebars_memory.h>
#include <handlebars_yaml.h>

#if EXPECT_MEMORY != defined(HANDLEBARS_MEMORY)
#error Imported target supplied a header for the wrong memory configuration
#endif

int main(void)
{
    int *value = handlebars_talloc(NULL, int);
    if (!value) return 1;
    *value = 42;
    if (talloc_free(value) != 0) return 1;

    if (handlebars_version() != HANDLEBARS_VERSION_INT) return 1;

#if EXPECT_JSON
    {
        enum handlebars_error_type (* volatile probe)(
            struct handlebars_context *, struct handlebars_value *, const char *)
            = handlebars_value_init_json_string_try;
        if (!probe) return 1;
    }
#endif
#if EXPECT_LMDB
    {
        struct handlebars_cache *(* volatile probe)(
            struct handlebars_context *, const char *) = handlebars_cache_lmdb_ctor;
        if (!probe) return 1;
    }
#endif
#if EXPECT_YAML
    {
        enum handlebars_error_type (* volatile probe)(
            struct handlebars_context *, struct handlebars_value *, const char *)
            = handlebars_value_init_yaml_string_try;
        if (!probe) return 1;
    }
#endif
#if EXPECT_MEMORY
    if (handlebars_memory_fail_get_state() != 0) return 1;
#endif
    return 0;
}

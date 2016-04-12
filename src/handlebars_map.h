
#ifndef HANDLEBARS_MAP_H
#define HANDLEBARS_MAP_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;
struct handlebars_value;

struct handlebars_map_entry {
    struct handlebars_string * key;
    struct handlebars_value * value;
    struct handlebars_map_entry * next;
    struct handlebars_map_entry * prev;
    struct handlebars_map_entry * parent;
    struct handlebars_map_entry * child;
};

struct handlebars_map {
    struct handlebars_context * ctx;
    size_t i;
    struct handlebars_map_entry * first;
    struct handlebars_map_entry * last;
    size_t table_size;
    struct handlebars_map_entry ** table;
    size_t collisions;
};

#define handlebars_map_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

/**
 * @brief Construct a new map
 * @param[in] ctx The handlebars context
 * @return The new map
 */
struct handlebars_map * handlebars_map_ctor(struct handlebars_context * ctx) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a map
 * @param[in] map The name
 * @return void
 */
void handlebars_map_dtor(struct handlebars_map * map) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Find a value by key (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @return The found value, or NULL
 */
struct handlebars_value * handlebars_map_find(struct handlebars_map * map, struct handlebars_string * key) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Find a value by key (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @return The found value, or NULL
 */
struct handlebars_value * handlebars_map_str_find(struct handlebars_map * map, const char * key, size_t len) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Add a value to a map. Adding a key twice is an error, use #handlebars_map_update instead. (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @param[in] value
 * @return void
 */
void handlebars_map_add(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Add a value to a map. Adding a key twice is an error, use #handlebars_map_str_update instead. (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @param[in] value
 * @return void
 */
void handlebars_map_str_add(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Update or add a value (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @param[in] value
 * @return void
 */
void handlebars_map_update(struct handlebars_map * map, struct handlebars_string * key, struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Update or add a value (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @param[in] value
 * @return void
 */
void handlebars_map_str_update(struct handlebars_map * map, const char * key, size_t len, struct handlebars_value * value) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Remove a value by key (#handlebars_string variant)
 * @param[in] map
 * @param[in] key
 * @return Whether or not an element was removed
 */
bool handlebars_map_remove(struct handlebars_map * map, struct handlebars_string * key) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Remove a value by key (const char[] with length variant)
 * @param[in] map
 * @param[in] key
 * @param[in] len
 * @return Whether or not an element was removed
 */
bool handlebars_map_str_remove(struct handlebars_map * map, const char * key, size_t len) HBS_ATTR_NONNULL_ALL;

#ifdef	__cplusplus
}
#endif

#endif

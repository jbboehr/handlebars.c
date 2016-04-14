
#ifndef HANDLEBARS_STACK_H
#define HANDLEBARS_STACK_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_context;
struct handlebars_value;

struct handlebars_stack {
    //! Handlebars context
    struct handlebars_context * ctx;
    //! Number of elements in the stack
    size_t i;
    //! Currently available number of elements (size of the buffer)
    size_t s;
    //! Data
    struct handlebars_value ** v;
};

/**
 * @brief Construct a new stack
 * @param[in] ctx The handlebars context
 * @return The new stack
 */
struct handlebars_stack * handlebars_stack_ctor(struct handlebars_context * ctx) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct a stack
 * @param[in] stack
 * @return void
 */
void handlebars_stack_dtor(struct handlebars_stack * stack) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Get the number of elements in a stack
 * @param[in] stack
 * @return The number of elements
 */
size_t handlebars_stack_length(struct handlebars_stack * stack) HBS_ATTR_NONNULL_ALL;

/**
 * @brief Push a value onto the top of a stack
 * @param[in] stack
 * @param[in] value
 * @return The pushed value
 */
struct handlebars_value * handlebars_stack_push(
    struct handlebars_stack * stack,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Pop a value from the top of a stack
 * @param[in] stack
 * @return The popped value
 */
struct handlebars_value * handlebars_stack_pop(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the value on the top of the stack
 * @param[in] stack
 * @return The value on the top of the stack
 */
struct handlebars_value * handlebars_stack_top(
    struct handlebars_stack * stack
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Get the value at the specified offset from the bottom of the stack
 * @param[in] stack
 * @param[in] offset
 * @return The value at the specified offset
 */
struct handlebars_value * handlebars_stack_get(
    struct handlebars_stack * stack, size_t offset
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Set the value at the specified offset from the bottom of the stack
 * @param[in] stack
 * @param[in] offset
 * @returnb
 */
struct handlebars_value * handlebars_stack_set(
    struct handlebars_stack * stack,
    size_t offset,
    struct handlebars_value * value
) HBS_ATTR_NONNULL_ALL HBS_ATTR_RETURNS_NONNULL;

void handlebars_stack_reverse(struct handlebars_stack * stack) HBS_ATTR_NONNULL_ALL;

#ifdef	__cplusplus
}
#endif

#endif

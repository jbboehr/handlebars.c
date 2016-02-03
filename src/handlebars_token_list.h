
/**
 * @file
 * @brief Linked list for tokens
 */

#ifndef HANDLEBARS_TOKEN_LIST_H
#define HANDLEBARS_TOKEN_LIST_H

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_token;

/**
 * @brief Token linked list item
 */
struct handlebars_token_list_item {
    struct handlebars_token_list_item * next;
    struct handlebars_token * data;
};

/**
 * @brief Token linked list root
 */
struct handlebars_token_list {
    struct handlebars_token_list_item * first;
    struct handlebars_token_list_item * last;
};

/**
 * @brief Iterate over a token list
 *
 * @param[in] list The list to iterate over
 * @param[out] el The current element
 * @param[out] tmp A temporary element
 */
#define handlebars_token_list_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

/**
 * @brief Append a token to a list
 * 
 * @param[in] list The list to which to append
 * @param[in] token The token to append
 * @return A return code from the handlebars_error_type enum. Success is zero.
 */
int handlebars_token_list_append(struct handlebars_token_list * list, struct handlebars_token * token);

/**
 * @brief Contruct a new token list
 * 
 * @param[in] ctx The talloc memory context
 * @return The newly constructed list
 */
struct handlebars_token_list * handlebars_token_list_ctor(void * ctx);

/**
 * @brief Destruct a token list
 * 
 * @param[in] list The list to destruct
 * @return void
 */
void handlebars_token_list_dtor(struct handlebars_token_list * list);

/**
 * @brief Prepend a token to a list
 * 
 * @param[in] list The list to which to prepend
 * @param[in] token The token to prepend
 * @return A return code from the handlebars_error_type enum. Success is zero.
 */
int handlebars_token_list_prepend(struct handlebars_token_list * list, struct handlebars_token * token);

#ifdef	__cplusplus
}
#endif

#endif

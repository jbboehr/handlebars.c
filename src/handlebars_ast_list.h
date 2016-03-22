
/**
 * @file
 * @brief Linked list for AST nodes
 */

#ifndef HANDLEBARS_AST_LIST_H
#define HANDLEBARS_AST_LIST_H

#include <stddef.h>

#include "handlebars.h"

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_node;
struct handlebars_parser;

/**
 * @brief AST node linked list item
 */
struct handlebars_ast_list_item {
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_list_item * prev;
    struct handlebars_ast_node * data;
};

/**
 * @brief AST node linked list root
 */
struct handlebars_ast_list {
    struct handlebars_context * ctx;
    struct handlebars_ast_list_item * first;
    struct handlebars_ast_list_item * last;
    size_t count;
};

/**
 * @brief Iterate over an AST list
 *
 * @param[in] list The list to iterate over
 * @param[out] el The current element
 * @param[out] tmp A temporary element
 */
#define handlebars_ast_list_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

/**
 * @brief Append an AST node to a list
 * 
 * @param[in] list The list to which to append
 * @param[in] ast_node The AST node to append
 */
void handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

/**
 * @brief Count the number of items in an AST list
 * 
 * @param[in] list The list to count
 * @return The number of items in the list
 */
size_t handlebars_ast_list_count(struct handlebars_ast_list * list);

/**
 * @brief Contruct a new AST node list
 * 
 * @param[in] context The handlebars context
 * @return The newly constructed list
 */
struct handlebars_ast_list * handlebars_ast_list_ctor(struct handlebars_context * context) HBS_ATTR_RETURNS_NONNULL;

/**
 * @brief Destruct an AST node list
 * 
 * @param[in] list The list to destruct
 * @return void
 */
void handlebars_ast_list_dtor(struct handlebars_ast_list * list);

/**
 * @brief Find a linked list node by its AST node
 *
 * @param[in] list The AST list
 * @param[in] ast_node The AST node
 * @return The linked list node, or NULL if not found
 */
struct handlebars_ast_list_item * handlebars_ast_list_find(
        struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

/**
 * @brief Insert a list node after the specified node
 *
 * @param[in] list The AST list
 * @param[in] item The item after which to insert
 * @param[in] new_item The new AST list item
 * @return void
 */
void handlebars_ast_list_insert_after(struct handlebars_ast_list * list, 
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item);

/**
 * @brief Insert a list node before the specified node
 *
 * @param[in] list The AST list
 * @param[in] item The item before which to insert
 * @param[in] new_item The new AST list item
 * @return void
 */
void handlebars_ast_list_insert_before(struct handlebars_ast_list * list, 
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item);

/**
 * @brief Remove an AST node from a list
 * 
 * @param[in] list The list from which to remove
 * @param[in] ast_node The node to remove
 * @return If the item was removed from the list
 */
bool handlebars_ast_list_remove(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

/**
 * @brief Prepend an AST node to a list
 * 
 * @param[in] list The list to which to prepend
 * @param[in] ast_node The node to prepend
 * @return A return code from the handlebars_error_type enum. Success is zero.
 */
void handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

#ifdef	__cplusplus
}
#endif

#endif

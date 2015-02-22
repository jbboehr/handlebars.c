
/**
 * @file
 * @brief Linked list for AST nodes
 */

#ifndef HANDLEBARS_AST_LIST_H
#define HANDLEBARS_AST_LIST_H

#ifdef	__cplusplus
extern "C" {
#endif

// Declarations
struct handlebars_ast_node;

/**
 * @brief AST node linked list item
 */
struct handlebars_ast_list_item {
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_node * data;
};

/**
 * @brief AST node linked list root
 */
struct handlebars_ast_list {
    struct handlebars_ast_list_item * first;
    struct handlebars_ast_list_item * last;
};

#define handlebars_ast_list_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

/**
 * @brief Append an AST node to a list
 * 
 * @param[in] list The list to which to append
 * @param[in] ast_node The AST node to append
 * @return A return code from the handlebars_error_type enum. Success is zero.
 */
int handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

/**
 * @brief Count the number of items in an AST list
 * 
 * @param[in] lust The list to count
 * @return The number of items in the list
 */
int handlebars_ast_list_count(struct handlebars_ast_list * list);

/**
 * @brief Contruct a new AST node list
 * 
 * @param[in] ctx The talloc memory context
 * @return The newly constructed list
 */
struct handlebars_ast_list * handlebars_ast_list_ctor(void * ctx);

/**
 * @brief Destruct an AST node list
 * 
 * @param[in] list The list to destruct
 * @return void
 */
void handlebars_ast_list_dtor(struct handlebars_ast_list * list);

/**
 * @brief Remove an AST node from a list
 * 
 * @param[in] list The list from which to remove
 * @param[in] ast_node The node to remove
 * @return The number of items removed (zero or one)
 */
int handlebars_ast_list_remove(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

/**
 * @brief Prepend an AST node to a list
 * 
 * @param[in] list The list to which to prepend
 * @param[in] ast_node The node to prepend
 * @return A return code from the handlebars_error_type enum. Success is zero.
 */
int handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

#ifdef	__cplusplus
}
#endif

#endif

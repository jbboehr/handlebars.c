
#ifndef HANDLEBARS_AST_LIST_H
#define HANDLEBARS_AST_LIST_H

#ifdef	__cplusplus
extern "C" {
#endif

struct handlebars_ast_node;

struct handlebars_ast_list_item {
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_node * data;
};

struct handlebars_ast_list {
    struct handlebars_ast_list_item * first;
    struct handlebars_ast_list_item * last;
};

#define handlebars_ast_list_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

int handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

struct handlebars_ast_list * handlebars_ast_list_ctor(void * ctx);

void handlebars_ast_list_dtor(struct handlebars_ast_list * list);

int handlebars_ast_list_remove(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

int handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node);

#ifdef	__cplusplus
}
#endif

#endif

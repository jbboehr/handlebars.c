
#ifndef HANDLEBARS_TOKEN_LIST_H
#define HANDLEBARS_TOKEN_LIST_H

struct handlebars_token;

struct handlebars_token_list_item {
    struct handlebars_token_list_item * next;
    struct handlebars_token * data;
};

struct handlebars_token_list {
    struct handlebars_token_list_item * first;
    struct handlebars_token_list_item * last;
};

#define handlebars_token_list_foreach(list, el, tmp) \
    for( (el) = (list->first); (el) && (tmp = (el)->next, 1); (el) = tmp)

int handlebars_token_list_append(struct handlebars_token_list * list, struct handlebars_token * token);

struct handlebars_token_list * handlebars_token_list_ctor(void * ctx);

void handlebars_token_list_dtor(struct handlebars_token_list * list);

#endif


#include <stdlib.h>
#include <errno.h>

#include "handlebars_ast.h"
#include "handlebars_memory.h"

struct handlebars_ast_node * handlebars_ast_node_ctor(enum handlebars_node_type type, void * ctx)
{
    struct handlebars_ast_node * ast_node;
    
    ast_node = handlebars_talloc(ctx, struct handlebars_ast_node);
    if( ast_node == NULL ) {
        errno = ENOMEM;
        goto done;
    }
    
done:
    return ast_node;
}

void handlebars_ast_node_dtor(struct handlebars_ast_node * ast_node)
{
    handlebars_talloc_free(ast_node);
}

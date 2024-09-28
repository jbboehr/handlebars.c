/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <talloc.h>

#define HANDLEBARS_AST_LIST_PRIVATE

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"



#undef CONTEXT
#define CONTEXT HBSCTX(context)

struct handlebars_ast_list * handlebars_ast_list_ctor(struct handlebars_context * context)
{
    struct handlebars_ast_list * list = MC(handlebars_talloc_zero(CONTEXT, struct handlebars_ast_list));
    list->ctx = CONTEXT;
    return list;
}

#undef CONTEXT
#define CONTEXT HBSCTX(list->ctx)

void handlebars_ast_list_append(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * item = NULL;

    assert(list != NULL);
    assert(ast_node != NULL);

    // Initialize list item
    item = MC(handlebars_talloc_zero(list, struct handlebars_ast_list_item));
    item->data = ast_node;

    // Append item
    if( list->last == NULL ) {
        // Initialize
        list->first = item;
        list->last = item;
    } else {
        // Append
        handlebars_ast_list_insert_after(list, list->last, item);
    }
    list->count++;
}

size_t handlebars_ast_list_count(struct handlebars_ast_list * list)
{
    if( unlikely(list == NULL) ) {
        return 0;
    } else {
        return list->count;
    }
}

void handlebars_ast_list_dtor(struct handlebars_ast_list * list)
{
   handlebars_talloc_free(list);
}

struct handlebars_ast_list_item * handlebars_ast_list_find(
        struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * item = NULL;
    struct handlebars_ast_list_item * tmp = NULL;

    handlebars_ast_list_foreach(list, item, tmp) {
        if( item->data == ast_node ) {
            return item;
        }
    }

    return NULL;
}

void handlebars_ast_list_insert_after(struct handlebars_ast_list * list,
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item)
{
    new_item->prev = item;
    new_item->next = item->next;
    if( item->next == NULL ) {
        list->last = new_item;
    } else {
        item->next->prev = new_item;
    }
    item->next = new_item;
}

void handlebars_ast_list_insert_before(struct handlebars_ast_list * list,
        struct handlebars_ast_list_item * item,
        struct handlebars_ast_list_item * new_item)
{
    new_item->prev = item->prev;
    new_item->next = item;
    if( item->prev == NULL ) {
        list->first = new_item;
    } else {
        item->prev->next = new_item;
    }
    item->prev = new_item;
}

bool handlebars_ast_list_remove(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * it;
    struct handlebars_ast_list_item * tmp;
    struct handlebars_ast_list_item * found = NULL;

    if( !list->first || !list->last ) {
        return false;
    }

    handlebars_ast_list_foreach(list, it, tmp) {
        if( it->data == ast_node ) {
            found = it;
            break;
        }
    }

    if( !found ) {
        return false;
    }

    if( found->prev == NULL ) {
        list->first = found->next;
    } else {
        found->prev->next = found->next;
    }
    if( found->next == NULL ) {
        list->last = found->prev;
    } else {
        found->next->prev = found->prev;
    }
    list->count--;

    handlebars_talloc_free(found);

    return true;
}

void handlebars_ast_list_prepend(struct handlebars_ast_list * list, struct handlebars_ast_node * ast_node)
{
    struct handlebars_ast_list_item * item = NULL;

    assert(list != NULL);
    assert(ast_node != NULL);

    // Initialize list item
    item = MC(handlebars_talloc_zero(list, struct handlebars_ast_list_item));
    item->data = ast_node;

    // Prepend item
    if( list->first == NULL ) {
        // Initialize
        list->first = item;
        list->last = item;
    } else {
        handlebars_ast_list_insert_before(list, list->first, item);
    }
    list->count++;
}

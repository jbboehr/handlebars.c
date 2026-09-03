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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "handlebars.h"
#include "handlebars_private.h"
#include "handlebars_memory.h"
#include "handlebars_value_private.h"

#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_yaml.h"



#define HANDLEBARS_YAML_MAX_DEPTH 256
#define HANDLEBARS_YAML_MAX_VALUES (64 * 1024)

struct _yaml_ctx {
    yaml_parser_t parser;
    yaml_document_t document;
    bool parser_initialized;
    bool document_initialized;
};

struct handlebars_yaml_allocation;

struct handlebars_yaml_allocation_watch {
    struct handlebars_yaml_allocation * allocation;
};

struct handlebars_yaml_allocation {
    void * ptr;
    struct handlebars_yaml_allocation_watch * watch;
    struct handlebars_yaml_allocation * next;
};

struct handlebars_yaml_convert_state {
    struct handlebars_context * context;
    yaml_document_t * document;
    void * allocations;
    struct handlebars_yaml_allocation * tracked;
    unsigned char * active;
    size_t node_count;
    size_t visited;
};

enum handlebars_yaml_try_operation {
    handlebars_yaml_try_node,
    handlebars_yaml_try_string
};

struct handlebars_yaml_try_state {
    struct handlebars_value value;
    enum handlebars_yaml_try_operation operation;
    yaml_document_t * document;
    yaml_node_t * node;
    const char * yaml;
    size_t length;
};

static int _yaml_ctx_dtor(struct _yaml_ctx * holder)
{
    if( holder->document_initialized ) {
        yaml_document_delete(&holder->document);
        holder->document_initialized = false;
    }
    if( holder->parser_initialized ) {
        yaml_parser_delete(&holder->parser);
        holder->parser_initialized = false;
    }
    return 0;
}

static HBS_ATTR_NORETURN void handlebars_yaml_rethrow(
    struct handlebars_context * context,
    jmp_buf * previous
)
{
    if( previous != NULL ) {
        handlebars_longjmp(context, previous, context->e->num);
    }
    abort();
}

static yaml_node_t * handlebars_yaml_get_node(
    struct handlebars_yaml_convert_state * state,
    int index,
    size_t * node_index
)
{
    if( unlikely(index <= 0 || (size_t) index > state->node_count) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid YAML node reference");
    }

    *node_index = (size_t) index - 1;
    return &state->document->nodes.start[*node_index];
}

static void handlebars_yaml_check_child_count(
    struct handlebars_yaml_convert_state * state,
    size_t count
)
{
    if( unlikely(count > HANDLEBARS_YAML_MAX_VALUES - state->visited) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "YAML value count exceeds limit");
    }
}

static int handlebars_yaml_allocation_watch_dtor(
    struct handlebars_yaml_allocation_watch * watch
)
{
    watch->allocation->ptr = NULL;
    watch->allocation->watch = NULL;
    return 0;
}

static struct handlebars_yaml_allocation * handlebars_yaml_track_allocation(
    struct handlebars_yaml_convert_state * state,
    void * ptr
)
{
    struct handlebars_yaml_allocation * allocation;
    struct handlebars_yaml_allocation_watch * watch;

    talloc_steal(state->allocations, ptr);
    allocation = handlebars_talloc(
        state->allocations,
        struct handlebars_yaml_allocation
    );
    HANDLEBARS_MEMCHECK(allocation, state->context);
    talloc_steal(allocation, ptr);
    watch = handlebars_talloc(ptr, struct handlebars_yaml_allocation_watch);
    HANDLEBARS_MEMCHECK(watch, state->context);
    watch->allocation = allocation;
    talloc_set_destructor(watch, handlebars_yaml_allocation_watch_dtor);
    allocation->ptr = ptr;
    allocation->watch = watch;
    allocation->next = state->tracked;
    state->tracked = allocation;
    return allocation;
}

static void handlebars_yaml_commit_allocations(
    struct handlebars_yaml_convert_state * state
)
{
    struct handlebars_yaml_allocation * allocation;

    for( allocation = state->tracked;
            allocation != NULL;
            allocation = allocation->next ) {
        if( allocation->ptr != NULL ) {
            talloc_set_destructor(allocation->watch, NULL);
            handlebars_talloc_free(allocation->watch);
            allocation->watch = NULL;
            talloc_steal(state->context, allocation->ptr);
        }
    }
    handlebars_talloc_free(state->allocations);
}

static size_t handlebars_yaml_find_node(
    struct handlebars_context * context,
    yaml_document_t * document,
    yaml_node_t * node,
    size_t node_count
) HBS_ATTR_NOINLINE;

static size_t handlebars_yaml_find_node(
    struct handlebars_context * context,
    yaml_document_t * document,
    yaml_node_t * node,
    size_t node_count
)
{
    for( size_t i = 0; i < node_count; i++ ) {
        if( &document->nodes.start[i] == node ) {
            return i;
        }
    }

    handlebars_throw(context, HANDLEBARS_ERROR, "YAML node is not part of document");
}

static void handlebars_value_init_yaml_node_ex(
    struct handlebars_yaml_convert_state * state,
    struct handlebars_value * value,
    yaml_node_t * node,
    size_t node_index,
    size_t depth
)
{
    HANDLEBARS_VALUE_DECL(tmp);
    yaml_node_pair_t * pair;
    yaml_node_item_t * item;
    const char * scalar;
    char * end;

    if( unlikely(depth >= HANDLEBARS_YAML_MAX_DEPTH) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "YAML nesting depth exceeds limit");
    }
    if( unlikely(state->visited == HANDLEBARS_YAML_MAX_VALUES) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "YAML value count exceeds limit");
    }
    if( unlikely(state->active[node_index]) ) {
        handlebars_throw(state->context, HANDLEBARS_ERROR, "Cyclic YAML alias reference");
    }

    state->active[node_index] = 1;
    state->visited++;

    switch( node->type ) {
        case YAML_MAPPING_NODE: {
            size_t count = (size_t) (
                node->data.mapping.pairs.top - node->data.mapping.pairs.start
            );
            struct handlebars_map * map;

            handlebars_yaml_check_child_count(state, count);
            map = handlebars_map_ctor(state->context, count);
            handlebars_yaml_track_allocation(state, map);
            for( pair = node->data.mapping.pairs.start;
                    pair < node->data.mapping.pairs.top;
                    pair++ ) {
                size_t key_index;
                size_t map_count;
                size_t value_index;
                struct handlebars_yaml_allocation * key_allocation;
                struct handlebars_string * key;
                yaml_node_t * key_node = handlebars_yaml_get_node(
                    state,
                    pair->key,
                    &key_index
                );
                yaml_node_t * value_node = handlebars_yaml_get_node(
                    state,
                    pair->value,
                    &value_index
                );

                (void) key_index;
                if( unlikely(key_node->type != YAML_SCALAR_NODE) ) {
                    handlebars_throw(
                        state->context,
                        HANDLEBARS_ERROR,
                        "Unsupported YAML mapping key type"
                    );
                }
                handlebars_value_init_yaml_node_ex(
                    state,
                    tmp,
                    value_node,
                    value_index,
                    depth + 1
                );
                map_count = handlebars_map_count(map);
                key = handlebars_string_ctor(
                    state->context,
                    (const char *) key_node->data.scalar.value,
                    key_node->data.scalar.length
                );
                key_allocation = handlebars_yaml_track_allocation(state, key);
                handlebars_string_addref(key);
                map = handlebars_map_update(map, key, tmp);
                if( handlebars_map_count(map) > map_count ) {
                    struct handlebars_string * stored_key =
                        handlebars_map_get_key_at_index(map, map_count);

                    if( stored_key != key ) {
                        key_allocation->ptr = NULL;
                        handlebars_yaml_track_allocation(state, stored_key);
                    }
                } else {
                    key_allocation->ptr = NULL;
                }
                handlebars_string_delref(key);
            }
            handlebars_value_map(value, map);
            break;
        }
        case YAML_SEQUENCE_NODE: {
            size_t count = (size_t) (
                node->data.sequence.items.top - node->data.sequence.items.start
            );
            struct handlebars_stack * stack;

            handlebars_yaml_check_child_count(state, count);
            stack = handlebars_stack_ctor(state->context, count);
            handlebars_yaml_track_allocation(state, stack);
            for( item = node->data.sequence.items.start;
                    item < node->data.sequence.items.top;
                    item++ ) {
                size_t value_index;
                yaml_node_t * value_node = handlebars_yaml_get_node(
                    state,
                    *item,
                    &value_index
                );

                handlebars_value_init_yaml_node_ex(
                    state,
                    tmp,
                    value_node,
                    value_index,
                    depth + 1
                );
                stack = handlebars_stack_push(stack, tmp);
            }
            handlebars_value_array(value, stack);
            break;
        }
        case YAML_SCALAR_NODE: {
            long lval;
            double dval;

            scalar = (const char *) node->data.scalar.value;
            if( node->data.scalar.length == 4 && memcmp(scalar, "true", 4) == 0 ) {
                handlebars_value_boolean(value, true);
                break;
            }
            if( node->data.scalar.length == 5 && memcmp(scalar, "false", 5) == 0 ) {
                handlebars_value_boolean(value, false);
                break;
            }

            errno = 0;
            end = NULL;
            lval = strtol(scalar, &end, 10);
            if( errno != ERANGE
                    && end != scalar
                    && (size_t) (end - scalar) == node->data.scalar.length ) {
                handlebars_value_integer(value, lval);
                break;
            }

            errno = 0;
            end = NULL;
            dval = strtod(scalar, &end);
            if( errno != ERANGE
                    && end != scalar
                    && (size_t) (end - scalar) == node->data.scalar.length ) {
                handlebars_value_float(value, dval);
                break;
            }

            {
                struct handlebars_string * string = handlebars_string_ctor(
                    state->context,
                    scalar,
                    node->data.scalar.length
                );
                handlebars_yaml_track_allocation(state, string);
                handlebars_value_str(value, string);
            }
            break;
        }
        case YAML_NO_NODE:
        default:
            handlebars_throw(state->context, HANDLEBARS_ERROR, "Invalid YAML node type");
    }

    state->active[node_index] = 0;
    HANDLEBARS_VALUE_UNDECL(tmp);
}

void handlebars_value_init_yaml_node(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct yaml_document_s * document,
    struct yaml_node_s * node
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    void * allocations;
    struct handlebars_yaml_convert_state state;
    size_t node_index;
    jmp_buf buf;

    if( unlikely(document->nodes.start == NULL
            || document->nodes.top == NULL
            || document->nodes.top <= document->nodes.start) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "Invalid YAML document");
    }

    state.context = context;
    state.document = document;
    state.allocations = NULL;
    state.tracked = NULL;
    state.node_count = (size_t) (document->nodes.top - document->nodes.start);
    state.visited = 0;
    state.active = NULL;
    node_index = handlebars_yaml_find_node(context, document, node, state.node_count);

    allocations = handlebars_talloc_size(context, 0);
    HANDLEBARS_MEMCHECK(allocations, context);
    state.allocations = allocations;

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_talloc_free(allocations);
        error->jmp = previous;
        handlebars_yaml_rethrow(context, previous);
    }

    state.active = handlebars_talloc_zero_size(allocations, state.node_count);
    HANDLEBARS_MEMCHECK(state.active, context);
    handlebars_value_init_yaml_node_ex(&state, value, node, node_index, 0);

    handlebars_talloc_free(state.active);
    state.active = NULL;
    handlebars_yaml_commit_allocations(&state);
    error->jmp = previous;
}

void handlebars_value_init_yaml_stringl(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * yaml,
    size_t length
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    struct _yaml_ctx * yctx = handlebars_talloc_zero(context, struct _yaml_ctx);
    yaml_node_t * node;
    jmp_buf buf;

    HANDLEBARS_MEMCHECK(yctx, context);
    talloc_set_destructor(yctx, _yaml_ctx_dtor);

    if( handlebars_setjmp_ex(context, &buf) ) {
        handlebars_talloc_free(yctx);
        error->jmp = previous;
        handlebars_yaml_rethrow(context, previous);
    }

    if( unlikely(!yaml_parser_initialize(&yctx->parser)) ) {
        handlebars_throw(context, HANDLEBARS_NOMEM, "Failed to initialize YAML parser");
    }
    yctx->parser_initialized = true;
    yaml_parser_set_input_string(
        &yctx->parser,
        (const unsigned char *) yaml,
        length
    );

    if( unlikely(!yaml_parser_load(&yctx->parser, &yctx->document)) ) {
        enum handlebars_error_type error_type = yctx->parser.error == YAML_MEMORY_ERROR
            ? HANDLEBARS_NOMEM
            : HANDLEBARS_ERROR;
        const char * problem = yctx->parser.problem != NULL
            ? yctx->parser.problem
            : "unknown error";

        handlebars_throw(
            context,
            error_type,
            "YAML Parse Error: [%d] %s",
            yctx->parser.error,
            problem
        );
    }
    yctx->document_initialized = true;

    node = yaml_document_get_root_node(&yctx->document);
    if( unlikely(node == NULL) ) {
        handlebars_throw(context, HANDLEBARS_ERROR, "YAML Parse Error: empty document");
    }
    handlebars_value_init_yaml_node(context, value, &yctx->document, node);

    handlebars_talloc_free(yctx);
    error->jmp = previous;
}

void handlebars_value_init_yaml_string(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * yaml
)
{
    handlebars_value_init_yaml_stringl(context, value, yaml, strlen(yaml));
}

HBS_ATTR_NOINLINE
static enum handlebars_error_type handlebars_yaml_try_guarded(
    struct handlebars_context * context,
    struct handlebars_yaml_try_state * state
)
{
    struct handlebars_error * error = context->e;
    jmp_buf * volatile previous = error->jmp;
    enum handlebars_error_type volatile caught = HANDLEBARS_SUCCESS;
    jmp_buf buf;

    if( handlebars_setjmp_ex(context, &buf) ) {
        caught = error->num;
    } else {
        switch( state->operation ) {
            case handlebars_yaml_try_node:
                handlebars_value_init_yaml_node(
                    context,
                    &state->value,
                    state->document,
                    state->node
                );
                break;
            case handlebars_yaml_try_string:
                handlebars_value_init_yaml_stringl(
                    context,
                    &state->value,
                    state->yaml,
                    state->length
                );
                break;
            default: abort(); // LCOV_EXCL_LINE
        }
    }

    error->jmp = previous;
    return caught;
}

static enum handlebars_error_type handlebars_yaml_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct handlebars_yaml_try_state * state
)
{
    enum handlebars_error_type error;

    handlebars_value_init(&state->value);
    handlebars_error_clear(context);
    error = handlebars_yaml_try_guarded(context, state);
    if( error == HANDLEBARS_SUCCESS ) {
        handlebars_value_value(value, &state->value);
    }
    handlebars_value_dtor(&state->value);
    return error;
}

enum handlebars_error_type handlebars_value_init_yaml_node_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    struct yaml_document_s * document,
    struct yaml_node_s * node
)
{
    struct handlebars_yaml_try_state state = {
        .operation = handlebars_yaml_try_node,
        .document = document,
        .node = node
    };

    return handlebars_yaml_try(context, value, &state);
}

enum handlebars_error_type handlebars_value_init_yaml_stringl_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * yaml,
    size_t length
)
{
    struct handlebars_yaml_try_state state = {
        .operation = handlebars_yaml_try_string,
        .yaml = yaml,
        .length = length
    };

    return handlebars_yaml_try(context, value, &state);
}

enum handlebars_error_type handlebars_value_init_yaml_string_try(
    struct handlebars_context * context,
    struct handlebars_value * value,
    const char * yaml
)
{
    return handlebars_value_init_yaml_stringl_try(
        context,
        value,
        yaml,
        strlen(yaml)
    );
}

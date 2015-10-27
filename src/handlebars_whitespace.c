
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

#include "handlebars_whitespace.h"

#include "handlebars.h"
#include "handlebars_ast.h"
#include "handlebars_ast_list.h"
#include "handlebars_context.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_scanners.h"
#include "handlebars_utils.h"



int handlebars_whitespace_is_next_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * next;
    struct handlebars_ast_node * sibling;
    
    if( !statements ) {
        return is_root;
    }

    if( statement == NULL ) {
        next = statements->first;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        next = (item ? item->next : NULL);
    }
    
    if( !next || !next->data ) {
        return (int) is_root;
    }
    
    sibling = (next->next ? next->next->data : NULL);
    
    if( next->data->type == HANDLEBARS_AST_NODE_CONTENT ) {
        return handlebars_scanner_next_whitespace(next->data->node.content.original, !sibling && is_root);
    }
    
    return 0;
}

int handlebars_whitespace_is_prev_whitespace(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short is_root)
{
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * prev;
    struct handlebars_ast_node * sibling;

    if( !statements ) {
        return is_root;
    }

    if( statement == NULL ) {
        prev = statements->last;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        prev = (item ? item->prev : NULL);
    }
    
    if( !prev || !prev->data ) {
        return (int) is_root;
    }
    
    sibling = (prev->prev ? prev->prev->data : NULL);
    
    if( prev->data->type == HANDLEBARS_AST_NODE_CONTENT ) {
        return handlebars_scanner_prev_whitespace(prev->data->node.content.original, !sibling && is_root);
    }
    
    return 0;
}

int handlebars_whitespace_omit_left(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple)
{
    struct handlebars_ast_node * current;
    struct handlebars_ast_list_item * item;
    size_t original_length;
    
    if( statement == NULL ) {
        current = statements->last ? statements->last->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->prev ? item->prev->data : NULL;
    }

    if( !current || current->type != HANDLEBARS_AST_NODE_CONTENT ||
            (!multiple && (current->strip & handlebars_ast_strip_flag_left_stripped)) ) {
        return 0;
    }
    
    original_length = strlen(current->node.content.value);

    if( multiple ) {
        current->node.content.value = handlebars_rtrim(current->node.content.value, " \v\t\r\n");
    } else {
        current->node.content.value = handlebars_rtrim(current->node.content.value, " \t");
    }
    
    if( original_length == strlen(current->node.content.value) ) {
        current->strip &= ~handlebars_ast_strip_flag_left_stripped;
    } else {
        current->strip |= handlebars_ast_strip_flag_left_stripped;
    }
    current->strip |= handlebars_ast_strip_flag_set;
    
    return 1 && (current->strip & handlebars_ast_strip_flag_left_stripped);
}

int handlebars_whitespace_omit_right(struct handlebars_ast_list * statements,
        struct handlebars_ast_node * statement, short multiple)
{
    struct handlebars_ast_node * current;
    struct handlebars_ast_list_item * item;
    size_t original_length;
    
    if( statement == NULL ) {
        current = statements->first ? statements->first->data : NULL;
    } else {
        item = handlebars_ast_list_find(statements, statement);
        current = item && item->next ? item->next->data : NULL;
    }

    if( !current || current->type != HANDLEBARS_AST_NODE_CONTENT ||
            (!multiple && (current->strip & handlebars_ast_strip_flag_right_stripped)) ) {
        return 0;
    }

    original_length = strlen(current->node.content.value);
    
    if( multiple ) {
        current->node.content.value = handlebars_ltrim(current->node.content.value, " \v\t\r\n");
    } else {
        current->node.content.value = handlebars_ltrim(current->node.content.value, " \t");
        if( *current->node.content.value == '\r' ) {
            memmove(current->node.content.value, current->node.content.value + 1, strlen(current->node.content.value));
        }
        if( *current->node.content.value == '\n' ) {
            memmove(current->node.content.value, current->node.content.value + 1, strlen(current->node.content.value));
        }
    }
    
    if( original_length == strlen(current->node.content.value) ) {
        current->strip &= ~handlebars_ast_strip_flag_right_stripped;
    } else {
        current->strip |= handlebars_ast_strip_flag_right_stripped;
    }
    current->strip |= handlebars_ast_strip_flag_set;

    return 1 && (current->strip & handlebars_ast_strip_flag_right_stripped);
}



static inline void handlebars_whitespace_accept_program(struct handlebars_context * context,
        struct handlebars_ast_node * program)
{
    short is_root = !context->whitespace_root_seen;
    int error = HANDLEBARS_SUCCESS;
    struct handlebars_ast_list * statements = program->node.program.statements;
    struct handlebars_ast_list_item * item;
    struct handlebars_ast_list_item * tmp;
    short do_standalone = 1; //!context->ignore_standalone;
    
    context->whitespace_root_seen = 1;
    
    if( !statements ) {
        return;
    }
    
    handlebars_ast_list_foreach(statements, item, tmp) {
        struct handlebars_ast_node * current = item->data;
        short is_prev_whitespace, is_next_whitespace, open_standalone,
              close_standalone, inline_standalone;
          
        handlebars_whitespace_accept(context, current);
        if( !current || !(current->strip & handlebars_ast_strip_flag_set) ) {
            continue;
        }
        is_prev_whitespace = handlebars_whitespace_is_prev_whitespace(statements, current, is_root);
        is_next_whitespace = handlebars_whitespace_is_next_whitespace(statements, current, is_root);
        open_standalone = (current->strip & handlebars_ast_strip_flag_open_standalone) && is_prev_whitespace;
        close_standalone = (current->strip & handlebars_ast_strip_flag_close_standalone) && is_next_whitespace;
        inline_standalone = (current->strip & handlebars_ast_strip_flag_inline_standalone) && is_prev_whitespace && is_next_whitespace;
        
        if( current->strip & handlebars_ast_strip_flag_right ) {
            handlebars_whitespace_omit_right(statements, current, 1);
        }
        if( current->strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(statements, current, 1);
        }
        if( do_standalone && inline_standalone ) {
            handlebars_whitespace_omit_right(statements, current, 0);
            if( handlebars_whitespace_omit_left(statements, current, 0) ) {
                struct handlebars_ast_node * prev = item->prev ? item->prev->data : NULL;
                if( current->type == HANDLEBARS_AST_NODE_PARTIAL &&
                        prev && prev->type == HANDLEBARS_AST_NODE_CONTENT ) {
                    char * start = prev->node.content.original;
                    char * ptr;
                    char * match = NULL;
                    for( ptr = start; *ptr; ++ptr ) {
                        if( *ptr == ' ' || *ptr == '\t' ) {
                            if( !match ) {
                                match = ptr;
                            }
                        } else if( *ptr ) {
                            match = NULL;
                        }
                    }
                    if( match ) {
                        current->node.partial.indent = handlebars_talloc_strdup(current, match);
                    }
                }
            }
        }
        if( do_standalone && open_standalone ) {
            if( current->type == HANDLEBARS_AST_NODE_BLOCK ) {
                if( current->node.block.program ) {
                    assert(current->node.block.program->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_whitespace_omit_right(current->node.block.program->node.program.statements, NULL, 0);
                } else if( current->node.block.inverse ) {
                    assert(current->node.block.inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_whitespace_omit_right(current->node.block.inverse->node.program.statements, NULL, 0);
                }
            }
            handlebars_whitespace_omit_left(statements, current, 0);
        }
        if( do_standalone && close_standalone ) {
            handlebars_whitespace_omit_right(statements, current, 0);
            if( current->type == HANDLEBARS_AST_NODE_BLOCK ) {
                if( current->node.block.inverse ) {
                    assert(current->node.block.inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_whitespace_omit_left(current->node.block.inverse->node.program.statements, NULL, 0);
                } else if( current->node.block.program ) {
                    assert(current->node.block.program->type == HANDLEBARS_AST_NODE_PROGRAM);
                    handlebars_whitespace_omit_left(current->node.block.program->node.program.statements, NULL, 0);
                }
            }
        }
    }
}

static inline struct handlebars_ast_node * _handlebars_whitespace_get_program(struct handlebars_ast_node * inverse, short first)
{
    struct handlebars_ast_list * statements;
    struct handlebars_ast_node * node = NULL;
    
    if( inverse && (statements = inverse->node.program.statements) ) {
        if( first ) {
            node = statements->first ? statements->first->data : NULL;
        } else {
            node = statements->last ? statements->last->data : NULL;
        }
    }
    if( node ) {
        if( node->type == HANDLEBARS_AST_NODE_BLOCK ) {
            return node->node.block.program;
        } else if( node->type == HANDLEBARS_AST_NODE_INVERSE ) {
            return node->node.inverse.program;
        } else if( node->type == HANDLEBARS_AST_NODE_RAW_BLOCK ) {
            return node->node.raw_block.program;
        }
    }
    
    return NULL;
}

static inline void handlebars_whitespace_accept_block(struct handlebars_context * context,
        struct handlebars_ast_node * block)
{
    struct handlebars_ast_node * program;
    struct handlebars_ast_node * inverse;
    struct handlebars_ast_node * firstInverse;
    struct handlebars_ast_node * lastInverse;
    unsigned strip = 0;
    short do_standalone = 1; //!context->ignore_standalone;
    
    handlebars_whitespace_accept(context, block->node.block.program);
    handlebars_whitespace_accept(context, block->node.block.inverse);
    
    program = (block->node.block.program ? block->node.block.program : block->node.block.inverse);
    inverse = (block->node.block.program ? block->node.block.inverse : NULL);
    firstInverse = lastInverse = inverse;
    
    if( inverse && inverse->node.program.chained ) {
        firstInverse = _handlebars_whitespace_get_program(inverse, 1);
        
        // Should this also allow inverse node?
        while( lastInverse && 
                lastInverse->type == HANDLEBARS_AST_NODE_PROGRAM && 
                lastInverse->node.program.chained ) {
            lastInverse = _handlebars_whitespace_get_program(lastInverse, 0);
            assert(lastInverse == NULL || lastInverse->type != HANDLEBARS_AST_NODE_INVERSE);
        }
    }
    
    strip |= (block->node.block.open_strip & handlebars_ast_strip_flag_left);
    strip |= (block->node.block.close_strip & handlebars_ast_strip_flag_right);
    if( program && handlebars_whitespace_is_next_whitespace(program->node.program.statements, NULL, 0) ) {
        strip |= handlebars_ast_strip_flag_open_standalone;
    }
    if( (program || firstInverse) && handlebars_whitespace_is_prev_whitespace((firstInverse ? firstInverse : program)->node.program.statements, NULL, 0) ) {
        strip |= handlebars_ast_strip_flag_close_standalone;
    }
    strip |= handlebars_ast_strip_flag_set;
    
    
    if( block->node.block.open_strip & handlebars_ast_strip_flag_right ) {
       handlebars_whitespace_omit_right(program->node.program.statements, NULL, 1);
    }
    
    if( inverse ) {
        unsigned inverse_strip = block->node.block.inverse_strip;
        
        if( inverse_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 1);
        }
        if( inverse_strip & handlebars_ast_strip_flag_right ) {
            handlebars_whitespace_omit_right(inverse->node.program.statements, NULL, 1);
        }
        if( block->node.block.close_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(inverse->node.program.statements, NULL, 1);
        }

        // Find standalone else statments
        if( do_standalone && program && firstInverse &&
                handlebars_whitespace_is_prev_whitespace(program->node.program.statements, NULL, 0) &&
                handlebars_whitespace_is_next_whitespace(firstInverse->node.program.statements, NULL, 0) ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 0);
            handlebars_whitespace_omit_right(firstInverse->node.program.statements, NULL, 0);
        }
    } else {
        if( block->node.block.close_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 1);
        }
    }
    
    block->strip = strip;
}

static inline void handlebars_whitespace_accept_partial(struct handlebars_context * context,
        struct handlebars_ast_node * partial)
{
    partial->strip |= handlebars_ast_strip_flag_set | handlebars_ast_strip_flag_inline_standalone;
}

static inline void handlebars_whitespace_accept_mustache(struct handlebars_context * context,
        struct handlebars_ast_node * mustache)
{
    // nothing?
}

static inline void handlebars_whitespace_accept_comment(struct handlebars_context * context,
        struct handlebars_ast_node * comment)
{
    comment->strip |= handlebars_ast_strip_flag_set | handlebars_ast_strip_flag_inline_standalone;
}

static inline void handlebars_whitespace_accept_raw_block(struct handlebars_context * context,
        struct handlebars_ast_node * raw_block)
{
    struct handlebars_ast_node * program;
    struct handlebars_ast_node * inverse;
    struct handlebars_ast_node * firstInverse;
    struct handlebars_ast_node * lastInverse;
    unsigned strip = 0;
    
    assert(raw_block != NULL);
    assert(raw_block->type == HANDLEBARS_AST_NODE_RAW_BLOCK);
    
    handlebars_whitespace_accept(context, raw_block->node.raw_block.program);
    handlebars_whitespace_accept(context, raw_block->node.raw_block.inverse);
    
    program = (raw_block->node.raw_block.program ? raw_block->node.raw_block.program : raw_block->node.raw_block.inverse);
    inverse = (raw_block->node.raw_block.program ? raw_block->node.raw_block.inverse : NULL);
    
    assert(program == NULL || program->type == HANDLEBARS_AST_NODE_PROGRAM);
    assert(inverse == NULL || inverse->type == HANDLEBARS_AST_NODE_PROGRAM);
    
    if( inverse && inverse->node.program.chained ) {
        firstInverse = _handlebars_whitespace_get_program(inverse, 1);
        
        // Should this also allow inverse node?
        while( lastInverse && 
                lastInverse->type == HANDLEBARS_AST_NODE_PROGRAM && 
                lastInverse->node.program.chained ) {
            lastInverse = _handlebars_whitespace_get_program(lastInverse, 0);
            assert(lastInverse == NULL || lastInverse->type != HANDLEBARS_AST_NODE_INVERSE);
        }
    }
    
    strip |= (raw_block->node.raw_block.open_strip & handlebars_ast_strip_flag_left);
    strip |= (raw_block->node.raw_block.close_strip & handlebars_ast_strip_flag_right);
    if( program && handlebars_whitespace_is_next_whitespace(program->node.program.statements, NULL, 0) ) {
        strip |= handlebars_ast_strip_flag_open_standalone;
    }
    if( (program || inverse) && handlebars_whitespace_is_prev_whitespace((inverse ? inverse : program)->node.program.statements, NULL, 0) ) {
        strip |= handlebars_ast_strip_flag_close_standalone;
    }
    strip |= handlebars_ast_strip_flag_set;
    
    
    if( raw_block->node.raw_block.open_strip & handlebars_ast_strip_flag_right ) {
       handlebars_whitespace_omit_right(program->node.program.statements, NULL, 1);
    }
    
    if( inverse ) {
        unsigned inverse_strip = raw_block->node.raw_block.inverse_strip;
        
        if( inverse_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 1);
        }
        if( inverse_strip & handlebars_ast_strip_flag_right ) {
            handlebars_whitespace_omit_right(inverse->node.program.statements, NULL, 1);
        }
        if( raw_block->node.raw_block.close_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(inverse->node.program.statements, NULL, 1);
        }

        // Find standalone else statments
        if( program && firstInverse &&
                handlebars_whitespace_is_prev_whitespace(program->node.program.statements, NULL, 0) &&
                handlebars_whitespace_is_next_whitespace(firstInverse->node.program.statements, NULL, 0) ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 0);
            handlebars_whitespace_omit_right(firstInverse->node.program.statements, NULL, 0);
        }
    } else {
        if( raw_block->node.raw_block.close_strip & handlebars_ast_strip_flag_left ) {
            handlebars_whitespace_omit_left(program->node.program.statements, NULL, 1);
        }
    }
    
    raw_block->strip = strip;
}

void handlebars_whitespace_accept(struct handlebars_context * context,
        struct handlebars_ast_node * node)
{
    if( unlikely(node == NULL) ) {
        return;
    }
    
    switch( node->type ) {
        case HANDLEBARS_AST_NODE_BLOCK: 
            return handlebars_whitespace_accept_block(context, node);
        case HANDLEBARS_AST_NODE_COMMENT: 
            return handlebars_whitespace_accept_comment(context, node);
        case HANDLEBARS_AST_NODE_MUSTACHE: 
            return handlebars_whitespace_accept_mustache(context, node);
        case HANDLEBARS_AST_NODE_PARTIAL: 
            return handlebars_whitespace_accept_partial(context, node);
        case HANDLEBARS_AST_NODE_PROGRAM: 
            return handlebars_whitespace_accept_program(context, node);
        case HANDLEBARS_AST_NODE_RAW_BLOCK:
            return handlebars_whitespace_accept_raw_block(context, node);
        
        // LCOV_EXCL_START
        // These don't do anything
        case HANDLEBARS_AST_NODE_BOOLEAN:
        case HANDLEBARS_AST_NODE_CONTENT:
        case HANDLEBARS_AST_NODE_HASH:
        case HANDLEBARS_AST_NODE_HASH_PAIR:
        case HANDLEBARS_AST_NODE_NUL:
        case HANDLEBARS_AST_NODE_NUMBER:
        case HANDLEBARS_AST_NODE_PATH:
        case HANDLEBARS_AST_NODE_SEXPR:
        case HANDLEBARS_AST_NODE_STRING:
        case HANDLEBARS_AST_NODE_UNDEFINED:
            break;
        // Note: these should never be used
        case HANDLEBARS_AST_NODE_INTERMEDIATE:
        case HANDLEBARS_AST_NODE_INVERSE:
        case HANDLEBARS_AST_NODE_PATH_SEGMENT:
        case HANDLEBARS_AST_NODE_NIL:
            assert(0);
            break;
        // LCOV_EXCL_STOP
    }
}

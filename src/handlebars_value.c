
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <talloc.h>
#include <yaml.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars.h"
#include "handlebars_map.h"
#include "handlebars_memory.h"
#include "handlebars_private.h"
#include "handlebars_stack.h"
#include "handlebars_utils.h"
#include "handlebars_value.h"
#include "handlebars_value_handlers.h"



#undef CONTEXT
#define CONTEXT HBSCTX(ctx)

struct handlebars_value * handlebars_value_ctor(struct handlebars_context * ctx)
{
    struct handlebars_value * value = MC(handlebars_talloc_zero(ctx, struct handlebars_value));
    value->ctx = CONTEXT;
    value->refcount = 1;
    value->flags = HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED;
    return value;
}

#undef CONTEXT
#define CONTEXT HBSCTX(value->ctx)

enum handlebars_value_type handlebars_value_get_type(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		return handlebars_value_get_handlers(value)->type(value);
	} else {
		return value->type;
	}
}

struct handlebars_value * handlebars_value_map_find(struct handlebars_value * value, struct handlebars_string * key)
{
    if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
        if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
            return handlebars_value_get_handlers(value)->map_find(value, key);
        }
    } else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        return handlebars_map_find(value->v.map, key);
    }

    return NULL;
}

struct handlebars_value * handlebars_value_map_str_find(struct handlebars_value * value, const char * key, size_t len)
{
    struct handlebars_value * ret = NULL;
    struct handlebars_string * str = handlebars_string_ctor(value->ctx, key, len);

	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_MAP ) {
			ret = handlebars_value_get_handlers(value)->map_find(value, str);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_MAP ) {
        ret = handlebars_map_find(value->v.map, str);
    }

    handlebars_talloc_free(str);
	return ret;
}

struct handlebars_value * handlebars_value_array_find(struct handlebars_value * value, size_t index)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_USER ) {
		if( handlebars_value_get_type(value) == HANDLEBARS_VALUE_TYPE_ARRAY ) {
			return handlebars_value_get_handlers(value)->array_find(value, index);
		}
	} else if( value->type == HANDLEBARS_VALUE_TYPE_ARRAY ) {
        return handlebars_stack_get(value->v.stack, index);
    }

	return NULL;
}

struct handlebars_string * handlebars_value_get_stringval(struct handlebars_value * value)
{
    enum handlebars_value_type type = value ? value->type : HANDLEBARS_VALUE_TYPE_NULL;

    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            return handlebars_string_copy_ctor(CONTEXT, value->v.string);
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return handlebars_string_asprintf(CONTEXT, "%ld", value->v.lval);
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return handlebars_string_asprintf(CONTEXT, "%g", value->v.dval);
        case HANDLEBARS_VALUE_TYPE_TRUE:
            return handlebars_string_ctor(CONTEXT, HBS_STRL("true"));
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return handlebars_string_ctor(CONTEXT, HBS_STRL("false"));
        default:
            return handlebars_string_init(CONTEXT, 0);
    }
}

char * handlebars_value_get_strval(struct handlebars_value * value)
{
    char * ret;
    enum handlebars_value_type type = value ? value->type : HANDLEBARS_VALUE_TYPE_NULL;

    switch( type ) {
        case HANDLEBARS_VALUE_TYPE_STRING:
            ret = handlebars_talloc_strndup(value->ctx, HBS_STR_STRL(value->v.string));
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            ret = handlebars_talloc_asprintf(value->ctx, "%ld", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            ret = handlebars_talloc_asprintf(value->ctx, "%g", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            ret = handlebars_talloc_strdup(value->ctx, "true");
            break;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            ret = handlebars_talloc_strdup(value->ctx, "false");
            break;
        default:
            ret = handlebars_talloc_strdup(value->ctx, "");
            break;
    }

    MEMCHK(ret);

    return ret;
}

size_t handlebars_value_get_strlen(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_STRING ) {
		return value->v.string->len;
	}

	return 0;
}

bool handlebars_value_get_boolval(struct handlebars_value * value)
{
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            return false;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            return true;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            return false;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            return value->v.dval != 0;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            return value->v.lval != 0;
        case HANDLEBARS_VALUE_TYPE_STRING:
            return value->v.string->len != 0 && strcmp(value->v.string->val, "0") != 0;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            return handlebars_stack_length(value->v.stack) != 0;
        case HANDLEBARS_VALUE_TYPE_MAP:
            return value->v.map->first != NULL;
        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_count(value) != 0;
        default:
            return false;
    }
}

long handlebars_value_get_intval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_INTEGER ) {
        return value->v.lval;
	}

	return 0;
}

double handlebars_value_get_floatval(struct handlebars_value * value)
{
	if( value->type == HANDLEBARS_VALUE_TYPE_FLOAT ) {
        return value->v.dval;
	}

	return 0;
}

void handlebars_value_convert_ex(struct handlebars_value * value, bool recurse)
{
    struct handlebars_value_iterator it;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_value_get_handlers(value)->convert(value, recurse);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_value_iterator_init(&it, value);
            for( ; it.current != NULL; it.next(&it) ) {
                handlebars_value_convert_ex(it.current, recurse);
            }
            break;
        default:
            // do nothing
            break;
    }
}

static bool handlebars_value_iterator_next_void(struct handlebars_value_iterator * it)
{
    return false;
}

static bool handlebars_value_iterator_next_stack(struct handlebars_value_iterator * it)
{
    struct handlebars_value * value = it->value;

    assert(value != NULL);
    assert(value->type == HANDLEBARS_VALUE_TYPE_ARRAY);

    if( it->current != NULL ) {
        handlebars_value_delref(it->current);
        it->current = NULL;
    }

    if( it->index >= handlebars_stack_length(value->v.stack) - 1 ) {
        return false;
    }

    it->index++;
    it->current = handlebars_stack_get(value->v.stack, it->index);
    return true;
}

static bool handlebars_value_iterator_next_map(struct handlebars_value_iterator * it)
{
    struct handlebars_map_entry * entry;

    assert(it->value != NULL);
    assert(it->value->type == HANDLEBARS_VALUE_TYPE_MAP);

    if( it->current != NULL ) {
        handlebars_value_delref(it->current);
        it->current = NULL;
    }

    entry = talloc_get_type(it->usr, struct handlebars_map_entry);
    if( !entry || !entry->next ) {
        return false;
    }

    it->usr = (void *) (entry = entry->next);
    it->key = entry->key;
    it->current = entry->value;
    handlebars_value_addref(it->current);
}

bool handlebars_value_iterator_init(struct handlebars_value_iterator * it, struct handlebars_value * value)
{
    struct handlebars_map_entry * entry;

    memset(it, 0, sizeof(struct handlebars_value_iterator));

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it->value = value;
            it->current = handlebars_stack_get(value->v.stack, 0);
            it->length = handlebars_stack_length(value->v.stack);
            it->next = &handlebars_value_iterator_next_stack;
            return true;

        case HANDLEBARS_VALUE_TYPE_MAP:
            entry = value->v.map->first;
            if( entry ) {
                it->value = value;
                it->usr = (void *) entry;
                it->key = entry->key;
                it->current = entry->value;
                it->length = value->v.map->i;
                it->next = &handlebars_value_iterator_next_map;
                handlebars_value_addref(it->current);
                return true;
            } else {
                it->next = &handlebars_value_iterator_next_void;
            }
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            return handlebars_value_get_handlers(value)->iterator(it, value);

        default:
            it->next = &handlebars_value_iterator_next_void;
            //handlebars_context_throw(value->ctx, HANDLEBARS_ERROR, "Cannot iterator over type %d", value->type);
            break;
    }

    return false;
}

struct handlebars_value * handlebars_value_call(struct handlebars_value * value, HANDLEBARS_HELPER_ARGS)
{
    struct handlebars_value * result = NULL;
    if( value->type == HANDLEBARS_VALUE_TYPE_HELPER ) {
        result = value->v.helper(argc, argv, options);
    } else if( value->type == HANDLEBARS_VALUE_TYPE_USER && handlebars_value_get_handlers(value)->call ) {
        result = handlebars_value_get_handlers(value)->call(value, argc, argv, options);
    }
    return result;
}

char * handlebars_value_dump(struct handlebars_value * value, size_t depth)
{
    char * buf = MC(handlebars_talloc_strdup(CONTEXT, ""));
    struct handlebars_value_iterator it;
    char indent[64];
    char indent2[64];

    if( value == NULL ) {
        handlebars_talloc_strdup_append_buffer(buf, "(nil)");
        return buf;
    }

    memset(indent, 0, sizeof(indent));
    memset(indent, ' ', depth * 4); // @todo fix

    memset(indent2, 0, sizeof(indent2));
    memset(indent2, ' ', (depth + 1) * 4); // @todo fix

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_NULL:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "NULL");
            break;
        case HANDLEBARS_VALUE_TYPE_TRUE:
            buf = handlebars_talloc_strndup_append_buffer(buf, HBS_STRL("boolean(true)"));
            break;
        case HANDLEBARS_VALUE_TYPE_FALSE:
            buf = handlebars_talloc_strndup_append_buffer(buf, HBS_STRL("boolean(false)"));
            break;
        case HANDLEBARS_VALUE_TYPE_FLOAT:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "float(%f)", value->v.dval);
            break;
        case HANDLEBARS_VALUE_TYPE_INTEGER:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "integer(%ld)", value->v.lval);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "string(%.*s)", (int) value->v.string->len, value->v.string->val);
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "[");
            handlebars_value_iterator_init(&it, value);
            for( ; it.current != NULL; it.next(&it) ) {
                char * tmp = handlebars_value_dump(it.current, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%ld => %s\n", indent2, it.index, tmp);
                handlebars_talloc_free(tmp);
            }
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "]");
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s\n", "{");
            handlebars_value_iterator_init(&it, value);
            for( ; it.current != NULL; it.next(&it) ) {
                char * tmp = handlebars_value_dump(it.current, depth + 1);
                buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%.*s => %s\n", indent2, (int) it.key->len, it.key->val, tmp);
                handlebars_talloc_free(tmp);
            }
            buf = handlebars_talloc_asprintf_append_buffer(buf, "%s%s", indent, "}");
            break;
        default:
            buf = handlebars_talloc_asprintf_append_buffer(buf, "unknown type %d", value->type);
            break;
    }

    return MC(buf);
}

struct handlebars_string * handlebars_value_expression(struct handlebars_value * value, bool escape)
{
    struct handlebars_string * string = handlebars_string_init(value->ctx, 0);
    string = handlebars_value_expression_append(string, value, escape);
    return string;
}

struct handlebars_string * handlebars_value_expression_append(
    struct handlebars_string * string,
    struct handlebars_value * value,
    bool escape
) {
    struct handlebars_value_iterator it;

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_TRUE:
            string = handlebars_string_append(value->ctx, string, HBS_STRL("true"));
            break;

        case HANDLEBARS_VALUE_TYPE_FALSE:
            string = handlebars_string_append(value->ctx, string, HBS_STRL("false"));
            break;

        case HANDLEBARS_VALUE_TYPE_FLOAT:
            string = handlebars_string_asprintf_append(value->ctx, string, "%g", value->v.dval);
            break;

        case HANDLEBARS_VALUE_TYPE_INTEGER:
            string = handlebars_string_asprintf_append(value->ctx, string, "%ld", value->v.lval);
            break;

        case HANDLEBARS_VALUE_TYPE_STRING:
            if( escape && !(value->flags & HANDLEBARS_VALUE_FLAG_SAFE_STRING) ) {
                string = handlebars_string_htmlspecialchars_append(value->ctx, string, HBS_STR_STRL(value->v.string));
            } else {
                string = handlebars_string_append(value->ctx, string, value->v.string->val, value->v.string->len);
            }
            break;

        case HANDLEBARS_VALUE_TYPE_USER:
            if( handlebars_value_get_type(value) != HANDLEBARS_VALUE_TYPE_ARRAY ) {
                break;
            }
            // fall-through to array
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_value_iterator_init(&it, value);
            bool first = true;
            for( ; it.current != NULL; it.next(&it) ) {
                if( !first ) {
                    string = handlebars_string_append(value->ctx, string, HBS_STRL(","));
                }
                string = handlebars_value_expression_append(string, it.current, escape);
                first = false;
            }
            break;

        default:
            // nothing
            break;
    }

    return string;
}

struct handlebars_value * handlebars_value_copy(struct handlebars_value * value)
{
    struct handlebars_value * new_value = NULL;
    struct handlebars_value_iterator it;

    assert(value != NULL);

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            new_value = handlebars_value_ctor(CONTEXT);
            handlebars_value_array_init(new_value);
            handlebars_value_iterator_init(&it, value);
            for( ; it.current != NULL; it.next(&it) ) {
                handlebars_stack_set(new_value->v.stack, it.index, it.current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            new_value = handlebars_value_ctor(CONTEXT);
            handlebars_value_map_init(new_value);
            handlebars_value_iterator_init(&it, value);
            for( ; it.current != NULL; it.next(&it) ) {
                handlebars_map_update(new_value->v.map, it.key, it.current);
            }
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            new_value = handlebars_value_get_handlers(value)->copy(value);
            break;

        default:
            new_value = handlebars_value_ctor(CONTEXT);
            memcpy(&new_value->v, &value->v, sizeof(value->v));
            break;
    }

    return MC(new_value);
}

void handlebars_value_dtor(struct handlebars_value * value)
{
    long restore_flags = 0;

    // Release old value
    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            handlebars_stack_dtor(value->v.stack);
            break;
        case HANDLEBARS_VALUE_TYPE_MAP:
            handlebars_map_dtor(value->v.map);
            break;
        case HANDLEBARS_VALUE_TYPE_STRING:
            handlebars_talloc_free(value->v.string);
            break;
        case HANDLEBARS_VALUE_TYPE_USER:
            handlebars_value_get_handlers(value)->dtor(value);
            break;
        case HANDLEBARS_VALUE_TYPE_PTR:
            handlebars_talloc_free(value->v.ptr);
            break;
        default:
            // do nothing
            break;
    }

    if( value->flags & HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED ) {
        talloc_free_children(value);
        restore_flags = HANDLEBARS_VALUE_FLAG_HEAP_ALLOCATED;
    }

    // Initialize to null
    value->type = HANDLEBARS_VALUE_TYPE_NULL;
    memset(&value->v, 0, sizeof(value->v));
    value->flags = restore_flags;
}


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "handlebars_value.h"
#include "handlebars_vm.h"

#define FIXTURE_FN(hash) static struct handlebars_value * fixture_ ## hash(struct handlebars_options * options)
#define FIXTURE_STRING(string) \
    struct handlebars_value * value = handlebars_value_ctor(options->vm); \
    handlebars_value_string(value, string); \
    return value;
#define FIXTURE_INTEGER(integer) \
    struct handlebars_value * value = handlebars_value_ctor(options->vm); \
    handlebars_value_integer(value, integer); \
    return value;

FIXTURE_FN(49286285)
{
    struct handlebars_value * arg = handlebars_stack_get(options->params, 0);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * r1 = handlebars_value_get_strval(arg);
    char * r2 = handlebars_talloc_strdup(result, "bar");
    handlebars_talloc_strdup_append(r2, r1);
    handlebars_value_string(result, r2);
    return result;
}

FIXTURE_FN(662835958)
{
    // "function () { return {first: 'Alan', last: 'Johnson'}; }",
    struct handlebars_value * value = handlebars_value_from_json_string(options->vm, "{\"first\": \"Alan\", \"last\": \"Johnson\"}");
    handlebars_value_convert(value);
    return value;
}

FIXTURE_FN(739773491)
{
    // "function (arg) {\n        return arg;\n      }"
    return handlebars_stack_get(options->params, 0);
}

FIXTURE_FN(788468697) {
    assert(options->scope != NULL);
    assert(options->scope->type == HANDLEBARS_VALUE_TYPE_STRING);
    FIXTURE_INTEGER(strlen(options->scope->v.strval));
}

FIXTURE_FN(929767352)
{
    // "function (options) {\n        return options.data.adjective + ' world' + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_find(options->data, "adjective");
    struct handlebars_value * exclaim = handlebars_value_map_find(options->scope, "exclaim");
    char * ret = handlebars_talloc_asprintf(
            options->vm,
            "%s world%s",
            handlebars_value_get_strval(adjective),
            handlebars_value_is_empty(exclaim) ? "" : "!"
    );
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, ret);
    handlebars_talloc_free(ret);
    return result;
}

FIXTURE_FN(931412676)
{
    // "function (options) {\n            var frame = Handlebars.createFrame(options.data);\n            frame.depth = options.data.depth + 1;\n            return options.fn(this, {data: frame});\n          }"
    struct handlebars_value * frame = handlebars_value_ctor(options->vm);
    handlebars_value_map_init(frame);
    struct handlebars_value_iterator * it = handlebars_value_iterator_ctor(options->data);
    for( ; it->current; handlebars_value_iterator_next(it) ) {
        if( 0 == strcmp(it->key, "depth") ) {
            struct handlebars_value * tmp = handlebars_value_ctor(options->vm);
            handlebars_value_integer(tmp, handlebars_value_get_intval(it->current) + 1);
            handlebars_map_add(frame->v.map, it->key, tmp);
            handlebars_value_delref(tmp);
        } else {
            handlebars_map_add(frame->v.map, it->key, it->current);
        }
    }
    handlebars_map_update(frame->v.map, "_parent", options->data);
    char * res = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, frame, NULL);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(1198465479)
{
    // "function (noun, options) {\n        return options.data.adjective + ' ' + noun + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_find(options->data, "adjective");
    struct handlebars_value * noun = handlebars_stack_get(options->params, 0);
    struct handlebars_value * exclaim = handlebars_value_map_find(options->scope, "exclaim");
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * tmp = handlebars_talloc_asprintf(
            result,
            "%s %s%s",
            handlebars_value_get_strval(adjective),
            handlebars_value_get_strval(noun),
            (handlebars_value_get_boolval(exclaim) ? "!" : "")
    );
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(1283397100)
{
    // "function (options) {\n        return options.fn({exclaim: '?'});\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(options->vm, "{\"exclaim\": \"?\"}");
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * tmp = handlebars_vm_execute_program(options->vm, options->program, context);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(1341397520)
{
    // "function (options) {\n        return options.data && options.data.exclaim;\n      }"
    if( options->data ) {
        return handlebars_value_map_find(options->data, "exclaim");
    } else {
        return handlebars_value_ctor(options->vm);
    }
}

FIXTURE_FN(1623791204)
{
    struct handlebars_value * noun = handlebars_value_map_find(options->hash, "noun");
    char * tmp = handlebars_value_get_strval(noun);
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(1872958178)
{
    // "function (options) {\n        return options.fn(this);\n      }"
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * tmp = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2084318034)
{
    // This is a dumb test
    // "function (_undefined, _null, options) {\n            return (_undefined === undefined) + ' ' + (_null === null) + ' ' + (typeof options);\n          }"
    struct handlebars_value * arg1 = handlebars_stack_get(options->params, 0);
    struct handlebars_value * arg2 = handlebars_stack_get(options->params, 1);
    char * res = handlebars_talloc_asprintf(options->vm, "%s %s %s",
                                            (arg1->type == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            (arg1->type == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            "object");
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_string(value, res);
    handlebars_talloc_free(res);
    return value;
}

FIXTURE_FN(2096893161)
{
    // "function () {\n            return 'null!';\n          }"
    FIXTURE_STRING("null!");
}

FIXTURE_FN(2259424295)
{
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_string(value, "&'\\<>");
    value->flags |= HANDLEBARS_VALUE_FLAG_SAFE_STRING;
    return value;
}

FIXTURE_FN(2305563493)
{
    // "function () { return [{text: 'goodbye'}, {text: 'Goodbye'}, {text: 'GOODBYE'}]; }"
    struct handlebars_value * value = handlebars_value_from_json_string(options->vm, "[{\"text\": \"goodbye\"}, {\"text\": \"Goodbye\"}, {\"text\": \"GOODBYE\"}]");
    handlebars_value_convert(value);
    return value;
}

FIXTURE_FN(2439252451)
{
    struct handlebars_value * value = options->scope;
    handlebars_value_addref(value);
    return value;
}

FIXTURE_FN(2499873302)
{
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_boolean(value, 0);
    return value;
}

FIXTURE_FN(2554595758)
{
    // "function () { return 'bar'; }"
    FIXTURE_STRING("bar");
}

FIXTURE_FN(2596410860)
{
    // "function (context, options) { return options.fn(context); }"
    struct handlebars_value * context = handlebars_stack_get(options->params, 0);
    char * res = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_string(value, res);
    handlebars_talloc_free(res);
    return value;
}

FIXTURE_FN(2818908139)
{
    // "function (options) {\n        return options.fn({exclaim: '?'}, { data: {adjective: 'sad'} });\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(options->vm, "{\"exclaim\": \"?\"}");
    struct handlebars_value * data = handlebars_value_from_json_string(options->vm, "{\"adjective\": \"sad\"}");
    struct handlebars_value * exclaim = handlebars_value_ctor(options->vm);
    handlebars_value_string(exclaim, "!");
    char * res = handlebars_vm_execute_program_ex(options->vm, options->program, context, data, NULL);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(2919388099)
{
    // "function (options) {\n        var frame = Handlebars.createFrame(options.data);\n        for (var prop in options.hash) {\n          if (prop in options.hash) {\n            frame[prop] = options.hash[prop];\n          }\n        }\n        return options.fn(this, {data: frame});\n      }"
    struct handlebars_value * frame = handlebars_value_ctor(options->vm);
    handlebars_value_map_init(frame);
    struct handlebars_value_iterator *it = handlebars_value_iterator_ctor(options->data);
    for (; it->current; handlebars_value_iterator_next(it)) {
        handlebars_map_add(frame->v.map, it->key, it->current);
    }
    struct handlebars_value_iterator *it2 = handlebars_value_iterator_ctor(options->hash);
    for (; it2->current; handlebars_value_iterator_next(it2)) {
        handlebars_map_update(frame->v.map, it2->key, it2->current);
    }
    handlebars_map_update(frame->v.map, "_parent", options->data);
    char * res = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, frame, NULL);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(2927692429)
{
    // "function () { return 'hello'; }"
    FIXTURE_STRING("hello");
}

FIXTURE_FN(2961119846)
{
    // "function (options) {\n        return options.data.adjective + ' ' + this.noun;\n      }"
    struct handlebars_value * adjective = handlebars_value_map_find(options->data, "adjective");
    struct handlebars_value * noun = handlebars_value_map_find(options->scope, "noun");
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * tmp = handlebars_talloc_asprintf(
            result,
            "%s %s",
            handlebars_value_get_strval(adjective),
            handlebars_value_get_strval(noun)
    );
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3058305845)
{
    // "function () {return this.foo; }"
    struct handlebars_value * value = handlebars_value_map_find(options->scope, "foo");
    if( !value ) {
        value = handlebars_value_ctor(options->vm);
    }
    return value;
}

FIXTURE_FN(3153085867)
{
    // @tod remove me?
}

FIXTURE_FN(3168412868)
{
    // "function (options) {\n        return options.fn({exclaim: '?', zomg: 'world'}, { data: {adjective: 'sad'} });\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(options->vm, "{\"exclaim\":\"?\", \"zomg\":\"world\"}");
    struct handlebars_value * data = handlebars_value_from_json_string(options->vm, "{\"adjective\": \"sad\"}");
    char * res = handlebars_vm_execute_program_ex(options->vm, options->program, context, data, NULL);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(3307473738)
{
    FIXTURE_STRING("Awesome");
}

FIXTURE_FN(3379432388)
{
    // "function () { return this.more; }"
    struct handlebars_value * value = handlebars_value_map_find(options->scope, "more");
    if( !value ) {
        value = handlebars_value_ctor(options->vm);
    }
    return value;
}

FIXTURE_FN(3578728160)
{
    // "function () {\n            return 'undefined!';\n          }"
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_string(value, "undefined!");
    return value;
}

FIXTURE_FN(3659403207)
{
    // "function (value) {\n        return 'bar ' + value;\n    }"
    struct handlebars_value * arg = handlebars_stack_get(options->params, 0);
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    char * r1 = handlebars_value_get_strval(arg);
    char * r2 = handlebars_talloc_strdup(result, "bar ");
    handlebars_talloc_strdup_append(r2, r1);
    handlebars_value_string(result, r2);
    return result;
}

FIXTURE_FN(3707047013)
{
    // "function (value) { return value; }"
    struct handlbars_value * value = handlebars_stack_get(options->params, 0);
    handlebars_value_addref(value);
    return value;
}

FIXTURE_FN(3728875550)
{
    // "function (options) {\n        return options.data.accessData + ' ' + options.fn({exclaim: '?'});\n      }"
    struct handlebars_value * access_data = handlebars_value_map_find(options->data, "accessData");
    struct handlebars_value * context = handlebars_value_from_json_string(options->vm, "{\"exclaim\": \"?\"}");
    char * ret = handlebars_vm_execute_program(options->vm, options->program, context);
    ret = handlebars_talloc_asprintf(
            options->vm,
            "%s %s",
            handlebars_value_get_strval(access_data),
            ret
    );
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, ret);
    handlebars_talloc_free(ret);
    return result;
}

FIXTURE_FN(4112130635)
{
    // "function (thing, options) {\n        return options.data.adjective + ' ' + thing + (this.exclaim || '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_find(options->data, "adjective");
    struct handlebars_value * thing = handlebars_stack_get(options->params, 0);
    struct handlebars_value * exclaim = handlebars_value_map_find(options->scope, "exclaim");
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "%s %s%s",
            handlebars_value_get_strval(adjective),
            handlebars_value_get_strval(thing),
            handlebars_value_get_strval(exclaim)
    );
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(4158918668)
{
    struct handlebars_value * noun = handlebars_stack_get(options->params, 0);
    char * tmp = handlebars_value_get_strval(noun);
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    struct handlebars_value * result = handlebars_value_ctor(options->vm);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

static void convert_value_to_fixture(struct handlebars_value * value)
{
#define FIXTURE_CASE(hash) \
    case hash: \
        SET_FUNCTION(fixture_ ## hash); \
        break
#define FIXTURE_CASE_ALIAS(hash1, hash2) \
    case hash1: \
        SET_FUNCTION(fixture_ ## hash2); \
        break;

#define SET_FUNCTION(func) \
    handlebars_value_null(value); \
    value->type = HANDLEBARS_VALUE_TYPE_HELPER; \
    value->v.helper = &func;

    assert(value->type == HANDLEBARS_VALUE_TYPE_MAP);

    struct handlebars_value * jsvalue = handlebars_value_map_find(value, "javascript");
    if( !jsvalue ) {
        jsvalue = handlebars_value_map_find(value, "php");
    }
    assert(jsvalue != NULL);
    assert(jsvalue->type == HANDLEBARS_VALUE_TYPE_STRING);
    uint32_t hash = adler32(jsvalue->v.strval, strlen(jsvalue->v.strval));

    switch( hash ) {
        FIXTURE_CASE(49286285);
        FIXTURE_CASE(662835958);
        FIXTURE_CASE(739773491);
        FIXTURE_CASE(788468697);
        FIXTURE_CASE(929767352);
        FIXTURE_CASE(931412676);
        FIXTURE_CASE(1198465479);
        FIXTURE_CASE(1283397100);
        FIXTURE_CASE(1341397520);
        FIXTURE_CASE(1623791204);
        FIXTURE_CASE(1872958178);
        FIXTURE_CASE(2084318034);
        FIXTURE_CASE(2096893161);
        FIXTURE_CASE(2259424295);
        FIXTURE_CASE(2305563493);
        FIXTURE_CASE(2439252451);
        FIXTURE_CASE(2499873302);
        FIXTURE_CASE(2554595758);
        FIXTURE_CASE(2596410860);
        FIXTURE_CASE(2818908139);
        FIXTURE_CASE(2919388099);
        FIXTURE_CASE(2927692429);
        FIXTURE_CASE(2961119846);
        FIXTURE_CASE(3058305845);
        FIXTURE_CASE_ALIAS(3153085867, 2919388099);
        FIXTURE_CASE(3168412868);
        FIXTURE_CASE(3307473738);
        FIXTURE_CASE(3379432388);
        FIXTURE_CASE(3578728160);
        FIXTURE_CASE(3659403207);
        FIXTURE_CASE(3707047013);
        FIXTURE_CASE(3728875550);
        FIXTURE_CASE(4112130635);
        FIXTURE_CASE(4158918668);

        FIXTURE_CASE_ALIAS(401083957, 3707047013);
        FIXTURE_CASE_ALIAS(1111103580, 1341397520);
        FIXTURE_CASE_ALIAS(2836204191, 739773491);
        default:
            fprintf(stderr, "Unimplemented test fixture [%u]:\n%s\n", hash, jsvalue->v.strval);
            return;
    }

#ifndef NDEBUG
    fprintf(stderr, "Got fixture [%u]\n", hash);
#endif

#undef SET_FUNCTION
}

void load_fixtures(struct handlebars_value * value)
{
    struct handlebars_value_iterator * it;
    struct handlebars_value * child;

    // This shouldn't happen ...
    assert(value != NULL);

    handlebars_value_convert(value);

    switch( value->type ) {
        case HANDLEBARS_VALUE_TYPE_MAP:
            // Check if it contains a "!code" key
            child = handlebars_value_map_find(value, "!code");
            if( child ) {
                // Convert to helper
                convert_value_to_fixture(value);
            } else {
                // Recurse
                it = handlebars_value_iterator_ctor(value);
                for( ; it && it->current != NULL; handlebars_value_iterator_next(it) ) {
                    load_fixtures(it->current);
                }
            }
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            it = handlebars_value_iterator_ctor(value);
            for( ; it && it->current != NULL; handlebars_value_iterator_next(it) ) {
                load_fixtures(it->current);
            }
            break;
    }
}

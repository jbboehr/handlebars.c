/**
 * Copyright (c) anno Domini nostri Jesu Christi MMXVI-MMXXIV John Boehr & contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "handlebars.h"
#include "handlebars_memory.h"

#include "handlebars_helpers.h"
#ifdef HANDLEBARS_HAVE_JSON
#include "handlebars_json.h"
#endif
#include "handlebars_map.h"
#include "handlebars_stack.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "utils.h"

// @TODO FIXME
#pragma GCC diagnostic ignored "-Wshadow"



#define CONTEXT ((struct handlebars_context *)vm)
#define FIXTURE_FN(hash) static struct handlebars_value * fixture_ ## hash(HANDLEBARS_HELPER_ARGS)
#define FIXTURE_STRING(string) \
    handlebars_value_cstrl(rv, HBS_STRL(string)); \
    return rv;
#define FIXTURE_INTEGER(integer) \
    handlebars_value_integer(rv, integer); \
    return rv;
#define VALUE_TO_STRING(value) handlebars_value_to_string(value, CONTEXT)

static inline void cstr_steal(struct handlebars_context * context, struct handlebars_value * value, char * strval) {
    struct handlebars_string * string = handlebars_string_ctor(context, strval, strlen(strval));
    handlebars_value_str(value, string);
    handlebars_talloc_free(strval);
}
#define handlebars_value_cstr_steal(value, strval) cstr_steal(CONTEXT, value, strval)

static inline void cstrl(struct handlebars_context * context, struct handlebars_value * value, const char * strval, size_t strlen) {
    struct handlebars_string * string = handlebars_string_ctor(context, strval, strlen);
    handlebars_value_str(value, string);
}
#define handlebars_value_cstrl(value, ...) cstrl(CONTEXT, value, __VA_ARGS__)

#ifndef HANDLEBARS_HAVE_JSON
static struct handlebars_value * handlebars_value_init_json_string(
    struct handlebars_context *ctx,
    struct handlebars_value * value,
    const char * json
) {
    fprintf(stderr, "JSON is disabled");
    abort();
}
#endif

HBS_ATTR_NONNULL_ALL
static struct handlebars_value * map_str_find(
    struct handlebars_value * value,
    const char * key,
    size_t len
) {
    assert(NULL != handlebars_value_get_map(value));
    struct handlebars_value * tmp = handlebars_map_str_find(handlebars_value_get_map(value), key, len);
    if (tmp != NULL) {
        return tmp;
    }
    return NULL;
};


FIXTURE_FN(20974934)
{
    // "function (arg) { return typeof arg; }"
    struct handlebars_value * arg = HANDLEBARS_ARG_AT(0);
    if( handlebars_value_get_type(arg) == HANDLEBARS_VALUE_TYPE_NULL ) {
        FIXTURE_STRING("undefined");
    } else {
        FIXTURE_STRING("not undefined");
    }
}

FIXTURE_FN(49286285)
{
    struct handlebars_value * arg = HANDLEBARS_ARG_AT(0);
    handlebars_value_cstr_steal(rv, handlebars_talloc_asprintf(
            CONTEXT, "%s%s", "bar",  hbs_str_val(VALUE_TO_STRING(arg))
    ));
    return rv;
}

FIXTURE_FN(126946175)
{
    // "function () {\n        return true;\n      }"
    handlebars_value_boolean(rv, 1);
    return rv;
}

FIXTURE_FN(169151220)
{
    // "function () {\n        return 'LOL';\n      }"
    FIXTURE_STRING("LOL");
}

FIXTURE_FN(276040703)
{
    // "function () {\n      return 'Colors';\n    }"
    FIXTURE_STRING("Colors");
}

FIXTURE_FN(454102302)
{
    // "function (value) { return value + ''; }"
    // @todo implement undefined?
    struct handlebars_value * prefix = HANDLEBARS_ARG_AT(0);
    if (handlebars_value_get_type(prefix) == HANDLEBARS_VALUE_TYPE_NULL) {
        handlebars_value_cstrl(rv, HBS_STRL("undefined"));
    } else {
        handlebars_value_str(rv, handlebars_value_expression(CONTEXT, prefix, 1));
    }
    return rv;
}

FIXTURE_FN(459219799)
{
    // "function (prefix, options) {\n        return '<a href=\"' + prefix + '\/' + this.url + '\">' + options.fn(this) + '<\/a>';\n    }"
    struct handlebars_value * prefix = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * url = map_str_find(options->scope, HBS_STRL("url"));
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            CONTEXT,
            "<a href=\"%s/%s\">%s</a>",
            hbs_str_val(VALUE_TO_STRING(prefix)),
            hbs_str_val(VALUE_TO_STRING(url)),
            hbs_str_val(res)
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));

    handlebars_talloc_free(tmp);
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(461441956)
{
    // "function () {\n        return 'blah';\n      }"
    FIXTURE_STRING("blah");
}

FIXTURE_FN(464915369)
{
    // "function () {\n        return 'fail';\n      }"
    FIXTURE_STRING("fail");
}

FIXTURE_FN(471141295)
{
    // "function () {\n        return 'dude';\n      }"
    FIXTURE_STRING("dude");
}

static int value_for_510017722 = 1;
FIXTURE_FN(510017722)
{
    // "function (options) {\n          if( typeof value === 'undefined' ) { value = 1; } return options.fn({value: 'bar'}, {blockParams: options.fn.blockParams === 1 ? [value++, value++] : undefined});\n        }"
    HANDLEBARS_VALUE_DECL(context);
    HANDLEBARS_VALUE_DECL(bp1);
    HANDLEBARS_VALUE_DECL(bp2);
    HANDLEBARS_VALUE_DECL(block_params);

    handlebars_value_init_json_string(CONTEXT, context, "{\"value\": \"bar\"}");
    handlebars_value_integer(bp1, value_for_510017722++);
    handlebars_value_integer(bp2, value_for_510017722++);

    handlebars_value_array(block_params, handlebars_stack_ctor(CONTEXT, 2));
    handlebars_value_array_push(block_params, bp1); // @TODO ignoring return value - should probably make a handlebars_value_push()
    handlebars_value_array_push(block_params, bp2); // @TODO ignoring return value - should probably make a handlebars_value_push()

    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(vm, options->program, context, NULL, block_params);
    handlebars_value_str(rv, tmp);

    HANDLEBARS_VALUE_UNDECL(block_params);
    HANDLEBARS_VALUE_UNDECL(bp2);
    HANDLEBARS_VALUE_UNDECL(bp1);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(585442881)
{
    // "function (cruel, world, options) {\n        return options.fn({greeting: 'Goodbye', adj: cruel, noun: world});\n      }"
    struct handlebars_value * cruel = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * world = HANDLEBARS_ARG_AT(1);
    HANDLEBARS_VALUE_DECL(greeting);
    handlebars_value_cstrl(greeting, HBS_STRL("Goodbye"));
    struct handlebars_map * context_map = handlebars_map_ctor(CONTEXT, 3);
    context_map = handlebars_map_str_update(context_map, HBS_STRL("greeting"), greeting);
    context_map = handlebars_map_str_update(context_map, HBS_STRL("adj"), cruel);
    context_map = handlebars_map_str_update(context_map, HBS_STRL("noun"), world);
    HANDLEBARS_VALUE_UNDECL(greeting);
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_map(context, context_map);
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, context);
    HANDLEBARS_VALUE_UNDECL(context);
    handlebars_value_str(rv, tmp);

    return rv;
}

FIXTURE_FN(620640779)
{
    // "function (times, times2) {\n      if (typeof times !== 'number') { times = 'NaN'; }\n      if (typeof times2 !== 'number') { times2 = 'NaN'; }\n      return 'Hello ' + times + ' ' + times2 + ' times';\n    }"
    struct handlebars_value * times = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * times2 = HANDLEBARS_ARG_AT(1);
    // @todo this should be a float perhaps?
    /* if( times->type != HANDLEBARS_VALUE_TYPE_FLOAT || times->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_cstrl(times, HBS_STRL("NaN"));
    }
    if( times2->type != HANDLEBARS_VALUE_TYPE_FLOAT || times2->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_cstrl(times2, HBS_STRL("NaN"));
    } */
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "Hello %s %s times",
            hbs_str_val(VALUE_TO_STRING(times)),
            hbs_str_val(VALUE_TO_STRING(times2))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(620828131)
{
    // "function (cruel, world) { return 'Goodbye ' + cruel + ' ' + world; }"
    struct handlebars_value * cruel = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * world = HANDLEBARS_ARG_AT(1);

    char * tmp = handlebars_talloc_asprintf(
            vm,
            "Goodbye %s %s",
            hbs_str_val(VALUE_TO_STRING(cruel)),
            hbs_str_val(VALUE_TO_STRING(world))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(665715952)
{
    // "function () {}"
    return rv;
}

FIXTURE_FN(662835958)
{
    // "function () { return {first: 'Alan', last: 'Johnson'}; }",
    handlebars_value_init_json_string(CONTEXT, rv, "{\"first\": \"Alan\", \"last\": \"Johnson\"}");
    handlebars_value_convert(rv);
    return rv;
}

FIXTURE_FN(690821881)
{
    // this one is actually a partial
    // "function partial(context) {\n      return context.name + ' (' + context.url + ') ';\n    }"
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * name = map_str_find(context, HBS_STRL("name"));
    struct handlebars_value * url = map_str_find(context, HBS_STRL("url"));
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s (%s) ",
            hbs_str_val(VALUE_TO_STRING(name)),
            hbs_str_val(VALUE_TO_STRING(url))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(666457330)
{
    // "function (options) {\n          if (options.hash.print === true) {\n            return 'GOODBYE ' + options.hash.cruel + ' ' + options.fn(this);\n          } else if (options.hash.print === false) {\n            return 'NOT PRINTING';\n          } else {\n            return 'THIS SHOULD NOT HAPPEN';\n          }\n        }"
    struct handlebars_value * print = map_str_find(options->hash, HBS_STRL("print"));
    if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE ) {
        struct handlebars_value * cruel = map_str_find(options->hash, HBS_STRL("cruel"));
        struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
        char * tmp = handlebars_talloc_asprintf(
                vm,
                "GOODBYE %s %s",
                hbs_str_val(VALUE_TO_STRING(cruel)),
                hbs_str_val(res)
        );
        handlebars_talloc_free(res);
        handlebars_value_cstrl(rv, tmp, strlen(tmp));
        handlebars_talloc_free(tmp);
        return rv;
    } else if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_FALSE ) {
        FIXTURE_STRING("NOT PRINTING");
    } else {
        FIXTURE_STRING("THIS SHOULD NOT HAPPEN");
    }
}

FIXTURE_FN(730081660)
{
    // "function (options) {\n          return 'GOODBYE ' + options.hash.cruel + ' ' + options.fn(this) + ' ' + options.hash.times + ' TIMES';\n        }"
    struct handlebars_value * cruel = map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * times = map_str_find(options->hash, HBS_STRL("times"));
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "GOODBYE %s %s %s TIMES",
            hbs_str_val(VALUE_TO_STRING(cruel)),
            hbs_str_val(res),
            hbs_str_val(VALUE_TO_STRING(times))
    );
    handlebars_talloc_free(res);
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(730672213)
{
    if( 0 == strcmp(hbs_str_val(options->name), "link_to") ) {
        struct handlebars_value * mesg = HANDLEBARS_ARG_AT(0);
        char * tmp = handlebars_talloc_asprintf(
                vm,
                "<a>%s</a>",
                hbs_str_val(VALUE_TO_STRING(mesg))
        );
        handlebars_value_cstrl(rv, tmp, strlen(tmp));
        handlebars_talloc_free(tmp);
        handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    }

    return rv;
}

FIXTURE_FN(739773491)
{
    // "function (arg) {\n        return arg;\n      }"
    return HANDLEBARS_ARG_AT(0);
}

FIXTURE_FN(748362646)
{
    // "function (options) { return '<a href=\"' + this.name + '\">' + options.fn(this) + '<\/a>'; }"
    struct handlebars_value * name = map_str_find(options->scope, HBS_STRL("name"));
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "<a href=\"%s\">%s</a>",
            hbs_str_val(VALUE_TO_STRING(name)),
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(788468697)
{
    assert(options->scope != NULL);
    assert(handlebars_value_get_type(options->scope) == HANDLEBARS_VALUE_TYPE_STRING);
    FIXTURE_INTEGER(handlebars_value_get_strlen(options->scope));
}

FIXTURE_FN(902433745)
{
    // "function () {\n          return 'foo';\n        }"
    FIXTURE_STRING("foo");
}

FIXTURE_FN(922226146)
{
    // "function (block) { return block.fn(''); }"
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_cstrl(context, HBS_STRL(""));
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, context);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(929767352)
{
    // "function (options) {\n        return options.data.adjective + ' world' + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * exclaim = map_str_find(options->scope, HBS_STRL("exclaim"));
    char * ret = handlebars_talloc_asprintf(
            vm,
            "%s world%s",
            hbs_str_val(VALUE_TO_STRING(adjective)),
            handlebars_value_is_empty(exclaim) ? "" : "!"
    );
    handlebars_value_cstrl(rv, ret, strlen(ret));
    handlebars_talloc_free(ret);
    return rv;
}

FIXTURE_FN(931412676)
{
    // "function (options) {\n            var frame = Handlebars.createFrame(options.data);\n            frame.depth = options.data.depth + 1;\n            return options.fn(this, {data: frame});\n          }"
    struct handlebars_map * map = handlebars_map_ctor(CONTEXT, handlebars_value_count(options->data) + 1);

    HANDLEBARS_VALUE_FOREACH_KV(options->data, key, child) {
        if( 0 == strcmp(hbs_str_val(key), "depth") ) {
            HANDLEBARS_VALUE_DECL(tmp);
            handlebars_value_integer(tmp, handlebars_value_get_intval(child) + 1);
            map = handlebars_map_update(map, key, tmp);
            HANDLEBARS_VALUE_UNDECL(tmp);
        } else {
            map = handlebars_map_update(map, key, child);
        }
    } HANDLEBARS_VALUE_FOREACH_END();

    map = handlebars_map_str_update(map, HBS_STRL("_parent"), options->data);

    HANDLEBARS_VALUE_DECL(frame);
    handlebars_value_map(frame, map);

    struct handlebars_string * res = handlebars_vm_execute_program_ex(vm, options->program, options->scope, frame, NULL);
    handlebars_value_str(rv, res);

    HANDLEBARS_VALUE_UNDECL(frame);

    return rv;
}

FIXTURE_FN(958795451)
{
    struct handlebars_value * truthiness = map_str_find(options->scope, HBS_STRL("truthiness"));
    if( handlebars_value_is_callable(truthiness) ) {
        return handlebars_value_call(truthiness, argc, argv, options, vm, rv);
    }
    return truthiness; // @TODO possibly wrong
}

FIXTURE_FN(1211570580)
{
    // "function () {\n        return 'ran: ' + arguments[arguments.length - 1].name;\n      }"
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "ran: %s",
            hbs_str_val(options->name)
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(1250888967)
{
    FIXTURE_STRING("missing");
}

FIXTURE_FN(1091971719)
{
    // "function (options) {\n          if (options.name === 'link_to') {\n            return new Handlebars.SafeString('<a>winning<\/a>');\n          }\n        }"
    if( 0 == strcmp(hbs_str_val(options->name), "link_to") ) {
        char * tmp = handlebars_talloc_asprintf(
                vm,
                "<a>%s</a>",
                "winning"
        );
        handlebars_value_cstrl(rv, tmp, strlen(tmp));
        handlebars_talloc_free(tmp);
        handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    }

    return rv;
}

FIXTURE_FN(1041501180)
{
    // "function (a, b) {\n        return a + b;\n      }"
    struct handlebars_value * a = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * b = HANDLEBARS_ARG_AT(1);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s",
            hbs_str_val(VALUE_TO_STRING(a)),
            hbs_str_val(VALUE_TO_STRING(b))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(1102272015)
{
    struct handlebars_value * print = map_str_find(options->hash, HBS_STRL("print"));
    if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE ) {
        struct handlebars_value * cruel = map_str_find(options->hash, HBS_STRL("cruel"));
        struct handlebars_value * world = map_str_find(options->hash, HBS_STRL("world"));
        char * tmp = handlebars_talloc_asprintf(
                vm,
                "GOODBYE %s %s",
                hbs_str_val(VALUE_TO_STRING(cruel)),
                hbs_str_val(VALUE_TO_STRING(world))
        );
        handlebars_value_cstrl(rv, tmp, strlen(tmp));
        handlebars_talloc_free(tmp);
        return rv;
    } else if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_FALSE ) {
        FIXTURE_STRING("NOT PRINTING");
    } else {
        FIXTURE_STRING("THIS SHOULD NOT HAPPEN");
    }
}

FIXTURE_FN(1198465479)
{
    // "function (noun, options) {\n        return options.data.adjective + ' ' + noun + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * noun = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * exclaim = map_str_find(options->scope, HBS_STRL("exclaim"));
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s %s%s",
            hbs_str_val(VALUE_TO_STRING(adjective)),
            hbs_str_val(VALUE_TO_STRING(noun)),
            (handlebars_value_get_boolval(exclaim) ? "!" : "")
    );
    handlebars_value_cstr_steal(rv, tmp);
    return rv;
}

FIXTURE_FN(1252527367)
{
    // "function () {\n        return 'winning';\n      }"
    FIXTURE_STRING("winning");
}

FIXTURE_FN(1283397100)
{
    // "function (options) {\n        return options.fn({exclaim: '?'});\n      }"
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_init_json_string(CONTEXT, context, "{\"exclaim\": \"?\"}");
    handlebars_value_convert(context); // @TODO we shouldn't have to do this
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, context);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(1341397520)
{
    // "function (options) {\n        return options.data && options.data.exclaim;\n      }"
    if( options->data ) {
        return map_str_find(options->data, HBS_STRL("exclaim"));
    } else {
        return rv;
    }
}

FIXTURE_FN(1582700088)
{
    struct handlebars_value * fun = map_str_find(options->hash, HBS_STRL("fun"));
    char * res = handlebars_talloc_asprintf(
            vm,
            "val is %s",
            hbs_str_val(VALUE_TO_STRING(fun))
    );
    handlebars_value_cstrl(rv, res, strlen(res));
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(1623791204)
{
    struct handlebars_value * noun = map_str_find(options->hash, HBS_STRL("noun"));
    char * tmp = hbs_str_val(VALUE_TO_STRING(noun));
    char * res = handlebars_talloc_asprintf(
            vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    handlebars_value_cstrl(rv, res, strlen(res));
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(1644694756)
{
    // "function (x, y) {\n        return x === y;\n      }"
    struct handlebars_value * x = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * y = HANDLEBARS_ARG_AT(1);
    bool ret = false;
    if( handlebars_value_get_type(x) != handlebars_value_get_type(y) ) {
        // nothing
    // } else if( memcmp(&x->v, &y->v, sizeof(x->v)) ) {
    //     ret = true;
    // } else if( x->type == HANDLEBARS_VALUE_TYPE_MAP ||
    //         x->type == HANDLEBARS_VALUE_TYPE_ARRAY ||
    //         y->type == HANDLEBARS_VALUE_TYPE_MAP ||
    //         y->type == HANDLEBARS_VALUE_TYPE_MAP ) {
    //     // nothing
    } else if (handlebars_string_eq(VALUE_TO_STRING(x), VALUE_TO_STRING(y))) {
        ret = true;
    }
    handlebars_value_boolean(rv, ret);
    return rv;
}

FIXTURE_FN(1774917451)
{
    // "function (options) { return '<form>' + options.fn(this) + '<\/form>'; }"
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "<form>%s</form>",
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(1818365722)
{
    // "function (param) { return 'Hello ' + param; }"
    struct handlebars_value * param = HANDLEBARS_ARG_AT(0);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "Hello %s",
            hbs_str_val(VALUE_TO_STRING(param))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(1872958178)
{
    // "function (options) {\n        return options.fn(this);\n      }"
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, options->scope);
    handlebars_value_str(rv, tmp);
    return rv;
}

FIXTURE_FN(1983911259)
{
    struct handlebars_string * ret = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s%s%s",
            hbs_str_val(ret),
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0))),
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(1))),
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(2)))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2084318034)
{
    // This is a dumb test
    // "function (_undefined, _null, options) {\n            return (_undefined === undefined) + ' ' + (_null === null) + ' ' + (typeof options);\n          }"
    struct handlebars_value * arg1 = HANDLEBARS_ARG_AT(0);
    char * res = handlebars_talloc_asprintf(vm, "%s %s %s",
                                            (handlebars_value_get_type(arg1) == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            (handlebars_value_get_type(arg1) == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            "object");
    handlebars_value_cstrl(rv, res, strlen(res));
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(2089689191)
{
    // "function link(options) {\n      return '<a href=\"\/people\/' + this.id + '\">' + options.fn(this) + '<\/a>';\n    }"
    struct handlebars_value * id = map_str_find(options->scope, HBS_STRL("id"));
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "<a href=\"/people/%s\">%s</a>",
            hbs_str_val(VALUE_TO_STRING(id)),
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2096893161)
{
    // "function () {\n            return 'null!';\n          }"
    FIXTURE_STRING("null!");
}

FIXTURE_FN(2107645267)
{
    // "function (prefix) {\n      return '<a href=\"' + prefix + '\/' + this.url + '\">' + this.text + '<\/a>';\n    }"
    struct handlebars_value * prefix = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * url = map_str_find(options->scope, HBS_STRL("url"));
    struct handlebars_value * text = map_str_find(options->scope, HBS_STRL("text"));
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "<a href=\"%s/%s\">%s</a>",
            hbs_str_val(VALUE_TO_STRING(prefix)),
            hbs_str_val(VALUE_TO_STRING(url)),
            hbs_str_val(VALUE_TO_STRING(text))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2182811123)
{
    // "function (val) {\n        return val + val;\n      }"
    struct handlebars_value * value = HANDLEBARS_ARG_AT(0);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s",
            hbs_str_val(VALUE_TO_STRING(value)),
            hbs_str_val(VALUE_TO_STRING(value))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2259424295)
{
    handlebars_value_cstrl(rv, HBS_STRL("&'\\<>"));
    handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return rv;
}

FIXTURE_FN(2262633698)
{
    // "function (a, b) {\n        return a + '-' + b;\n      }"
    struct handlebars_value * a = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * b = HANDLEBARS_ARG_AT(1);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s-%s",
            hbs_str_val(VALUE_TO_STRING(a)),
            hbs_str_val(VALUE_TO_STRING(b))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2305563493)
{
    // "function () { return [{text: 'goodbye'}, {text: 'Goodbye'}, {text: 'GOODBYE'}]; }"
    handlebars_value_init_json_string(CONTEXT, rv, "[{\"text\": \"goodbye\"}, {\"text\": \"Goodbye\"}, {\"text\": \"GOODBYE\"}]");
    handlebars_value_convert(rv);
    return rv;
}

FIXTURE_FN(2327777290)
{
    // "function (block) { return block.inverse(''); }"
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_cstrl(context, HBS_STRL(""));
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->inverse, context);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(2439252451)
{
    handlebars_value_value(rv, options->scope);
    return rv;
}

FIXTURE_FN(2499873302)
{
    handlebars_value_boolean(rv, 0);
    return rv;
}

FIXTURE_FN(2515293198)
{
    // "function (param, times, bool1, bool2) {\n        if (typeof times !== 'number') { times = 'NaN'; }\n        if (typeof bool1 !== 'boolean') { bool1 = 'NaB'; }\n        if (typeof bool2 !== 'boolean') { bool2 = 'NaB'; }\n        return 'Hello ' + param + ' ' + times + ' times: ' + bool1 + ' ' + bool2;\n      }"
    struct handlebars_value * param = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * times = HANDLEBARS_ARG_AT(1);
    struct handlebars_value * bool1 = HANDLEBARS_ARG_AT(2);
    struct handlebars_value * bool2 = HANDLEBARS_ARG_AT(3);
    // @todo check types
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "Hello %s %s times: %s %s",
            hbs_str_val(VALUE_TO_STRING(param)),
            hbs_str_val(VALUE_TO_STRING(times)),
            hbs_str_val(VALUE_TO_STRING(bool1)),
            hbs_str_val(VALUE_TO_STRING(bool2))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2554595758)
{
    // "function () { return 'bar'; }"
    FIXTURE_STRING("bar");
}

FIXTURE_FN(2560952765)
{
    // "function () { return 'foo'; }"
    FIXTURE_STRING("foo");
}

FIXTURE_FN(2573932141)
{
    // "function (world) {\n          return 'cruel ' + world.toUpperCase();\n        }"
    struct handlebars_value * value = HANDLEBARS_ARG_AT(0);
    char * tmp = handlebars_talloc_strdup(vm, hbs_str_val(VALUE_TO_STRING(value)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    char * tmp2 = handlebars_talloc_asprintf(
            vm,
            "cruel %s",
            tmp
    );
    handlebars_talloc_free(tmp);
    handlebars_value_cstrl(rv, tmp2, strlen(tmp2));
    handlebars_talloc_free(tmp2);
    return rv;
}

FIXTURE_FN(2596410860)
{
    // "function (context, options) { return options.fn(context); }"
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, context);
    handlebars_value_str(rv, res);
    return rv;
}

FIXTURE_FN(2600345162)
{
    // "function (defaultString) {\n        return new Handlebars.SafeString(defaultString);\n      }"
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    handlebars_value_str(rv, VALUE_TO_STRING(context));
    handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return rv;
}

FIXTURE_FN(2608073270)
{
    handlebars_value_boolean(rv, false);
    return rv;
}

FIXTURE_FN(2632597106)
{
    // "function (options) {\n      var out = '';\n      var byes = ['Goodbye', 'goodbye', 'GOODBYE'];\n      for (var i = 0, j = byes.length; i < j; i++) {\n        out += byes[i] + ' ' + options.fn(this) + '! ';\n      }\n      return out;\n    }",
    struct handlebars_string * tmp1 = handlebars_vm_execute_program(vm, options->program, options->scope);
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(vm, options->program, options->scope);
    struct handlebars_string * tmp3 = handlebars_vm_execute_program(vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s %s! %s %s! %s %s! ",
            "Goodbye",
            hbs_str_val(tmp1),
            "goodbye",
            hbs_str_val(tmp2),
            "GOODBYE",
            hbs_str_val(tmp3)
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2659134105)
{
    // "function (options) {\n          return 'GOODBYE ' + options.hash.cruel + ' ' + options.hash.world + ' ' + options.hash.times + ' TIMES';\n        }"
    struct handlebars_value * cruel = map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * world = map_str_find(options->hash, HBS_STRL("world"));
    struct handlebars_value * times = map_str_find(options->hash, HBS_STRL("times"));
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "GOODBYE %s %s %s TIMES",
            hbs_str_val(VALUE_TO_STRING(cruel)),
            hbs_str_val(VALUE_TO_STRING(world)),
            hbs_str_val(VALUE_TO_STRING(times))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2736662431)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn({}, {blockParams: [1, 2]});\n        }"
    // @todo equals
    HANDLEBARS_VALUE_DECL(block_params);
    HANDLEBARS_VALUE_DECL(context);

    handlebars_value_init_json_string(CONTEXT, block_params, "[1, 2]");
    handlebars_value_convert(block_params);
    handlebars_value_map(context, handlebars_map_ctor(CONTEXT, 0)); // zero may trigger extra rehashes - good for testing
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(vm, options->program, context, NULL, block_params);
    handlebars_value_str(rv, tmp);

    HANDLEBARS_VALUE_UNDECL(context);
    HANDLEBARS_VALUE_UNDECL(block_params);

    return rv;
}

FIXTURE_FN(2795443460)
{
    // "function (options) { return options.fn({text: 'GOODBYE'}); }"
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_init_json_string(CONTEXT, context, "{\"text\": \"GOODBYE\"}");
    handlebars_value_convert(context);
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, context);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(2818908139)
{
    // "function (options) {\n        return options.fn({exclaim: '?'}, { data: {adjective: 'sad'} });\n      }"
    HANDLEBARS_VALUE_DECL(context);
    HANDLEBARS_VALUE_DECL(data);
    handlebars_value_init_json_string(CONTEXT, context, "{\"exclaim\": \"?\"}");
    handlebars_value_init_json_string(CONTEXT, data, "{\"adjective\": \"sad\"}");
    handlebars_value_convert(context); // @TODO we shouldn't have to do this
    handlebars_value_convert(data); // @TODO we shouldn't have to do this
    struct handlebars_string * res = handlebars_vm_execute_program_ex(vm, options->program, context, data, NULL);
    handlebars_value_str(rv, res);
    HANDLEBARS_VALUE_UNDECL(data);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(2842041837)
{
    // "function () {\n        return 'helper missing: ' + arguments[arguments.length - 1].name;\n      }"
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "helper missing: %s",
            hbs_str_val(options->name)
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2857704189)
{
    // "function (options) {\n        return new Handlebars.SafeString(options.fn());\n      }"
    struct handlebars_string * tmp = handlebars_vm_execute_program(vm, options->program, options->scope);
    handlebars_value_str(rv, tmp);
    handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return rv;
}

FIXTURE_FN(2919388099)
{
    // "function (options) {\n        var frame = Handlebars.createFrame(options.data);\n        for (var prop in options.hash) {\n          if (prop in options.hash) {\n            frame[prop] = options.hash[prop];\n          }\n        }\n        return options.fn(this, {data: frame});\n      }"
    struct handlebars_map * map = handlebars_map_ctor(CONTEXT, handlebars_value_count(options->data) + handlebars_value_count(options->hash) + 1);

    HANDLEBARS_VALUE_FOREACH_KV(options->data, key, child) {
        map = handlebars_map_update(map, key, child);
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_FOREACH_KV(options->hash, key, child) {
        map = handlebars_map_update(map, key, child);
    } HANDLEBARS_VALUE_FOREACH_END();

    map = handlebars_map_str_update(map, HBS_STRL("_parent"), options->data);

    HANDLEBARS_VALUE_DECL(frame);
    handlebars_value_map(frame, map);

    struct handlebars_string * res = handlebars_vm_execute_program_ex(vm, options->program, options->scope, frame, NULL);
    handlebars_value_str(rv, res);

    HANDLEBARS_VALUE_UNDECL(frame);

    return rv;
}

FIXTURE_FN(2927692429)
{
    // "function () { return 'hello'; }"
    FIXTURE_STRING("hello");
}

FIXTURE_FN(2940930721)
{
    // "function () { return 'world'; }"
    FIXTURE_STRING("world");
}

FIXTURE_FN(2961119846)
{
    // "function (options) {\n        return options.data.adjective + ' ' + this.noun;\n      }"
    struct handlebars_value * adjective = map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * noun = map_str_find(options->scope, HBS_STRL("noun"));
    assert(adjective != NULL);
    assert(noun != NULL);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s %s",
            hbs_str_val(VALUE_TO_STRING(adjective)),
            hbs_str_val(VALUE_TO_STRING(noun))
    );
    handlebars_value_cstr_steal(rv, tmp);
    return rv;
}

FIXTURE_FN(3011980185)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn({value: 'bar'}, {blockParams: [1, 2]});\n        }"
    // @todo equals
    HANDLEBARS_VALUE_DECL(block_params);
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_init_json_string(CONTEXT, block_params, "[1, 2]");
    handlebars_value_convert(block_params);
    handlebars_value_init_json_string(CONTEXT, context, "{\"value\": \"bar\"}");
    handlebars_value_convert(context);
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(vm, options->program, context, NULL, block_params);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(context);
    HANDLEBARS_VALUE_UNDECL(block_params);
    return rv;
}

FIXTURE_FN(3058305845)
{
    // "function () {return this.foo; }"
    struct handlebars_value * value = map_str_find(options->scope, HBS_STRL("foo"));
    if (value) {
        rv = value; // @TODO possibly wrong
    }
    return rv;
}

FIXTURE_FN(3065257350)
{
    // "function (options) {\n          return this.goodbye.toUpperCase() + options.fn(this);\n        }"
    struct handlebars_value * goodbye = map_str_find(options->scope, HBS_STRL("goodbye"));
    char * tmp = handlebars_talloc_strdup(vm, hbs_str_val(VALUE_TO_STRING(goodbye)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(vm, options->program, options->scope);
    tmp = handlebars_talloc_strdup_append(tmp, hbs_str_val(tmp2));
    handlebars_value_cstr_steal(rv, tmp);
    handlebars_talloc_free(tmp2);
    return rv;
}

FIXTURE_FN(3113355970)
{
    FIXTURE_STRING("world!");
}

FIXTURE_FN(3123251961)
{
    // "function () { return 'helper'; }"
    FIXTURE_STRING("helper");
}

FIXTURE_FN(3167423302)
{
    // "function () { return this.bar; }"
    return map_str_find(options->scope, HBS_STRL("bar"));
}

FIXTURE_FN(3168412868)
{
    // "function (options) {\n        return options.fn({exclaim: '?', zomg: 'world'}, { data: {adjective: 'sad'} });\n      }"
    HANDLEBARS_VALUE_DECL(context);
    HANDLEBARS_VALUE_DECL(data);
    handlebars_value_init_json_string(CONTEXT, context, "{\"exclaim\":\"?\", \"zomg\":\"world\"}");
    handlebars_value_init_json_string(CONTEXT, data, "{\"adjective\": \"sad\"}");
    handlebars_value_convert(context);
    handlebars_value_convert(data);
    struct handlebars_string * res = handlebars_vm_execute_program_ex(vm, options->program, context, data, NULL);
    handlebars_value_str(rv, res);
    HANDLEBARS_VALUE_UNDECL(data);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(3206093801)
{
    // "function (context, options) { return '<form>' + options.fn(context) + '<\/form>'; }"
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    struct handlebars_string * res = handlebars_vm_execute_program(vm, options->program, context);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "<form>%s</form>",
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    handlebars_value_cstr_steal(rv, tmp);
    return rv;
}

FIXTURE_FN(325991858)
{
    // "function (options) {\n      var out = '';\n      var byes = ['Goodbye', 'goodbye', 'GOODBYE'];\n      for (var i = 0, j = byes.length; i < j; i++) {\n        out += byes[i] + ' ' + options.fn({}) + '! ';\n      }\n      return out;\n    }"
    HANDLEBARS_VALUE_DECL(context);
    handlebars_value_map(context, handlebars_map_ctor(CONTEXT, 0)); // zero may trigger extra rehashes - good for testing
    struct handlebars_string * tmp1 = handlebars_vm_execute_program(vm, options->program, context);
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(vm, options->program, context);
    struct handlebars_string * tmp3 = handlebars_vm_execute_program(vm, options->program, context);
    HANDLEBARS_VALUE_UNDECL(context);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s %s! %s %s! %s %s! ",
            "Goodbye",
            hbs_str_val(tmp1),
            "goodbye",
            hbs_str_val(tmp2),
            "GOODBYE",
            hbs_str_val(tmp3)
    );
    handlebars_value_cstr_steal(rv, tmp);
    handlebars_talloc_free(tmp1);
    handlebars_talloc_free(tmp2);
    handlebars_talloc_free(tmp3);
    return rv;
}

FIXTURE_FN(3307473738)
{
    FIXTURE_STRING("Awesome");
}

FIXTURE_FN(3287026061)
{
    // "function () {\n      return '';\n    }"
    FIXTURE_STRING("");
}

FIXTURE_FN(3308130198)
{
    // "function (value) {\n      return 'foo ' + value;\n    }"
    struct handlebars_value * value = HANDLEBARS_ARG_AT(0);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "foo %s",
            hbs_str_val(VALUE_TO_STRING(value))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(3325763044)
{
    // "function (val, that, theOther) {\n        return 'val is ' + val + ', ' + that + ' and ' + theOther;\n      }"
    struct handlebars_value * val = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * that = HANDLEBARS_ARG_AT(1);
    struct handlebars_value * theOther = HANDLEBARS_ARG_AT(2);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "val is %s, %s and %s",
            hbs_str_val(VALUE_TO_STRING(val)),
            hbs_str_val(VALUE_TO_STRING(that)),
            hbs_str_val(VALUE_TO_STRING(theOther))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(3327136760)
{
    // "function () {\n          return this.goodbye.toUpperCase();\n        }"
    struct handlebars_value * goodbye = map_str_find(options->scope, HBS_STRL("goodbye"));
    char * tmp = handlebars_talloc_strdup(vm, hbs_str_val(VALUE_TO_STRING(goodbye)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(3328314220)
{
    // "function () { return 'helpers'; }"
    FIXTURE_STRING("helpers");
}

FIXTURE_FN(3379432388)
{
    // "function () { return this.more; }"
    struct handlebars_value * value = map_str_find(options->scope, HBS_STRL("more"));
    if( !value ) {
        value = rv;
    }
    return value;
}

FIXTURE_FN(3407223629)
{
    // "function () {\n        return 'missing: ' + arguments[arguments.length - 1].name;\n      }",
    char * tmp = handlebars_talloc_asprintf(
        vm,
        "missing: %s",
        hbs_str_val(options->name)
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(3477736473)
{
    // "function () {return this.world; }"
    return map_str_find(options->scope, HBS_STRL("world"));
}

FIXTURE_FN(3578728160)
{
    // "function () {\n            return 'undefined!';\n          }"
    handlebars_value_cstrl(rv, HBS_STRL("undefined!"));
    return rv;
}

FIXTURE_FN(3659403207)
{
    // "function (value) {\n        return 'bar ' + value;\n    }"
    struct handlebars_value * arg = HANDLEBARS_ARG_AT(0);
    handlebars_value_cstr_steal(rv, handlebars_talloc_asprintf(
            CONTEXT, "%s%s", "bar ",  hbs_str_val(VALUE_TO_STRING(arg))
    ));
    return rv;
}

FIXTURE_FN(3691188061)
{
    // "function (val) {\n        return 'val is ' + val;\n      }"
    struct handlebars_value * value = HANDLEBARS_ARG_AT(0);
    char * ret = handlebars_talloc_asprintf(
            vm,
            "val is %s",
            hbs_str_val(VALUE_TO_STRING(value))
    );
    handlebars_value_cstrl(rv, ret, strlen(ret));
    handlebars_talloc_free(ret);
    return rv;
}

FIXTURE_FN(3697740723)
{
    FIXTURE_STRING("found it!");
}

FIXTURE_FN(3707047013)
{
    // "function (value) { return value; }"
    handlebars_value_value(rv, HANDLEBARS_ARG_AT(0));
    return rv;
}

FIXTURE_FN(3728875550)
{
    // "function (options) {\n        return options.data.accessData + ' ' + options.fn({exclaim: '?'});\n      }"
    HANDLEBARS_VALUE_DECL(context);
    struct handlebars_value * access_data = map_str_find(options->data, HBS_STRL("accessData"));
    handlebars_value_init_json_string(CONTEXT, context, "{\"exclaim\": \"?\"}");
    handlebars_value_convert(context); // @TODO we shouldn't have to do this
    struct handlebars_string * ret = handlebars_vm_execute_program(vm, options->program, context);
    char * ret2 = handlebars_talloc_asprintf(
            vm,
            "%s %s",
            hbs_str_val(VALUE_TO_STRING(access_data)),
            hbs_str_val(ret)
    );
    handlebars_value_cstr_steal(rv, ret2);
    HANDLEBARS_VALUE_UNDECL(context);
    return rv;
}

FIXTURE_FN(3781305181)
{
    // "function (times) {\n      if (typeof times !== 'number') { times = 'NaN'; }\n      return 'Hello ' + times + ' times';\n    }"
    struct handlebars_value * times = HANDLEBARS_ARG_AT(0);
    // @todo this should be a float perhaps?
    /* if( times->type != HANDLEBARS_VALUE_TYPE_FLOAT || times->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_cstrl(times, HBS_STRL("NaN"));
    }*/
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "Hello %s times",
            hbs_str_val(VALUE_TO_STRING(times))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(3878511480)
{
    // "function list(context, options) {\n      if (context.length > 0) {\n        var out = '<ul>';\n        for (var i = 0, j = context.length; i < j; i++) {\n          out += '<li>';\n          out += options.fn(context[i]);\n          out += '<\/li>';\n        }\n        out += '<\/ul>';\n        return out;\n      } else {\n        return '<p>' + options.inverse(this) + '<\/p>';\n      }\n    }"
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    char *tmp;
    if( !handlebars_value_is_empty(context) ) {
        tmp = handlebars_talloc_strdup(vm, "<ul>");
        HANDLEBARS_VALUE_FOREACH(context, child) {
            struct handlebars_string * tmp2 = handlebars_vm_execute_program(vm, options->program, child);
            tmp = handlebars_talloc_asprintf_append(
                    tmp,
                    "<li>%s</li>",
                    hbs_str_val(tmp2)
            );
            handlebars_talloc_free(tmp2);
        } HANDLEBARS_VALUE_FOREACH_END();
        tmp = handlebars_talloc_strdup_append(tmp, "</ul>");
    } else {
        struct handlebars_string * tmp2 = handlebars_vm_execute_program(vm, options->inverse, options->scope);
        tmp = handlebars_talloc_asprintf(
            vm,
            "<p>%s</p>",
            hbs_str_val(tmp2)
        );
        handlebars_talloc_free(tmp2);
    }
    handlebars_value_cstr_steal(rv, tmp);
    return rv;
}

FIXTURE_FN(3963629287)
{
    FIXTURE_STRING("success");
}

FIXTURE_FN(4005129518)
{
    // "function (options) {\n        var hash = options.hash;\n        var ariaLabel = Handlebars.Utils.escapeExpression(hash['aria-label']);\n        var placeholder = Handlebars.Utils.escapeExpression(hash.placeholder);\n        return new Handlebars.SafeString('<input aria-label=\"' + ariaLabel + '\" placeholder=\"' + placeholder + '\" \/>');\n      }"
    struct handlebars_value * label = map_str_find(options->hash, HBS_STRL("aria-label"));
    struct handlebars_value * placeholder = map_str_find(options->hash, HBS_STRL("placeholder"));
    struct handlebars_string * tmp1 = handlebars_value_expression(CONTEXT, label, 1);
    struct handlebars_string * tmp2 = handlebars_value_expression(CONTEXT, placeholder, 1);
    char * tmp3 = handlebars_talloc_asprintf(
            vm,
            "<input aria-label=\"%s\" placeholder=\"%s\" />",
            hbs_str_val(tmp1),
            hbs_str_val(tmp2)
    );
    handlebars_value_cstrl(rv, tmp3, strlen(tmp3));
    handlebars_value_set_flag(rv, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    handlebars_talloc_free(tmp1);
    handlebars_talloc_free(tmp2);
    handlebars_talloc_free(tmp3);
    return rv;
}

FIXTURE_FN(4112130635)
{
    // "function (thing, options) {\n        return options.data.adjective + ' ' + thing + (this.exclaim || '');\n      }"
    struct handlebars_value * adjective = map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * thing = HANDLEBARS_ARG_AT(0);
    struct handlebars_value * exclaim = map_str_find(options->scope, HBS_STRL("exclaim"));
    char * res = handlebars_talloc_asprintf(
            vm,
            "%s %s%s",
            hbs_str_val(VALUE_TO_STRING(adjective)),
            hbs_str_val(VALUE_TO_STRING(thing)),
            hbs_str_val(VALUE_TO_STRING(exclaim))
    );
    handlebars_value_cstrl(rv, res, strlen(res));
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(4158918668)
{
    struct handlebars_value * noun = HANDLEBARS_ARG_AT(0);
    char * tmp = hbs_str_val(VALUE_TO_STRING(noun));
    char * res = handlebars_talloc_asprintf(
            vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    handlebars_value_cstrl(rv, res, strlen(res));
    handlebars_talloc_free(res);
    return rv;
}

FIXTURE_FN(4204859626)
{
    struct handlebars_string * res = handlebars_vm_execute_program_ex(vm, options->program, options->scope, NULL, NULL);
    handlebars_value_str(rv, res);
    return rv;
}

FIXTURE_FN(4207421535)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn(this, {blockParams: [1, 2]});\n        }"
    // @todo equals
    HANDLEBARS_VALUE_DECL(block_params);
    handlebars_value_init_json_string(CONTEXT, block_params, "[1, 2]");
    handlebars_value_convert(block_params);
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(vm, options->program, options->scope, NULL, block_params);
    handlebars_value_str(rv, tmp);
    HANDLEBARS_VALUE_UNDECL(block_params);
    return rv;
}

FIXTURE_FN(1414406764)
{
    // function testHelper(options) {\n          return options.lookupProperty(this, 'testProperty');\n        }
    return map_str_find(options->scope, HBS_STRL("testProperty"));
}

FIXTURE_FN(1830193534)
{
    // function() {\n        \/\/ It's valid to execute a block against an undefined context, but\n        \/\/ helpers can not do so, so we expect to have an empty object here;\n        for (var name in this) {\n          if (Object.prototype.hasOwnProperty.call(this, name)) {\n            return 'found';\n          }\n        }\n        \/\/ And to make IE happy, check for the known string as length is not enumerated.\n        return this === 'bat' ? 'found' : 'not';\n      }
    if (!handlebars_value_is_empty(options->scope)) {
        FIXTURE_STRING("found");
    } else {
        FIXTURE_STRING("not");
    }
}

FIXTURE_FN(1569150712)
{
    // function goodbye(options) {\n        if (options.hash.print === true) {\n          return 'GOODBYE ' + options.hash.cruel + ' ' + options.hash.world;\n        } else if (options.hash.print === false) {\n          return 'NOT PRINTING';\n        } else {\n          return 'THIS SHOULD NOT HAPPEN';\n        }\n      }
    struct handlebars_value * print = map_str_find(options->hash, HBS_STRL("print"));
    struct handlebars_value * cruel = map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * world = map_str_find(options->hash, HBS_STRL("world"));

    if (print && handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE) {
        char * tmp = handlebars_talloc_asprintf(
                vm,
                "GOODBYE %s %s",
                hbs_str_val(VALUE_TO_STRING(cruel)),
                hbs_str_val(VALUE_TO_STRING(world))
        );
        handlebars_value_cstrl(rv, tmp, strlen(tmp));
        handlebars_talloc_free(tmp);
        return rv;
    } else if (print && handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_FALSE) {
        FIXTURE_STRING("NOT PRINTING");
    } else {
        FIXTURE_STRING("THIS SHOULD NOT HAPPEN");
    }
}

FIXTURE_FN(1646670161)
{
    // function(conditional, options) {\n        if (conditional) {\n          return options.fn(this);\n        } else {\n          return options.inverse(this);\n        }\n      }
    assert(argc > 0);
    struct handlebars_value * conditional = HANDLEBARS_ARG_AT(0);
    struct handlebars_string * str;
    if (!handlebars_value_is_empty(conditional)) {
        // assert(options->program > 0);
        str = handlebars_vm_execute_program(vm, options->program, options->scope);
    } else {
        // assert(options->inverse > 0);
        str = handlebars_vm_execute_program(vm, options->inverse, options->scope);
    }
    handlebars_value_str(rv, str);
    return rv;
}

FIXTURE_FN(2855343161)
{
    // function(options) {\n        return options.hash.length;\n      }
    return map_str_find(options->hash, HBS_STRL("length"));
}

// static struct handlebars_value * last_options = NULL;

FIXTURE_FN(3568189707)
{
    // function(x, y, options) {\n        if (!options || options === global.lastOptions) {\n          throw new Error('options hash was reused');\n        }\n        global.lastOptions = options;\n        return x === y;\n      }

    // Yeah so each stack allocation of this produces the same pointer to options, so we can't test for pointer equality here
    // @TODO add a field to options to uniquely identify it? (breaks ABI)

    // if (!options || options == last_options) {
    //     last_options = NULL;
    //     FIXTURE_STRING("options hash was reused");
    // }
    // last_options = options;

    assert(argc >= 2);

    if (handlebars_value_get_type(HANDLEBARS_ARG_AT(0)) == HANDLEBARS_VALUE_TYPE_TRUE && handlebars_value_get_type(HANDLEBARS_ARG_AT(1)) == HANDLEBARS_VALUE_TYPE_TRUE) {
        handlebars_value_boolean(rv, true);
    } else {
        handlebars_value_boolean(rv, false);
    }
    return rv;
}

FIXTURE_FN(1561073198)
{
    // function (options) {\n        return \"val is \" + options.hash.fun;\n      }
    struct handlebars_value * fun = map_str_find(options->hash, HBS_STRL("fun"));
    assert(fun != NULL);
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "val is %s",
            hbs_str_val(VALUE_TO_STRING(fun))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

// mustache lambda fixtures

FIXTURE_FN(779421032)
{
    // return "world";
    FIXTURE_STRING("world");
}

FIXTURE_FN(1396901812)
{
    // return "{{planet}}";
    FIXTURE_STRING("{{planet}}");
}

FIXTURE_FN(3353742315)
{
    // return "|planet| => {{planet}}";
    FIXTURE_STRING("|planet| => {{planet}}");
}

static int calls = 0;

FIXTURE_FN(2925333156)
{
    // global $calls; return ++$calls;
    calls++;
    FIXTURE_INTEGER(calls);
}

FIXTURE_FN(414319486)
{
    // return ">";
    FIXTURE_STRING(">");
}

FIXTURE_FN(401804363)
{
    // return ($text == "{{x}}") ? "yes" : "no";
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    struct handlebars_value * context = HANDLEBARS_ARG_AT(0);
    struct handlebars_string * str = VALUE_TO_STRING(context);
    if (0 == strncmp(hbs_str_val(str), "{{x}}", sizeof("{{x}}") - 1)) {
        FIXTURE_STRING("yes");
    } else {
        FIXTURE_STRING("no");
    }
}

FIXTURE_FN(3964931170)
{
    // return $text . "{{planet}}" . $text;
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s%s",
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0))),
            "{{planet}}",
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0)))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2718175385)
{
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    // return $text . "{{planet}} => |planet|" . $text;
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s%s",
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0))),
            "{{planet}} => |planet|",
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0)))
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(2000357317)
{
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    // return "__" . $text . "__";
    char * tmp = handlebars_talloc_asprintf(
            vm,
            "%s%s%s",
            "__",
            hbs_str_val(VALUE_TO_STRING(HANDLEBARS_ARG_AT(0))),
            "__"
    );
    handlebars_value_cstrl(rv, tmp, strlen(tmp));
    handlebars_talloc_free(tmp);
    return rv;
}

FIXTURE_FN(617219335)
{
    // return false;
    handlebars_value_boolean(rv, 0);
    return rv;
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
    handlebars_value_helper(value, func); \

    assert(handlebars_value_get_real_type(value) == HANDLEBARS_VALUE_TYPE_MAP);

    struct handlebars_value * jsvalue = map_str_find(value, HBS_STRL("javascript"));
    if( !jsvalue ) {
        jsvalue = map_str_find(value, HBS_STRL("php"));
    }
    assert(jsvalue != NULL);
    assert(handlebars_value_get_real_type(jsvalue) == HANDLEBARS_VALUE_TYPE_STRING);
    uint32_t hash = adler32((unsigned char *) handlebars_value_get_strval(jsvalue), handlebars_value_get_strlen(jsvalue));

    switch( hash ) {
        FIXTURE_CASE(20974934);
        FIXTURE_CASE(49286285);
        FIXTURE_CASE(126946175);
        FIXTURE_CASE(169151220);
        FIXTURE_CASE(276040703);
        FIXTURE_CASE(454102302);
        FIXTURE_CASE(459219799);
        FIXTURE_CASE(461441956);
        FIXTURE_CASE(464915369);
        FIXTURE_CASE(471141295);
        FIXTURE_CASE(510017722);
        FIXTURE_CASE(585442881);
        FIXTURE_CASE(620640779);
        FIXTURE_CASE(620828131);
        FIXTURE_CASE(665715952);
        FIXTURE_CASE(666457330);
        FIXTURE_CASE(662835958);
        FIXTURE_CASE(690821881);
        FIXTURE_CASE(730081660);
        FIXTURE_CASE(730672213);
        FIXTURE_CASE(739773491);
        FIXTURE_CASE(748362646);
        FIXTURE_CASE(788468697);
        FIXTURE_CASE(902433745);
        FIXTURE_CASE(922226146);
        FIXTURE_CASE(929767352);
        FIXTURE_CASE(931412676);
        FIXTURE_CASE(958795451);
        FIXTURE_CASE(1041501180);
        FIXTURE_CASE(1091971719);
        FIXTURE_CASE(1102272015);
        FIXTURE_CASE(1198465479);
        FIXTURE_CASE(1211570580);
        FIXTURE_CASE(1250888967);
        FIXTURE_CASE(1252527367);
        FIXTURE_CASE(1283397100);
        FIXTURE_CASE(1341397520);
        FIXTURE_CASE(1414406764);
        FIXTURE_CASE(1561073198);
        FIXTURE_CASE(1569150712);
        FIXTURE_CASE(1582700088);
        FIXTURE_CASE(1623791204);
        FIXTURE_CASE(1644694756);
        FIXTURE_CASE(1646670161);
        FIXTURE_CASE(1774917451);
        FIXTURE_CASE(1818365722);
        FIXTURE_CASE(1830193534);
        FIXTURE_CASE(1872958178);
        FIXTURE_CASE(1983911259);
        FIXTURE_CASE(2084318034);
        FIXTURE_CASE(2089689191);
        FIXTURE_CASE(2096893161);
        FIXTURE_CASE(2107645267);
        FIXTURE_CASE(2182811123);
        FIXTURE_CASE(2259424295);
        FIXTURE_CASE(2262633698);
        FIXTURE_CASE(2305563493);
        FIXTURE_CASE(2327777290);
        FIXTURE_CASE(2439252451);
        FIXTURE_CASE(2499873302);
        FIXTURE_CASE(2515293198);
        FIXTURE_CASE(2554595758);
        FIXTURE_CASE(2560952765);
        FIXTURE_CASE(2573932141);
        FIXTURE_CASE(2596410860);
        FIXTURE_CASE(2600345162);
        FIXTURE_CASE(2608073270);
        FIXTURE_CASE(2632597106);
        FIXTURE_CASE(2659134105);
        FIXTURE_CASE(2736662431);
        FIXTURE_CASE(2795443460);
        FIXTURE_CASE(2818908139);
        FIXTURE_CASE(2842041837);
        FIXTURE_CASE(2855343161);
        FIXTURE_CASE(2857704189);
        FIXTURE_CASE(2919388099);
        FIXTURE_CASE(2927692429);
        FIXTURE_CASE(2940930721);
        FIXTURE_CASE(2961119846);
        FIXTURE_CASE(3011980185);
        FIXTURE_CASE(3058305845);
        FIXTURE_CASE(3065257350);
        FIXTURE_CASE(3113355970);
        FIXTURE_CASE(3123251961);
        FIXTURE_CASE(3167423302);
        FIXTURE_CASE(3168412868);
        FIXTURE_CASE(3206093801);
        FIXTURE_CASE(325991858);
        FIXTURE_CASE(3287026061);
        FIXTURE_CASE(3307473738);
        FIXTURE_CASE(3308130198);
        FIXTURE_CASE(3325763044);
        FIXTURE_CASE(3327136760);
        FIXTURE_CASE(3328314220);
        FIXTURE_CASE(3379432388);
        FIXTURE_CASE(3407223629);
        FIXTURE_CASE(3477736473);
        FIXTURE_CASE(3568189707);
        FIXTURE_CASE(3578728160);
        FIXTURE_CASE(3659403207);
        FIXTURE_CASE(3691188061);
        FIXTURE_CASE(3697740723);
        FIXTURE_CASE(3707047013);
        FIXTURE_CASE(3728875550);
        FIXTURE_CASE(3781305181);
        FIXTURE_CASE(3878511480);
        FIXTURE_CASE(3963629287);
        FIXTURE_CASE(4005129518);
        FIXTURE_CASE(4112130635);
        FIXTURE_CASE(4158918668);
        FIXTURE_CASE(4204859626);
        FIXTURE_CASE(4207421535);

        FIXTURE_CASE_ALIAS(304353340, 1644694756);
        FIXTURE_CASE_ALIAS(401083957, 3707047013);
        FIXTURE_CASE_ALIAS(882643306, 4204859626);
        FIXTURE_CASE_ALIAS(1111103580, 1341397520);
        FIXTURE_CASE_ALIAS(2836204191, 739773491);
        FIXTURE_CASE_ALIAS(3153085867, 2919388099);
        FIXTURE_CASE_ALIAS(3584035731, 3697740723);
        FIXTURE_CASE_ALIAS(2852917582, 1872958178);
        FIXTURE_CASE_ALIAS(2337343947, 126946175);
        FIXTURE_CASE_ALIAS(2443446763, 126946175);

        FIXTURE_CASE_ALIAS(2734230037, 464915369);
        FIXTURE_CASE_ALIAS(1276597723, 666457330);

        // mustache lambda fixtures
        FIXTURE_CASE(779421032);
        FIXTURE_CASE(1396901812);
        FIXTURE_CASE(3353742315);
        FIXTURE_CASE(2925333156);
        FIXTURE_CASE(414319486);
        FIXTURE_CASE(401804363);
        FIXTURE_CASE(3964931170);
        FIXTURE_CASE(2718175385);
        FIXTURE_CASE(2000357317);
        FIXTURE_CASE(617219335);

        default:
            fprintf(stderr, "Unimplemented test fixture [%u]:\n%s\n", hash, handlebars_value_get_strval(jsvalue));
            return;
    }

#ifndef NDEBUG
    fprintf(stderr, "FIXTURE: [%u]\n", hash);
#endif

#undef SET_FUNCTION
}

void load_fixtures(struct handlebars_value * value)
{
    struct handlebars_value * child;

    // This shouldn't happen ...
    assert(value != NULL);

    handlebars_value_convert(value);

    switch( handlebars_value_get_type(value) ) {
        case HANDLEBARS_VALUE_TYPE_MAP:
            // Check if it contains a "!code" key
            child = map_str_find(value, HBS_STRL("!code"));
            if (!child) {
                // this is for mustache, getting at the tags is a pain, so assume any object with a "php" key is code
                child = map_str_find(value, HBS_STRL("php"));
            }
            if( child ) {
                // Convert to helper
                convert_value_to_fixture(value);
            } else {
                // Recurse
                HANDLEBARS_VALUE_FOREACH_KV(value, key, child) {
                    load_fixtures(child);
                    handlebars_value_map_update(value, key, child);
                } HANDLEBARS_VALUE_FOREACH_END();
            }
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            HANDLEBARS_VALUE_FOREACH_IDX(value, index, child) {
                load_fixtures(child);
                handlebars_value_array_set(value, index, child);
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
        default:
            // do nothing
            break;
    }
}

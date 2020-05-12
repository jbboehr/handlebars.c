 /**
 * Copyright (C) 2020 John Boehr
 *
 * This file is part of handlebars.c.
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

#define HANDLEBARS_HELPERS_PRIVATE

#include "handlebars.h"
#include "handlebars_json.h"
#include "handlebars_map.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"



uint32_t adler32(unsigned char *data, size_t len);

#define CONTEXT ((struct handlebars_context *)options->vm)
#define FIXTURE_FN(hash) static struct handlebars_value * fixture_ ## hash(HANDLEBARS_HELPER_ARGS)
#define FIXTURE_STRING(string) \
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT); \
    handlebars_value_string(value, string); \
    return value;
#define FIXTURE_INTEGER(integer) \
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT); \
    handlebars_value_integer(value, integer); \
    return value;

FIXTURE_FN(20974934)
{
    // "function (arg) { return typeof arg; }"
    struct handlebars_value * arg = argv[0];
    if( handlebars_value_get_type(arg) == HANDLEBARS_VALUE_TYPE_NULL ) {
        FIXTURE_STRING("undefined");
    } else {
        FIXTURE_STRING("not undefined");
    }
}

FIXTURE_FN(49286285)
{
    struct handlebars_value * arg = argv[0];
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, handlebars_talloc_asprintf(
            arg, "%s%s", "bar",  hbs_str_val(handlebars_value_to_string(arg))
    ));
    return result;
}

FIXTURE_FN(126946175)
{
    // "function () {\n        return true;\n      }"
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_boolean(value, 1);
    return value;
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
    struct handlebars_value * prefix = argv[0];
    assert(handlebars_value_get_type(prefix) == HANDLEBARS_VALUE_TYPE_NULL);
    const char * tmp = "undefined";
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    return result;
}

FIXTURE_FN(459219799)
{
    // "function (prefix, options) {\n        return '<a href=\"' + prefix + '\/' + this.url + '\">' + options.fn(this) + '<\/a>';\n    }"
    struct handlebars_value * prefix = argv[0];
    struct handlebars_value * url = handlebars_value_map_str_find(options->scope, HBS_STRL("url"));
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<a href=\"%s/%s\">%s</a>",
            hbs_str_val(handlebars_value_to_string(prefix)),
            hbs_str_val(handlebars_value_to_string(url)),
            hbs_str_val(res)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);

    handlebars_talloc_free(tmp);
    handlebars_talloc_free(res);
    return result;
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
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"value\": \"bar\"}");
    struct handlebars_value * bp1 = handlebars_value_ctor(CONTEXT);
    struct handlebars_value * bp2 = handlebars_value_ctor(CONTEXT);
    handlebars_value_integer(bp1, value_for_510017722++);
    handlebars_value_integer(bp2, value_for_510017722++);

    struct handlebars_value * block_params = handlebars_value_ctor(CONTEXT);
    handlebars_value_array_init(block_params, 2);
    handlebars_value_array_push(block_params, bp1); // @TODO ignoring return value - should probably make a handlebars_value_push()
    handlebars_value_array_push(block_params, bp2); // @TODO ignoring return value - should probably make a handlebars_value_push()

    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(options->vm, options->program, context, NULL, block_params);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);

    return result;
}

FIXTURE_FN(585442881)
{
    // "function (cruel, world, options) {\n        return options.fn({greeting: 'Goodbye', adj: cruel, noun: world});\n      }"
    struct handlebars_value * cruel = argv[0];
    struct handlebars_value * world = argv[1];
    struct handlebars_value * context = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(context, 0); // zero may trigger extra rehashes - good for testing
    struct handlebars_value * greeting = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(greeting, "Goodbye");
    handlebars_map_str_update(handlebars_value_get_map(context), HBS_STRL("greeting"), greeting);
    handlebars_map_str_update(handlebars_value_get_map(context), HBS_STRL("adj"), cruel);
    handlebars_map_str_update(handlebars_value_get_map(context), HBS_STRL("noun"), world);
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);

    return result;
}

FIXTURE_FN(620640779)
{
    // "function (times, times2) {\n      if (typeof times !== 'number') { times = 'NaN'; }\n      if (typeof times2 !== 'number') { times2 = 'NaN'; }\n      return 'Hello ' + times + ' ' + times2 + ' times';\n    }"
    struct handlebars_value * times = argv[0];
    struct handlebars_value * times2 = argv[1];
    // @todo this should be a float perhaps?
    /* if( times->type != HANDLEBARS_VALUE_TYPE_FLOAT || times->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_string(times, "NaN");
    }
    if( times2->type != HANDLEBARS_VALUE_TYPE_FLOAT || times2->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_string(times2, "NaN");
    } */
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s %s times",
            hbs_str_val(handlebars_value_to_string(times)),
            hbs_str_val(handlebars_value_to_string(times2))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(620828131)
{
    // "function (cruel, world) { return 'Goodbye ' + cruel + ' ' + world; }"
    struct handlebars_value * cruel = argv[0];
    struct handlebars_value * world = argv[1];

    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "Goodbye %s %s",
            hbs_str_val(handlebars_value_to_string(cruel)),
            hbs_str_val(handlebars_value_to_string(world))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(665715952)
{
    // "function () {}"
    return handlebars_value_ctor(CONTEXT);
}

FIXTURE_FN(662835958)
{
    // "function () { return {first: 'Alan', last: 'Johnson'}; }",
    struct handlebars_value * value = handlebars_value_from_json_string(CONTEXT, "{\"first\": \"Alan\", \"last\": \"Johnson\"}");
    handlebars_value_convert(value);
    return value;
}

FIXTURE_FN(690821881)
{
    // this one is actually a partial
    // "function partial(context) {\n      return context.name + ' (' + context.url + ') ';\n    }"
    struct handlebars_value * context = argv[0];
    struct handlebars_value * name = handlebars_value_map_str_find(context, HBS_STRL("name"));
    struct handlebars_value * url = handlebars_value_map_str_find(context, HBS_STRL("url"));
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s (%s) ",
            hbs_str_val(handlebars_value_to_string(name)),
            hbs_str_val(handlebars_value_to_string(url))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(666457330)
{
    // "function (options) {\n          if (options.hash.print === true) {\n            return 'GOODBYE ' + options.hash.cruel + ' ' + options.fn(this);\n          } else if (options.hash.print === false) {\n            return 'NOT PRINTING';\n          } else {\n            return 'THIS SHOULD NOT HAPPEN';\n          }\n        }"
    struct handlebars_value * print = handlebars_value_map_str_find(options->hash, HBS_STRL("print"));
    if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE ) {
        struct handlebars_value * cruel = handlebars_value_map_str_find(options->hash, HBS_STRL("cruel"));
        struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
        char * tmp = handlebars_talloc_asprintf(
                options->vm,
                "GOODBYE %s %s",
                hbs_str_val(handlebars_value_to_string(cruel)),
                hbs_str_val(res)
        );
        handlebars_talloc_free(res);
        struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(result, tmp);
        handlebars_talloc_free(tmp);
        return result;
    } else if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_FALSE ) {
        FIXTURE_STRING("NOT PRINTING");
    } else {
        FIXTURE_STRING("THIS SHOULD NOT HAPPEN");
    }
}

FIXTURE_FN(730081660)
{
    // "function (options) {\n          return 'GOODBYE ' + options.hash.cruel + ' ' + options.fn(this) + ' ' + options.hash.times + ' TIMES';\n        }"
    struct handlebars_value * cruel = handlebars_value_map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * times = handlebars_value_map_str_find(options->hash, HBS_STRL("times"));
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "GOODBYE %s %s %s TIMES",
            hbs_str_val(handlebars_value_to_string(cruel)),
            hbs_str_val(res),
            hbs_str_val(handlebars_value_to_string(times))
    );
    handlebars_talloc_free(res);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(730672213)
{
    if( 0 == strcmp(hbs_str_val(options->name), "link_to") ) {
        struct handlebars_value * mesg = argv[0];
        char * tmp = handlebars_talloc_asprintf(
                options->vm,
                "<a>%s</a>",
                hbs_str_val(handlebars_value_to_string(mesg))
        );
        struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(result, tmp);
        handlebars_talloc_free(tmp);
        handlebars_value_set_flag(result, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
        return result;
    } else {
        return NULL;
    }
}

FIXTURE_FN(739773491)
{
    // "function (arg) {\n        return arg;\n      }"
    return argv[0];
}

FIXTURE_FN(748362646)
{
    // "function (options) { return '<a href=\"' + this.name + '\">' + options.fn(this) + '<\/a>'; }"
    struct handlebars_value * name = handlebars_value_map_str_find(options->scope, HBS_STRL("name"));
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<a href=\"%s\">%s</a>",
            hbs_str_val(handlebars_value_to_string(name)),
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
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
    struct handlebars_value * context = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(context, "");
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(929767352)
{
    // "function (options) {\n        return options.data.adjective + ' world' + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * exclaim = handlebars_value_map_str_find(options->scope, HBS_STRL("exclaim"));
    char * ret = handlebars_talloc_asprintf(
            options->vm,
            "%s world%s",
            hbs_str_val(handlebars_value_to_string(adjective)),
            handlebars_value_is_empty(exclaim) ? "" : "!"
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, ret);
    handlebars_talloc_free(ret);
    return result;
}

FIXTURE_FN(931412676)
{
    // "function (options) {\n            var frame = Handlebars.createFrame(options.data);\n            frame.depth = options.data.depth + 1;\n            return options.fn(this, {data: frame});\n          }"
    // struct handlebars_value * frame = handlebars_value_ctor(CONTEXT);
    // handlebars_value_map_init(frame, 0);  // zero may trigger extra rehashes - good for testing
    struct handlebars_map * map = handlebars_map_ctor(CONTEXT, handlebars_value_count(options->data) + 1);

    HANDLEBARS_VALUE_FOREACH_KV(options->data, key, child) {
        if( 0 == strcmp(hbs_str_val(key), "depth") ) {
            struct handlebars_value * tmp = handlebars_value_ctor(CONTEXT);
            handlebars_value_integer(tmp, handlebars_value_get_intval(child) + 1);
            handlebars_map_update(map, key, tmp);
        } else {
            handlebars_map_update(map, key, child);
        }
    } HANDLEBARS_VALUE_FOREACH_END();

    handlebars_map_str_update(map, HBS_STRL("_parent"), options->data);
    struct handlebars_value * frame = handlebars_value_ctor(CONTEXT);
    handlebars_value_map(frame, map);

    struct handlebars_string * res = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, frame, NULL);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, res);
    return result;
}

FIXTURE_FN(958795451)
{
    struct handlebars_value * truthiness = handlebars_value_map_str_find(options->scope, HBS_STRL("truthiness"));
    if( handlebars_value_is_callable(truthiness) ) {
        return handlebars_value_call(truthiness, argc, argv, options);
    }
    return truthiness;
}

FIXTURE_FN(1211570580)
{
    // "function () {\n        return 'ran: ' + arguments[arguments.length - 1].name;\n      }"
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "ran: %s",
            hbs_str_val(options->name)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
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
                options->vm,
                "<a>%s</a>",
                "winning"
        );
        struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(result, tmp);
        handlebars_talloc_free(tmp);
        handlebars_value_set_flag(result, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
        return result;
    } else {
        return NULL;
    }
}

FIXTURE_FN(1041501180)
{
    // "function (a, b) {\n        return a + b;\n      }"
    struct handlebars_value * a = argv[0];
    struct handlebars_value * b = argv[1];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s%s",
            hbs_str_val(handlebars_value_to_string(a)),
            hbs_str_val(handlebars_value_to_string(b))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(1102272015)
{
    struct handlebars_value * print = handlebars_value_map_str_find(options->hash, HBS_STRL("print"));
    if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE ) {
        struct handlebars_value * cruel = handlebars_value_map_str_find(options->hash, HBS_STRL("cruel"));
        struct handlebars_value * world = handlebars_value_map_str_find(options->hash, HBS_STRL("world"));
        char * tmp = handlebars_talloc_asprintf(
                options->vm,
                "GOODBYE %s %s",
                hbs_str_val(handlebars_value_to_string(cruel)),
                hbs_str_val(handlebars_value_to_string(world))
        );
        struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(result, tmp);
        handlebars_talloc_free(tmp);
        return result;
    } else if( handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_FALSE ) {
        FIXTURE_STRING("NOT PRINTING");
    } else {
        FIXTURE_STRING("THIS SHOULD NOT HAPPEN");
    }
}

FIXTURE_FN(1198465479)
{
    // "function (noun, options) {\n        return options.data.adjective + ' ' + noun + (this.exclaim ? '!' : '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * noun = argv[0];
    struct handlebars_value * exclaim = handlebars_value_map_str_find(options->scope, HBS_STRL("exclaim"));
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s %s%s",
            hbs_str_val(handlebars_value_to_string(adjective)),
            hbs_str_val(handlebars_value_to_string(noun)),
            (handlebars_value_get_boolval(exclaim) ? "!" : "")
    );
    handlebars_value_string_steal(result, tmp);
    return result;
}

FIXTURE_FN(1252527367)
{
    // "function () {\n        return 'winning';\n      }"
    FIXTURE_STRING("winning");
}

FIXTURE_FN(1283397100)
{
    // "function (options) {\n        return options.fn({exclaim: '?'});\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"exclaim\": \"?\"}");
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, context);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(1341397520)
{
    // "function (options) {\n        return options.data && options.data.exclaim;\n      }"
    if( options->data ) {
        return handlebars_value_map_str_find(options->data, HBS_STRL("exclaim"));
    } else {
        return handlebars_value_ctor(CONTEXT);
    }
}

FIXTURE_FN(1582700088)
{
    struct handlebars_value * fun = handlebars_value_map_str_find(options->hash, HBS_STRL("fun"));
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "val is %s",
            hbs_str_val(handlebars_value_to_string(fun))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(1623791204)
{
    struct handlebars_value * noun = handlebars_value_map_str_find(options->hash, HBS_STRL("noun"));
    char * tmp = hbs_str_val(handlebars_value_to_string(noun));
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(1644694756)
{
    // "function (x, y) {\n        return x === y;\n      }"
    struct handlebars_value * x = argv[0];
    struct handlebars_value * y = argv[1];
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
    } else if (handlebars_string_eq(handlebars_value_to_string(x), handlebars_value_to_string(y))) {
        ret = true;
    }
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_boolean(result, ret);
    return result;
}

FIXTURE_FN(1774917451)
{
    // "function (options) { return '<form>' + options.fn(this) + '<\/form>'; }"
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<form>%s</form>",
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(1818365722)
{
    // "function (param) { return 'Hello ' + param; }"
    struct handlebars_value * param = argv[0];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s",
            hbs_str_val(handlebars_value_to_string(param))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(1872958178)
{
    // "function (options) {\n        return options.fn(this);\n      }"
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(1983911259)
{
    struct handlebars_string * ret = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s%s%s%s",
            hbs_str_val(ret),
            hbs_str_val(handlebars_value_to_string(argv[0])),
            hbs_str_val(handlebars_value_to_string(argv[1])),
            hbs_str_val(handlebars_value_to_string(argv[2]))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2084318034)
{
    // This is a dumb test
    // "function (_undefined, _null, options) {\n            return (_undefined === undefined) + ' ' + (_null === null) + ' ' + (typeof options);\n          }"
    struct handlebars_value * arg1 = argv[0];
    char * res = handlebars_talloc_asprintf(options->vm, "%s %s %s",
                                            (handlebars_value_get_type(arg1) == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            (handlebars_value_get_type(arg1) == HANDLEBARS_VALUE_TYPE_NULL ? "true" : "false"),
                                            "object");
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(value, res);
    handlebars_talloc_free(res);
    return value;
}

FIXTURE_FN(2089689191)
{
    // "function link(options) {\n      return '<a href=\"\/people\/' + this.id + '\">' + options.fn(this) + '<\/a>';\n    }"
    struct handlebars_value * id = handlebars_value_map_str_find(options->scope, HBS_STRL("id"));
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<a href=\"/people/%s\">%s</a>",
            hbs_str_val(handlebars_value_to_string(id)),
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2096893161)
{
    // "function () {\n            return 'null!';\n          }"
    FIXTURE_STRING("null!");
}

FIXTURE_FN(2107645267)
{
    // "function (prefix) {\n      return '<a href=\"' + prefix + '\/' + this.url + '\">' + this.text + '<\/a>';\n    }"
    struct handlebars_value * prefix = argv[0];
    struct handlebars_value * url = handlebars_value_map_str_find(options->scope, HBS_STRL("url"));
    struct handlebars_value * text = handlebars_value_map_str_find(options->scope, HBS_STRL("text"));
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<a href=\"%s/%s\">%s</a>",
            hbs_str_val(handlebars_value_to_string(prefix)),
            hbs_str_val(handlebars_value_to_string(url)),
            hbs_str_val(handlebars_value_to_string(text))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2182811123)
{
    // "function (val) {\n        return val + val;\n      }"
    struct handlebars_value * value = argv[0];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s%s",
            hbs_str_val(handlebars_value_to_string(value)),
            hbs_str_val(handlebars_value_to_string(value))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;

}

FIXTURE_FN(2259424295)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(value, "&'\\<>");
    handlebars_value_set_flag(value, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return value;
}

FIXTURE_FN(2262633698)
{
    // "function (a, b) {\n        return a + '-' + b;\n      }"
    struct handlebars_value * a = argv[0];
    struct handlebars_value * b = argv[1];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s-%s",
            hbs_str_val(handlebars_value_to_string(a)),
            hbs_str_val(handlebars_value_to_string(b))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2305563493)
{
    // "function () { return [{text: 'goodbye'}, {text: 'Goodbye'}, {text: 'GOODBYE'}]; }"
    struct handlebars_value * value = handlebars_value_from_json_string(CONTEXT, "[{\"text\": \"goodbye\"}, {\"text\": \"Goodbye\"}, {\"text\": \"GOODBYE\"}]");
    handlebars_value_convert(value);
    return value;
}

FIXTURE_FN(2327777290)
{
    // "function (block) { return block.inverse(''); }"
    struct handlebars_value * context = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(context, "");
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->inverse, context);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(2439252451)
{
    struct handlebars_value * value = options->scope;
    handlebars_value_addref(value);
    return value;
}

FIXTURE_FN(2499873302)
{
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_boolean(value, 0);
    return value;
}

FIXTURE_FN(2515293198)
{
    // "function (param, times, bool1, bool2) {\n        if (typeof times !== 'number') { times = 'NaN'; }\n        if (typeof bool1 !== 'boolean') { bool1 = 'NaB'; }\n        if (typeof bool2 !== 'boolean') { bool2 = 'NaB'; }\n        return 'Hello ' + param + ' ' + times + ' times: ' + bool1 + ' ' + bool2;\n      }"
    struct handlebars_value * param = argv[0];
    struct handlebars_value * times = argv[1];
    struct handlebars_value * bool1 = argv[2];
    struct handlebars_value * bool2 = argv[3];
    // @todo check types
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s %s times: %s %s",
            hbs_str_val(handlebars_value_to_string(param)),
            hbs_str_val(handlebars_value_to_string(times)),
            hbs_str_val(handlebars_value_to_string(bool1)),
            hbs_str_val(handlebars_value_to_string(bool2))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
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
    struct handlebars_value * value = argv[0];
    char * tmp = handlebars_talloc_strdup(options->vm, hbs_str_val(handlebars_value_to_string(value)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    char * tmp2 = handlebars_talloc_asprintf(
            options->vm,
            "cruel %s",
            tmp
    );
    handlebars_talloc_free(tmp);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp2);
    handlebars_talloc_free(tmp2);
    return result;
}

FIXTURE_FN(2596410860)
{
    // "function (context, options) { return options.fn(context); }"
    struct handlebars_value * context = argv[0];
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(value, res);
    return value;
}

FIXTURE_FN(2600345162)
{
    // "function (defaultString) {\n        return new Handlebars.SafeString(defaultString);\n      }"
    struct handlebars_value * context = argv[0];
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(value, hbs_str_val(handlebars_value_to_string(context)));
    handlebars_value_set_flag(value, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return value;
}

FIXTURE_FN(2608073270)
{
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_boolean(result, false);
    return result;
}

FIXTURE_FN(2632597106)
{
    // "function (options) {\n      var out = '';\n      var byes = ['Goodbye', 'goodbye', 'GOODBYE'];\n      for (var i = 0, j = byes.length; i < j; i++) {\n        out += byes[i] + ' ' + options.fn(this) + '! ';\n      }\n      return out;\n    }",
    struct handlebars_string * tmp1 = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    struct handlebars_string * tmp3 = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s %s! %s %s! %s %s! ",
            "Goodbye",
            hbs_str_val(tmp1),
            "goodbye",
            hbs_str_val(tmp2),
            "GOODBYE",
            hbs_str_val(tmp3)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2659134105)
{
    // "function (options) {\n          return 'GOODBYE ' + options.hash.cruel + ' ' + options.hash.world + ' ' + options.hash.times + ' TIMES';\n        }"
    struct handlebars_value * cruel = handlebars_value_map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * world = handlebars_value_map_str_find(options->hash, HBS_STRL("world"));
    struct handlebars_value * times = handlebars_value_map_str_find(options->hash, HBS_STRL("times"));
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "GOODBYE %s %s %s TIMES",
            hbs_str_val(handlebars_value_to_string(cruel)),
            hbs_str_val(handlebars_value_to_string(world)),
            hbs_str_val(handlebars_value_to_string(times))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2736662431)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn({}, {blockParams: [1, 2]});\n        }"
    // @todo equals
    struct handlebars_value * block_params = handlebars_value_from_json_string(CONTEXT, "[1, 2]");
    handlebars_value_convert(block_params);
    struct handlebars_value * context = handlebars_value_ctor(CONTEXT);
    handlebars_value_map_init(context, 0); // zero may trigger extra rehashes - good for testing
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(options->vm, options->program, context, NULL, block_params);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(2795443460)
{
    // "function (options) { return options.fn({text: 'GOODBYE'}); }"
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"text\": \"GOODBYE\"}");
    handlebars_value_convert(context);
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(2818908139)
{
    // "function (options) {\n        return options.fn({exclaim: '?'}, { data: {adjective: 'sad'} });\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"exclaim\": \"?\"}");
    struct handlebars_value * data = handlebars_value_from_json_string(CONTEXT, "{\"adjective\": \"sad\"}");
    struct handlebars_value * exclaim = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(exclaim, "!");
    struct handlebars_string * res = handlebars_vm_execute_program_ex(options->vm, options->program, context, data, NULL);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, res);
    return result;
}

FIXTURE_FN(2842041837)
{
    // "function () {\n        return 'helper missing: ' + arguments[arguments.length - 1].name;\n      }"
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "helper missing: %s",
            hbs_str_val(options->name)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2857704189)
{
    // "function (options) {\n        return new Handlebars.SafeString(options.fn());\n      }"
    struct handlebars_string * tmp = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    handlebars_value_set_flag(result, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    return result;
}

FIXTURE_FN(2919388099)
{
    // "function (options) {\n        var frame = Handlebars.createFrame(options.data);\n        for (var prop in options.hash) {\n          if (prop in options.hash) {\n            frame[prop] = options.hash[prop];\n          }\n        }\n        return options.fn(this, {data: frame});\n      }"
    struct handlebars_map * map = handlebars_map_ctor(CONTEXT, handlebars_value_count(options->data) + handlebars_value_count(options->hash) + 1);

    HANDLEBARS_VALUE_FOREACH_KV(options->data, key, child) {
        handlebars_map_update(map, key, child);
    } HANDLEBARS_VALUE_FOREACH_END();

    HANDLEBARS_VALUE_FOREACH_KV(options->hash, key, child) {
        handlebars_map_update(map, key, child);
    } HANDLEBARS_VALUE_FOREACH_END();

    handlebars_map_str_update(map, HBS_STRL("_parent"), options->data);

    struct handlebars_value * frame = handlebars_value_ctor(CONTEXT);
    // handlebars_value_map_init(frame, 0); // zero may trigger extra rehashes - good for testing
    handlebars_value_map(frame, map);

    struct handlebars_string * res = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, frame, NULL);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, res);
    return result;
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
    struct handlebars_value * adjective = handlebars_value_map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * noun = handlebars_value_map_str_find(options->scope, HBS_STRL("noun"));
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    assert(adjective != NULL);
    assert(noun != NULL);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s %s",
            hbs_str_val(handlebars_value_to_string(adjective)),
            hbs_str_val(handlebars_value_to_string(noun))
    );
    handlebars_value_string_steal(result, tmp);
    return result;
}

FIXTURE_FN(3011980185)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn({value: 'bar'}, {blockParams: [1, 2]});\n        }"
    // @todo equals
    struct handlebars_value * block_params = handlebars_value_from_json_string(CONTEXT, "[1, 2]");
    handlebars_value_convert(block_params);
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"value\": \"bar\"}");
    handlebars_value_convert(context);
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(options->vm, options->program, context, NULL, block_params);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(3058305845)
{
    // "function () {return this.foo; }"
    struct handlebars_value * value = handlebars_value_map_str_find(options->scope, HBS_STRL("foo"));
    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    }
    return value;
}

FIXTURE_FN(3065257350)
{
    // "function (options) {\n          return this.goodbye.toUpperCase() + options.fn(this);\n        }"
    struct handlebars_value * goodbye = handlebars_value_map_str_find(options->scope, HBS_STRL("goodbye"));
    char * tmp = handlebars_talloc_strdup(options->vm, hbs_str_val(handlebars_value_to_string(goodbye)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    tmp = handlebars_talloc_strdup_append(tmp, hbs_str_val(tmp2));
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, tmp);
    handlebars_talloc_free(tmp2);
    return result;
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
    return handlebars_value_map_str_find(options->scope, HBS_STRL("bar"));
}

FIXTURE_FN(3168412868)
{
    // "function (options) {\n        return options.fn({exclaim: '?', zomg: 'world'}, { data: {adjective: 'sad'} });\n      }"
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"exclaim\":\"?\", \"zomg\":\"world\"}");
    struct handlebars_value * data = handlebars_value_from_json_string(CONTEXT, "{\"adjective\": \"sad\"}");
    handlebars_value_convert(context);
    handlebars_value_convert(data);
    struct handlebars_string * res = handlebars_vm_execute_program_ex(options->vm, options->program, context, data, NULL);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, res);
    return result;
}

FIXTURE_FN(3206093801)
{
    // "function (context, options) { return '<form>' + options.fn(context) + '<\/form>'; }"
    struct handlebars_value * context = argv[0];
    struct handlebars_string * res = handlebars_vm_execute_program(options->vm, options->program, context);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "<form>%s</form>",
            hbs_str_val(res)
    );
    handlebars_talloc_free(res);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, tmp);
    return result;
}

FIXTURE_FN(325991858)
{
    // "function (options) {\n      var out = '';\n      var byes = ['Goodbye', 'goodbye', 'GOODBYE'];\n      for (var i = 0, j = byes.length; i < j; i++) {\n        out += byes[i] + ' ' + options.fn({}) + '! ';\n      }\n      return out;\n    }"
    struct handlebars_value * context = handlebars_value_ctor(CONTEXT);
    handlebars_value_addref(context);
    handlebars_value_map_init(context, 0); // zero may trigger extra rehashes - good for testing
    struct handlebars_string * tmp1 = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_string * tmp2 = handlebars_vm_execute_program(options->vm, options->program, context);
    struct handlebars_string * tmp3 = handlebars_vm_execute_program(options->vm, options->program, context);
    handlebars_value_delref(context);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s %s! %s %s! %s %s! ",
            "Goodbye",
            hbs_str_val(tmp1),
            "goodbye",
            hbs_str_val(tmp2),
            "GOODBYE",
            hbs_str_val(tmp3)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, tmp);
    handlebars_talloc_free(tmp1);
    handlebars_talloc_free(tmp2);
    handlebars_talloc_free(tmp3);
    return result;
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
    struct handlebars_value * value = argv[0];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "foo %s",
            hbs_str_val(handlebars_value_to_string(value))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3325763044)
{
    // "function (val, that, theOther) {\n        return 'val is ' + val + ', ' + that + ' and ' + theOther;\n      }"
    struct handlebars_value * val = argv[0];
    struct handlebars_value * that = argv[1];
    struct handlebars_value * theOther = argv[2];
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "val is %s, %s and %s",
            hbs_str_val(handlebars_value_to_string(val)),
            hbs_str_val(handlebars_value_to_string(that)),
            hbs_str_val(handlebars_value_to_string(theOther))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3327136760)
{
    // "function () {\n          return this.goodbye.toUpperCase();\n        }"
    struct handlebars_value * goodbye = handlebars_value_map_str_find(options->scope, HBS_STRL("goodbye"));
    char * tmp = handlebars_talloc_strdup(options->vm, hbs_str_val(handlebars_value_to_string(goodbye)));
    size_t i  = 0;
    while( tmp[i] ) {
        tmp[i] = toupper(tmp[i]);
        i++;
    }
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3328314220)
{
    // "function () { return 'helpers'; }"
    FIXTURE_STRING("helpers");
}

FIXTURE_FN(3379432388)
{
    // "function () { return this.more; }"
    struct handlebars_value * value = handlebars_value_map_str_find(options->scope, HBS_STRL("more"));
    if( !value ) {
        value = handlebars_value_ctor(CONTEXT);
    }
    return value;
}

FIXTURE_FN(3407223629)
{
    // "function () {\n        return 'missing: ' + arguments[arguments.length - 1].name;\n      }",
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "missing: %s",
            hbs_str_val(options->name)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3477736473)
{
    // "function () {return this.world; }"
    return handlebars_value_map_str_find(options->scope, HBS_STRL("world"));
}

FIXTURE_FN(3578728160)
{
    // "function () {\n            return 'undefined!';\n          }"
    struct handlebars_value * value = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(value, "undefined!");
    return value;
}

FIXTURE_FN(3659403207)
{
    // "function (value) {\n        return 'bar ' + value;\n    }"
    struct handlebars_value * arg = argv[0];
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, handlebars_talloc_asprintf(
            arg, "%s%s", "bar ",  hbs_str_val(handlebars_value_to_string(arg))
    ));
    return result;
}

FIXTURE_FN(3691188061)
{
    // "function (val) {\n        return 'val is ' + val;\n      }"
    struct handlebars_value * value = argv[0];
    char * ret = handlebars_talloc_asprintf(
            options->vm,
            "val is %s",
            hbs_str_val(handlebars_value_to_string(value))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, ret);
    handlebars_talloc_free(ret);
    return result;
}

FIXTURE_FN(3697740723)
{
    FIXTURE_STRING("found it!");
}

FIXTURE_FN(3707047013)
{
    // "function (value) { return value; }"
    struct handlebars_value * value = argv[0];
    handlebars_value_addref(value);
    return value;
}

FIXTURE_FN(3728875550)
{
    // "function (options) {\n        return options.data.accessData + ' ' + options.fn({exclaim: '?'});\n      }"
    struct handlebars_value * access_data = handlebars_value_map_str_find(options->data, HBS_STRL("accessData"));
    struct handlebars_value * context = handlebars_value_from_json_string(CONTEXT, "{\"exclaim\": \"?\"}");
    struct handlebars_string * ret = handlebars_vm_execute_program(options->vm, options->program, context);
    char * ret2 = handlebars_talloc_asprintf(
            options->vm,
            "%s %s",
            hbs_str_val(handlebars_value_to_string(access_data)),
            hbs_str_val(ret)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, ret2);
    return result;
}

FIXTURE_FN(3781305181)
{
    // "function (times) {\n      if (typeof times !== 'number') { times = 'NaN'; }\n      return 'Hello ' + times + ' times';\n    }"
    struct handlebars_value * times = argv[0];
    // @todo this should be a float perhaps?
    /* if( times->type != HANDLEBARS_VALUE_TYPE_FLOAT || times->type != HANDLEBARS_VALUE_TYPE_INTEGER ) {
        handlebars_value_string(times, "NaN");
    }*/
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s times",
            hbs_str_val(handlebars_value_to_string(times))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(3878511480)
{
    // "function list(context, options) {\n      if (context.length > 0) {\n        var out = '<ul>';\n        for (var i = 0, j = context.length; i < j; i++) {\n          out += '<li>';\n          out += options.fn(context[i]);\n          out += '<\/li>';\n        }\n        out += '<\/ul>';\n        return out;\n      } else {\n        return '<p>' + options.inverse(this) + '<\/p>';\n      }\n    }"
    struct handlebars_value * context = argv[0];
    char *tmp;
    if( !handlebars_value_is_empty(context) ) {
        tmp = handlebars_talloc_strdup(options->vm, "<ul>");
        HANDLEBARS_VALUE_FOREACH(context, child) {
            struct handlebars_string * tmp2 = handlebars_vm_execute_program(options->vm, options->program, child);
            tmp = handlebars_talloc_asprintf_append(
                    tmp,
                    "<li>%s</li>",
                    hbs_str_val(tmp2)
            );
            handlebars_talloc_free(tmp2);
        } HANDLEBARS_VALUE_FOREACH_END();
        tmp = handlebars_talloc_strdup_append(tmp, "</ul>");
    } else {
        struct handlebars_string * tmp2 = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
        tmp = handlebars_talloc_asprintf(
                options->vm,
                "<p>%s</p>",
                hbs_str_val(tmp2)
        );
        handlebars_talloc_free(tmp2);
    }
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string_steal(result, tmp);
    return result;
}

FIXTURE_FN(3963629287)
{
    FIXTURE_STRING("success");
}

FIXTURE_FN(4005129518)
{
    // "function (options) {\n        var hash = options.hash;\n        var ariaLabel = Handlebars.Utils.escapeExpression(hash['aria-label']);\n        var placeholder = Handlebars.Utils.escapeExpression(hash.placeholder);\n        return new Handlebars.SafeString('<input aria-label=\"' + ariaLabel + '\" placeholder=\"' + placeholder + '\" \/>');\n      }"
    struct handlebars_value * label = handlebars_value_map_str_find(options->hash, HBS_STRL("aria-label"));
    struct handlebars_value * placeholder = handlebars_value_map_str_find(options->hash, HBS_STRL("placeholder"));
    struct handlebars_string * tmp1 = handlebars_value_expression(label, 1);
    struct handlebars_string * tmp2 = handlebars_value_expression(placeholder, 1);
    char * tmp3 = handlebars_talloc_asprintf(
            options->vm,
            "<input aria-label=\"%s\" placeholder=\"%s\" />",
            hbs_str_val(tmp1),
            hbs_str_val(tmp2)
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp3);
    handlebars_value_set_flag(result, HANDLEBARS_VALUE_FLAG_SAFE_STRING);
    handlebars_talloc_free(tmp1);
    handlebars_talloc_free(tmp2);
    handlebars_talloc_free(tmp3);
    return result;
}

FIXTURE_FN(4112130635)
{
    // "function (thing, options) {\n        return options.data.adjective + ' ' + thing + (this.exclaim || '');\n      }"
    struct handlebars_value * adjective = handlebars_value_map_str_find(options->data, HBS_STRL("adjective"));
    struct handlebars_value * thing = argv[0];
    struct handlebars_value * exclaim = handlebars_value_map_str_find(options->scope, HBS_STRL("exclaim"));
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "%s %s%s",
            hbs_str_val(handlebars_value_to_string(adjective)),
            hbs_str_val(handlebars_value_to_string(thing)),
            hbs_str_val(handlebars_value_to_string(exclaim))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(4158918668)
{
    struct handlebars_value * noun = argv[0];
    char * tmp = hbs_str_val(handlebars_value_to_string(noun));
    char * res = handlebars_talloc_asprintf(
            options->vm,
            "Hello %s",
            !*tmp ? "undefined" : tmp
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, res);
    handlebars_talloc_free(res);
    return result;
}

FIXTURE_FN(4204859626)
{
    struct handlebars_string * res = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, NULL, NULL);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, res);
    return result;
}

FIXTURE_FN(4207421535)
{
    // "function (options) {\n          equals(options.fn.blockParams, 1);\n          return options.fn(this, {blockParams: [1, 2]});\n        }"
    // @todo equals
    struct handlebars_value * block_params = handlebars_value_from_json_string(CONTEXT, "[1, 2]");
    handlebars_value_convert(block_params);
    struct handlebars_string * tmp = handlebars_vm_execute_program_ex(options->vm, options->program, options->scope, NULL, block_params);
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, tmp);
    return result;
}

FIXTURE_FN(1414406764)
{
    // function testHelper(options) {\n          return options.lookupProperty(this, 'testProperty');\n        }
    return handlebars_value_map_str_find(options->scope, HBS_STRL("testProperty"));
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
    struct handlebars_value * print = handlebars_value_map_str_find(options->hash, HBS_STRL("print"));
    struct handlebars_value * cruel = handlebars_value_map_str_find(options->hash, HBS_STRL("cruel"));
    struct handlebars_value * world = handlebars_value_map_str_find(options->hash, HBS_STRL("world"));

    if (print && handlebars_value_get_type(print) == HANDLEBARS_VALUE_TYPE_TRUE) {
        char * tmp = handlebars_talloc_asprintf(
                options->vm,
                "GOODBYE %s %s",
                hbs_str_val(handlebars_value_to_string(cruel)),
                hbs_str_val(handlebars_value_to_string(world))
        );
        struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
        handlebars_value_string(result, tmp);
        handlebars_talloc_free(tmp);
        return result;
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
    struct handlebars_value * conditional = argv[0];
    struct handlebars_string * str;
    if (!handlebars_value_is_empty(conditional)) {
        // assert(options->program > 0);
        str = handlebars_vm_execute_program(options->vm, options->program, options->scope);
    } else {
        // assert(options->inverse > 0);
        str = handlebars_vm_execute_program(options->vm, options->inverse, options->scope);
    }
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_str_steal(result, str);
    return result;
}

FIXTURE_FN(2855343161)
{
    // function(options) {\n        return options.hash.length;\n      }
    return handlebars_value_map_str_find(options->hash, HBS_STRL("length"));
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

    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    if (handlebars_value_get_type(argv[0]) == HANDLEBARS_VALUE_TYPE_TRUE && handlebars_value_get_type(argv[1]) == HANDLEBARS_VALUE_TYPE_TRUE) {
        handlebars_value_boolean(result, true);
    } else {
        handlebars_value_boolean(result, false);
    }
    return result;
}

FIXTURE_FN(1561073198)
{
    // function (options) {\n        return \"val is \" + options.hash.fun;\n      }
    struct handlebars_value * fun = handlebars_value_map_str_find(options->hash, HBS_STRL("fun"));
    assert(fun != NULL);
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "val is %s",
            hbs_str_val(handlebars_value_to_string(fun))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
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
    struct handlebars_value * context = argv[0];
    struct handlebars_string * str = handlebars_value_to_string(context);
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
            options->vm,
            "%s%s%s",
            hbs_str_val(handlebars_value_to_string(argv[0])),
            "{{planet}}",
            hbs_str_val(handlebars_value_to_string(argv[0]))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2718175385)
{
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    // return $text . "{{planet}} => |planet|" . $text;
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s%s%s",
            hbs_str_val(handlebars_value_to_string(argv[0])),
            "{{planet}} => |planet|",
            hbs_str_val(handlebars_value_to_string(argv[0]))
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(2000357317)
{
    if (argc < 1) {
        FIXTURE_STRING("must be run with mustache style lambdas")
    }
    // return "__" . $text . "__";
    char * tmp = handlebars_talloc_asprintf(
            options->vm,
            "%s%s%s",
            "__",
            hbs_str_val(handlebars_value_to_string(argv[0])),
            "__"
    );
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_string(result, tmp);
    handlebars_talloc_free(tmp);
    return result;
}

FIXTURE_FN(617219335)
{
    // return false;
    struct handlebars_value * result = handlebars_value_ctor(CONTEXT);
    handlebars_value_boolean(result, 0);
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
    handlebars_value_helper(value, func); \

    assert(handlebars_value_get_real_type(value) == HANDLEBARS_VALUE_TYPE_MAP);

    struct handlebars_value * jsvalue = handlebars_value_map_str_find(value, HBS_STRL("javascript"));
    if( !jsvalue ) {
        jsvalue = handlebars_value_map_str_find(value, HBS_STRL("php"));
    }
    assert(jsvalue != NULL);
    assert(handlebars_value_get_real_type(jsvalue) == HANDLEBARS_VALUE_TYPE_STRING);
    uint32_t hash = adler32((unsigned char *) hbs_str_val(handlebars_value_to_string(jsvalue)), handlebars_value_get_strlen(jsvalue));

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
            fprintf(stderr, "Unimplemented test fixture [%u]:\n%s\n", hash, hbs_str_val(handlebars_value_to_string(jsvalue)));
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
            child = handlebars_value_map_str_find(value, HBS_STRL("!code"));
            if (!child) {
                // this is for mustache, getting at the tags is a pain, so assume any object with a "php" key is code
                child = handlebars_value_map_str_find(value, HBS_STRL("php"));
            }
            if( child ) {
                // Convert to helper
                convert_value_to_fixture(value);
            } else {
                // Recurse
                HANDLEBARS_VALUE_FOREACH(value, child) {
                    load_fixtures(child);
                } HANDLEBARS_VALUE_FOREACH_END();
            }
            break;
        case HANDLEBARS_VALUE_TYPE_ARRAY:
            HANDLEBARS_VALUE_FOREACH(value, child) {
                load_fixtures(child);
            } HANDLEBARS_VALUE_FOREACH_END();
            break;
        default:
            // do nothing
            break;
    }
}

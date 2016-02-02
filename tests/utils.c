
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pcre.h>
#include <talloc.h>

#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(HAVE_JSON_C_JSON_H)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "utils.h"
#include "handlebars_compiler.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"


const int MOD_ADLER = 65521;

int file_get_contents(const char * filename, char ** buf, size_t * len)
{
	FILE * f;
	
	// Open the file
	f = fopen(filename, "rb");
	if( f == NULL ) {
		fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
		return -1;
	}
	
	// Get length
	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	*buf = malloc((*len + 1) * sizeof(char));
	if( *buf == NULL ) {
		fclose(f);
		return -2;
	}
	
	if( fread(*buf, 1, *len, f) != *len ) {
		free(*buf);
		*buf = NULL;
		fclose(f);
		return -3;
	}
	(*buf)[*len] = '\0';
	
	fclose(f);
	return 0;
}

int scan_directory_callback(char * dirname, scan_directory_cb cb)
{
	DIR * dir = NULL;
	struct dirent * ent = NULL;
	int error = 0;
	
	// Open the directory
	if( (dir = opendir(dirname)) == NULL ) {
		return -1;
	}
	
	// Read the directory
	while( (ent = readdir(dir)) != NULL ) {
		if( ent->d_name[0] == '.' ) continue;
		//if( strlen(ent->d_name) < 5 ) continue;
		//if( strcmp(ent->d_name + strlen(ent->d_name) - 4, ".yml") != 0 ) continue;
		//if( *(ent->d_name) == '~' ) continue; // Ignore lambdas
		if( strlen(ent->d_name) + strlen(dirname) + 1 >= 128 ) continue; // fear
		
		// Make filename
		char filename[128];
		snprintf(filename, 128, "%s/%s", dirname, ent->d_name);
		
		// Callback
		cb(filename);
	}
	
	if( dir != NULL) closedir(dir);
	return error;
}

int regex_compare(const char * regex, const char * string, char ** error)
{
    pcre * re;
    char * errmsg = NULL;
    int erroffset;
    int ovector[30];
    int rc, i, ret;
    
    re = pcre_compile(regex, 0, &errmsg, &erroffset, NULL);
    
    if( !re ) {
        *error = talloc_asprintf(NULL, "Regex '%s' compilation failed at offset %d: %s\n", regex, erroffset, errmsg);
        return 1;
    } else if( errmsg ) {
        *error = talloc_strdup(NULL, errmsg);
        ret = 2;
        goto error;
    }
    
    rc = pcre_exec(re, NULL, string, (int) strlen(string), 0, 0, ovector, 30);
    if( rc <= 0 ) {
        ret = 2;
        *error = talloc_asprintf(NULL, "Regex '%s' didn't match string '%s'", regex, string);
    } else {
        ret = 0;
    }
    
error:
    pcre_free(re);
    return ret;
}

// https://en.wikipedia.org/wiki/Adler-32
uint32_t adler32(unsigned char *data, size_t len)
{
	uint32_t a = 1, b = 0;
	size_t index;

	/* Process each byte of the data in order */
	for (index = 0; index < len; ++index)
	{
		a = (a + data[index]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}

	return (b << 16) | a;
}



/* Loaders */

long json_load_compile_flags(struct json_object * object)
{
    long flags = 0;
    json_object * cur = NULL;

    if( (cur = json_object_object_get(object, "compat")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_compat;
    }
    if( (cur = json_object_object_get(object, "data")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_compat; // @todo correct?
    }
    if( (cur = json_object_object_get(object, "knownHelpersOnly")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_known_helpers_only;
    }
    if( (cur = json_object_object_get(object, "stringParams")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_string_params;
    }
    if( (cur = json_object_object_get(object, "trackIds")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_track_ids;
    }
    if( (cur = json_object_object_get(object, "preventIndent")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_prevent_indent;
    }
    if( (cur = json_object_object_get(object, "explicitPartialContext")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_explicit_partial_context;
    }
    if( (cur = json_object_object_get(object, "ignoreStandalone")) && json_object_get_boolean(cur) ) {
        flags |= handlebars_compiler_flag_ignore_standalone;
    }

    return flags;
}



/* Helpers/Lambdas */

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

FIXTURE_FN(1341397520)
{
    // "function (options) {\n        return options.data && options.data.exclaim;\n      }"
    if( options->data ) {
        return handlebars_value_map_find(options->data, "exclaim");
    } else {
        return handlebars_value_ctor(options->vm);
    }
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

FIXTURE_FN(3058305845)
{
    // "function () {return this.foo; }"
    struct handlebars_value * value = handlebars_value_map_find(options->scope, "foo");
    if( !value ) {
        value = handlebars_value_ctor(options->vm);
    }
    return value;
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
        FIXTURE_CASE(1341397520);
        FIXTURE_CASE(2084318034);
        FIXTURE_CASE(2096893161);
        FIXTURE_CASE(2259424295);
        FIXTURE_CASE(2305563493);
        FIXTURE_CASE(2439252451);
        FIXTURE_CASE(2499873302);
        FIXTURE_CASE(2554595758);
        FIXTURE_CASE(2596410860);
        FIXTURE_CASE(3058305845);
        FIXTURE_CASE(3307473738);
        FIXTURE_CASE(3379432388);
        FIXTURE_CASE(3578728160);
        FIXTURE_CASE(3659403207);
        FIXTURE_CASE(3707047013);

        FIXTURE_CASE_ALIAS(401083957, 3707047013);
        FIXTURE_CASE_ALIAS(1111103580, 1341397520);
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


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
#include <src/handlebars_value.h>

#include "utils.h"
#include "handlebars_value.h"


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



/* Helpers/Lambdas */

#define FIXTURE_FN(hash) static struct handlebars_value * fixture_ ## hash(struct handlebars_options * options)
#define FIXTURE_STRING(string) \
    struct handlebars_value * value = handlebars_value_ctor(options->vm); \
    handlebars_value_string(value, string); \
    return value;

FIXTURE_FN(739773491)
{
    // "function (arg) {\n        return arg;\n      }"
    return handlebars_stack_get(options->params, 0);
}

FIXTURE_FN(2554595758)
{
    // "function () { return 'bar'; }"
    FIXTURE_STRING("bar");
}

FIXTURE_FN(3578728160)
{
    // "function () {\n            return 'undefined!';\n          }"
    struct handlebars_value * value = handlebars_value_ctor(options->vm);
    handlebars_value_string(value, "undefined!");
    return value;
}

static void convert_value_to_fixture(struct handlebars_value * value)
{
#define HASH_FIXTURE(hash) \
    case hash: \
        SET_FUNCTION(fixture_ ## hash); \
        break

#define SET_FUNCTION(func) \
    handlebars_value_null(value); \
    value->type = HANDLEBARS_VALUE_TYPE_HELPER; \
    value->v.helper = &func;

    assert(value->type == HANDLEBARS_VALUE_TYPE_MAP);

    struct handlebars_value * jsvalue = handlebars_value_map_find(value, "javascript", sizeof("javascript"));
    assert(jsvalue != NULL);
    assert(jsvalue->type == HANDLEBARS_VALUE_TYPE_STRING);
    uint32_t hash = adler32(jsvalue->v.strval, strlen(jsvalue->v.strval));

    switch( hash ) {
        HASH_FIXTURE(739773491);
        HASH_FIXTURE(2554595758);
        HASH_FIXTURE(3578728160);
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
            child = handlebars_value_map_find(value, "!code", sizeof("!code") - 1);
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

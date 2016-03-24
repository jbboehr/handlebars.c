
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_helpers.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"

const int MOD_ADLER = 65521;



TALLOC_CTX * root;
struct handlebars_context * context;
struct handlebars_parser * parser;
struct handlebars_compiler * compiler;
struct handlebars_vm * vm;
int init_blocks;

void default_setup(void)
{
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_disable();
#endif
    context = handlebars_context_ctor_ex(root);
    parser = handlebars_parser_ctor(context);
    compiler = handlebars_compiler_ctor(context);
    vm = handlebars_vm_ctor(context);
    init_blocks = talloc_total_blocks(context);
}

void default_teardown(void)
{
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_disable();
#endif
    handlebars_vm_dtor(vm);
    handlebars_compiler_dtor(compiler);
    handlebars_parser_dtor(parser);
    handlebars_context_dtor(context);
    vm = NULL;
    compiler = NULL;
    parser = NULL;
    context = NULL;
}



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
    const char * errmsg = NULL;
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

char ** json_load_known_helpers(void * ctx, struct json_object * object)
{
    struct json_object * array_item = NULL;
    int array_len = 0;
    // Let's just allocate a nice fat array >.>
    char ** known_helpers = talloc_zero_array(ctx, char *, 32);
    char ** ptr = known_helpers;
    const char ** ptr2 = handlebars_builtins_names();

    for( ; *ptr2 ; ++ptr2 ) {
        *ptr = handlebars_talloc_strdup(ctx, *ptr2);
        ptr++;
    }

    json_object_object_foreach(object, key, value) {
        *ptr = handlebars_talloc_strdup(ctx, key);
        ptr++;
    }

    *ptr++ = NULL;

    return known_helpers;
}


/* Helpers/Lambdas */

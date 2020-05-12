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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pcre.h>
#include <talloc.h>

#include <errno.h>
#include <dirent.h>
#ifndef YY_NO_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include <check.h>

#if defined(HAVE_LIBYAML)
#include <yaml.h>
#endif

#if defined(HAVE_JSON_C_JSON_H) || defined(JSONC_INCLUDE_WITH_C)
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#elif defined(HAVE_JSON_JSON_H) || defined(HAVE_LIBJSONC)
#include <json/json.h>
#include <json/json_object.h>
#include <json/json_tokener.h>
#endif

#include "handlebars.h"
#include "handlebars_compiler.h"
#include "handlebars_parser.h"
#include "handlebars_helpers.h"
#include "handlebars_string.h"
#include "handlebars_value.h"
#include "handlebars_vm.h"
#include "utils.h"


TALLOC_CTX * root = NULL;
struct handlebars_context * context;
struct handlebars_parser * parser;
struct handlebars_compiler * compiler;
struct handlebars_vm * vm;
size_t init_blocks;
static size_t root_blocks;

void default_setup(void)
{
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_disable();
#endif
    if (!root) {
        root = talloc_init(NULL);
    }
    root_blocks = talloc_total_blocks(root);
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
    // Make sure we aren't leaking anything
    ck_assert_uint_eq(root_blocks, talloc_total_blocks(root));
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
		char *filename = talloc_asprintf(NULL, "%s/%s", dirname, ent->d_name);
		// snprintf(filename, 254, "%s/%s", dirname, ent->d_name);
		cb(filename);
        talloc_free(filename);
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
    int rc, ret;

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

char * normalize_template_whitespace(TALLOC_CTX *ctx, struct handlebars_string * str)
{
    char *i = hbs_str_val(str);
    char *ret = handlebars_talloc_size(context, hbs_str_len(str) + 1);
    char *j = ret;
    while (1) {
        switch (*i) {
            case 0:
                *j++ = 0;
                return ret;

            case '\'':
                *j++ = '"';
                break;

            case '~': // sadface
            case '\r':
            case '\n':
            case '\t':
            case '\v':
                // ignore
                break;

            case ' ':
                // ignore
                // if (!in_ws) {
                //     *j++ = ' ';
                // }
                // in_ws = 1;
                break;
            default:
                // in_ws = 0;
                *j++ = *i;
                break;
        }
        i++;
    }
}

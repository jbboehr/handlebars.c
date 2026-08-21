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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pcre2.h>
#include <talloc.h>

#include <errno.h>
#include <dirent.h>
#ifndef YY_NO_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#include <check.h>

#ifdef HANDLEBARS_HAVE_JSON
// json-c undeprecated json_object_object_get, but the version in xenial
// is too old, so let's silence deprecated warnings for json-c
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <json.h>
#include <json_object.h>
#include <json_tokener.h>
#pragma GCC diagnostic pop
#endif

#include "handlebars.h"
#include "handlebars_memory.h"
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
static size_t null_blocks;

void clear_intentional_error(void)
{
    handlebars_talloc_free((char *) context->e->msg);
    context->e->msg = NULL;
    context->e->num = HANDLEBARS_SUCCESS;
}

void default_setup(void)
{
#ifdef HANDLEBARS_MEMORY
    handlebars_memory_fail_disable();
#endif
    if (!root) {
        root = talloc_init(NULL);
    }
    null_blocks = talloc_total_blocks(NULL);
    root_blocks = talloc_total_blocks(root);
#if 0
    do {
        FILE * tmp = fopen("./tmp1", "w");
        talloc_report_full(NULL, tmp);
        fclose(tmp);
    } while(0);
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
    // Make sure we aren't leaking anything
#if 0
    if (root_blocks != talloc_total_blocks(root)) {
        FILE * tmp = fopen("./tmp2", "w");
        talloc_report_full(NULL, tmp);
        fclose(tmp);
        abort();
    }
#endif
    // These used to be ck_assert_uint_eq but stuff was freeing
    // globals in mustache spec tests... so we changed it for now
    ck_assert_uint_le(talloc_total_blocks(root), root_blocks);
    ck_assert_uint_le(talloc_total_blocks(NULL), null_blocks);
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
    pcre2_code *re;
    int errorcode = 0;
    PCRE2_UCHAR errmsg[128];
    PCRE2_SIZE erroffset;
    pcre2_match_data *match_data;
    int rc, ret;

    re = pcre2_compile((PCRE2_SPTR)regex, PCRE2_ZERO_TERMINATED, 0, &errorcode, &erroffset, NULL);

    if (!re) {
        if( pcre2_get_error_message(errorcode, errmsg, sizeof(errmsg)) < 0 ) {
            snprintf((char *) errmsg, sizeof(errmsg), "PCRE2 error %d", errorcode);
        }
        *error = talloc_asprintf(
            NULL,
            "Regex '%s' compilation failed at offset %zu: %s",
            regex,
            erroffset,
            (const char *) errmsg
        );
        return 1;
    }

    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    if( !match_data ) {
        *error = talloc_strdup(NULL, "Failed to allocate PCRE2 match data");
        pcre2_code_free(re);
        return 1;
    }

    rc = pcre2_match(re, (PCRE2_SPTR)string, (PCRE2_SIZE)strlen(string), 0, 0, match_data, NULL);
    if( rc == PCRE2_ERROR_NOMATCH ) {
        ret = 2;
        *error = talloc_asprintf(NULL, "Regex '%s' didn't match string '%s'", regex, string);
    } else if( rc < 0 ) {
        if( pcre2_get_error_message(rc, errmsg, sizeof(errmsg)) < 0 ) {
            snprintf((char *) errmsg, sizeof(errmsg), "PCRE2 error %d", rc);
        }
        ret = 2;
        *error = talloc_asprintf(NULL, "Regex '%s' match failed: %s", regex, (const char *) errmsg);
    } else {
        ret = 0;
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

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

#ifdef HANDLEBARS_HAVE_JSON
int hbs_test_json_dtor(struct hbs_test_json_holder * holder)
{
    if (holder && holder->obj) {
        json_object_put(holder->obj);
        holder->obj = NULL;
    }

    return 0;
}
#endif

int default_main(suite_ctor_func suite_ctor)
{
    int number_failed;
    int exit_code;

    talloc_set_log_stderr();
    talloc_enable_null_tracking();

    // Setup root memory context
    if (!root) {
        root = talloc_new(NULL);
    }

    // Set up test suite
    Suite * s = suite_ctor();
    SRunner * sr = srunner_create(s);
#if IS_WIN
    srunner_set_fork_status(sr, CK_NOFORK);
#endif
#ifdef HANDLEBARS_HAVE_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        srunner_set_fork_status(sr, CK_NOFORK);
    }
#endif
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    exit_code = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

    if (root) {
        talloc_free(root);
    }

    // Generate report for memdebug
    if (talloc_total_size(NULL) > 0) {
        talloc_report_full(NULL, stderr);
        fprintf(stderr, "Talloc total size was greater than zero: %zu\n", talloc_total_size(NULL));
        exit_code = EXIT_FAILURE;
    }

    // If we don't do this, valgrind reports the null context as a leak
    talloc_disable_null_tracking();

    // Return
    return exit_code;
}

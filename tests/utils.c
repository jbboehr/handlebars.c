
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.h"

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

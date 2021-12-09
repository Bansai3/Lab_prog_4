#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Header.h"


typedef enum { cmd_nop, cmd_show, cmd_get, cmd_set } parsed_command_t;


int main(int argz, char* argv[])
{
	char* filename = 0;
	parsed_command_t parsed_command = cmd_nop;
	char* value = 0, * param_name = 0;
	if (argz == 1 || strcmp(argv[0], "--help") == 0) {
		printf("\nShow or change id3v2 info in MP3 file\n\n");
		printf(" argz:\n");
		printf("	--help                 display this help\n");
		printf("mandatory:\n");
		printf("	--filename=path\\to\\file.mp3     path to MP3 file\n");
		printf("one of the commands:\n");
		printf("	--show                 display all tags\n");
		printf("	--get=param            display param tag value\n");
		printf("	--set=param --value=value    set tag 'param' to 'value'\n\n");

		return 0;
	}

	for (int n = 0; n < argz; n++) {
		if (strncmp("--filepath=", argv[n], 11) == 0) {
			int fname_length = strlen(argv[n] + 11);
			filename = (char*)malloc(fname_length + 1);
			strcpy(filename, argv[n] + 11);
			continue;
		}
		if (strcmp("--show", argv[n]) == 0) {
			parsed_command = cmd_show;
			continue;
		}
		if (strncmp("--get=", argv[n], 6) == 0) {
			int paramlength = strlen(argv[n] + 6);
			parsed_command = cmd_get;
			param_name = (char*)malloc(paramlength + 1);
			strcpy(param_name, argv[n] + 6);
			continue;
		}
		if (strncmp("--set=", argv[n], 6) == 0) {
			int paramlength = strlen(argv[n] + 6);
			parsed_command = cmd_set;
			param_name = (char*)malloc(paramlength + 1);
			strcpy(param_name, argv[n] + 6);
			continue;
		}
		if (strncmp("--value", argv[n], 6) == 0) {
			int vallength = strlen(argv[n] + 8);
			value = (char*)malloc(vallength + 1);
			strcpy(value, argv[n] + 8);
			continue;
		}
	}

	if (!filename || strlen(filename) == 0) {
		printf("ERROR: filename not specified\n\n");
		if (filename) free(filename);
		return -1;
	}

	switch (parsed_command) {
	case cmd_show:
		do_show_command(filename);
		break;
	case cmd_get:
		do_get_command(filename, param_name);
		break;
	case cmd_set:
		printf("set command\n");
		if (param_name) printf("param=%s\n", param_name);
		if (value) printf("value=%s\n", value);
		do_set_command(filename, param_name, value);
		break;
	default:

		printf("no or invalid command");
		return -1;
	}
	return 0;
}


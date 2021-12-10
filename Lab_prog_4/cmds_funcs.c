#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Header.h"


int do_show_command(char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) return -1;
	int code = 0;
	struct id3_record* rec = init_empty_id3record();
	if (fill_id3record(rec, file) == 0) {
		show_id3record(rec);
	}
	else {
		code = -2;
	}
	free_id3record(rec);
	fclose(file);
	return code;
}

int do_get_command(char* filename, char* fieldname) {
	FILE* file = fopen(filename, "rb");
	if (!file) return -1;
	int code = 0;
	struct id3_record* rec = init_empty_id3record();
	if (fill_id3record(rec, file) == 0) {
		char* value = getFieldValue(rec, fieldname);
		if (!value) {
			printf("field:%s  not found !!!\n", fieldname);
		}
	}
	else {
		code = -2;
	}
	fclose(file);
	return code;
}

int do_set_command(char* filename, char* fieldname, char* value) {
	FILE* file = fopen(filename, "r+b");
	if (!file) return -1;
	int code = 0;
	struct id3_record* rec = init_empty_id3record();
	if (fill_id3record(rec, file) == 0) {
		setFieldValue(rec, fieldname, value);
		write_id3record(rec, file);
		show_id3record(rec);
		free_id3record(rec);
	}
	else {
		code = -2;
	}
	fclose(file);
	return code;
}

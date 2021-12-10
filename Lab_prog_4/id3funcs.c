#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Header.h"

unsigned long syncsafeBEtolong(unsigned long val) {
	unsigned long tmp = (val & 0xFF000000) >> 24 |
		(val & 0x00FF0000) >> 8 |
		(val & 0x0000FF00) << 8 |
		(val & 0x000000FF) << 24;

	unsigned long res1 = (tmp & 0x0000007F) |
		((tmp & 0x00007F00) >> 1) |
		((tmp & 0x007F0000) >> 2) |
		((tmp & 0x7F000000) >> 3);
	return res1;
}

unsigned long longtoBEsyncsafe(unsigned long val) {

	unsigned long res1 = (val & 0x0000007F) |
		((val & 0xFFFFFF80) << 1) |
		((val & 0xFFFF8000) << 1) |
		((val & 0xFF800000) << 1);


	unsigned long tmp = (val & 0xFF000000) >> 24 |
		(val & 0x00FF0000) >> 8 |
		(val & 0x0000FF00) << 8 |
		(val & 0x000000FF) << 24;
	return tmp;
}

struct id3_record* init_empty_id3record() {
	struct id3_record* rec = (struct id3_record*)malloc(sizeof(struct id3_record));
	rec->first = 0;
	rec->flags = 0;
	rec->size = 0;
	rec->version[0] = 0;
	rec->version[1] = 0;
	return rec;
}

void free_id3record(struct id3_record* rec) {
	struct id3_frame* p = rec->first;
	while (p != 0) {
		struct id3_frame* pnext = p->next;
		free(p);
		p = pnext;
	}
	free(rec);
}

int  fill_id3record(struct id3_record* dst, FILE* file) {
	if (!file) return -3;
	fseek(file, 0, SEEK_SET);
	char id3_label[4] = { 0,0,0,0 };
	unsigned long bytes_read = 0;

	int read_bytes = fread(id3_label, 1, 3, file);

	if (strcmp("ID3", id3_label) != 0) {
		return -1;
	}

	dst->version[0] = fgetc(file);
	dst->version[1] = fgetc(file);
	dst->flags = fgetc(file);
	int size;
	fread(&size, 1, 4, file);
	dst->size = syncsafeBEtolong(size);

	struct id3_frame* fr = 0, * prev = 0;
	while (bytes_read < dst->size) {
		int frame_bytes_read = 0;

		char tagname[5];
		fread(tagname, 1, 4, file);

		if (tagname[0] == 0) { break; }

		tagname[4] = 0;
		fr = (struct id3_frame*)malloc(sizeof(struct id3_frame));
		fr->value = 0;
		fr->next = 0;
		strcpy(fr->id, tagname);
		int size;
		fread(&size, 1, 4, file);
		fr->size = syncsafeBEtolong(size);
		fr->flags[0] = fgetc(file);
		fr->flags[1] = fgetc(file);

		bytes_read += 10;

		fr->value = (char*)malloc(fr->size);
		fread(fr->value, 1, fr->size, file);
		bytes_read += fr->size;
		if (prev == 0) {
			dst->first = fr;
		}
		else {
			prev->next = fr;
		}
		prev = fr;
	}
	return 0;
}


int  write_id3record(struct id3_record* rec, FILE* file) {
	unsigned long bytes_written = 0;
	char id3_label[4] = { 0,0,0,0 };
	unsigned long bytes_read = 0;
	fseek(file, 0, SEEK_SET);
	fwrite("ID3", 1, 3, file);
	fwrite(rec->version, 2, 1, file);
	fwrite(&rec->flags, 1, 1, file);
	int sz = longtoBEsyncsafe(rec->size);
	fwrite(&sz, 4, 1, file);
	bytes_written += 10;
	struct id3_frame* fr = rec->first;
	while (fr != 0) {
		fwrite(fr->id, 1, 4, file);
		sz = longtoBEsyncsafe(fr->size);
		fwrite(&sz, 1, 4, file);
		fwrite(fr->flags, 1, 2, file);
		fwrite(fr->value, 1, fr->size, file);
		bytes_written += 10 + fr->size;
		fr = fr->next;
	}
	while (bytes_written - 10 < rec->size) {
		fputc('\0', file);
		bytes_written++;
	}
	fclose(file);
	return 0;
}

void printTextFrameValue(struct id3_frame* fr) {
	int enc = fr->value[0];
	switch (enc) {
	case 0: for (int i = 1; i < fr->size; i++)
		putchar(fr->value[i]);
		break;
	}

}

char* getFieldValue(struct id3_record* id3, const char* field) {
	struct id3_frame* fr = id3->first;
	while (fr != 0) {
		if (strcmp(fr->id, field) == 0) {
			printf(" %s: ", fr->id);
			if (fr->id[0] == 'T') {
				printTextFrameValue(fr);
			}
			else {
				printf("%s", fr->value);
			}
			printf("\n");
			return fr->value;
		}
		fr = fr->next;
	}
	return 0;
}

void show_id3record(struct id3_record* id3) {
	if (!id3) {
		printf("<empty> \n");
		return;
	}
	struct id3_frame* fr = id3->first;
	while (fr != 0) {
		printf(" %s: ", fr->id);
		if (fr->id[0] == 'T') {
			printTextFrameValue(fr);
		}
		else{
			printf("%s", fr->value);
		}
		printf("\n");
		fr = fr->next;
	}
}

int setFieldValue(struct id3_record* id3, const char* field, const char* value) {
	struct id3_frame* fr = id3->first;
	while (fr != 0) {
		if (strcmp(fr->id, field) == 0) break;
		fr = fr->next;
	}
	if (fr == 0) {
		fr->next = (struct id3_frame*)malloc(sizeof(struct id3_frame));
		fr = fr->next;
		strncpy(fr->id, field, 4);
		fr->id[4] = '\0';
		fr->flags[2] = '1';
	}
	if (fr->value != 0) free(fr->value);
	if (fr->id[0] == 'T') {
		fr->value = (char*)malloc(strlen(value) + 1);
		fr->value[0] = 0;
		memcpy(fr->value + 1, value, strlen(value));
		fr->size = strlen(value) + 1;
	}
	else {
		memcpy(fr->value, value, strlen(value) + 1);
		fr->size = strlen(value) + 1;
	}
	return 0;
}


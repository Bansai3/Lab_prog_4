#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Header.h"

unsigned long syncsafeBEtolong(unsigned long val) {
	/* from Little Endian */
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

	/* from Little Endian */
	unsigned long tmp = (val & 0xFF000000) >> 24 |
		(val & 0x00FF0000) >> 8 |
		(val & 0x0000FF00) << 8 |
		(val & 0x000000FF) << 24;
	return tmp;
}

/* создать в памяти пустую запись*/
struct id3_record* init_empty_id3record() {
	struct id3_record* rec = (struct id3_record*)malloc(sizeof(struct id3_record));
	rec->first = 0;
	rec->flags = 0;
	rec->size = 0;
	rec->version[0] = 0;
	rec->version[1] = 0;
	return rec;
}

/* удалить из памяти пустую запись*/
void free_id3record(struct id3_record* rec) {
	struct id3_frame* p = rec->first; /* сначала убиваем фреймы*/
	while (p != 0) {
		struct id3_frame* pnext = p->next;
		//if (p->value != 0) free(p->value);
		free(p);
		p = pnext;
	}
	free(rec);
}

/* заполнить запись из файла*/
int  fill_id3record(struct id3_record* dst, FILE* file) {
	if (!file) return -3;
	fseek(file, 0, SEEK_SET);
	//unsigned long pos = ftell(file);

	// ищем "ID3" - скорее всего в начале
	// лоховской вариант -- пусть они или в начале или нет
	char id3_label[4] = { 0,0,0,0 };
	unsigned long bytes_read = 0;

	int read_bytes = fread(id3_label, 1, 3, file);

	if (strcmp("ID3", id3_label) != 0) {
		return -1;
	}

	/*id3 найден - заполняем заголовок*/
	dst->version[0] = fgetc(file);
	dst->version[1] = fgetc(file);
	dst->flags = fgetc(file);
	int size;
	fread(&size, 1, 4, file);
	/* synced little endian -> normal big endian */
	dst->size = syncsafeBEtolong(size);

	/*TODO: extended headers?*/

	/* читаем фреймы*/
	id3_frame* fr = 0, * prev = 0;
	while (bytes_read < dst->size) {
		/*long pos = ftell(file);*/
		/*printf("current pos: %x", pos);*/
		int frame_bytes_read = 0;

		char tagname[5];
		fread(tagname, 1, 4, file);

		/* если tag пустой -- значит конец кадров */
		if (tagname[0] == 0) { break; }

		tagname[4] = 0;
		fr = (id3_frame*)malloc(sizeof(id3_frame));
		fr->value = 0;
		fr->next = 0;
		strcpy(fr->id, tagname);
		/*printf("tag: %s;\n", fr->id);*/
		int size;
		fread(&size, 1, 4, file);
		/* syncsafe little endian -> normal big endian */
		fr->size = syncsafeBEtolong(size);
		fr->flags[0] = fgetc(file);
		fr->flags[1] = fgetc(file); // 7th bit always 1 ?

		bytes_read += 10; // 10 bytes of header read

		// value: may be any type, if string: may start with an encoding
		fr->value = (char*)malloc(fr->size);
		fread(fr->value, 1, fr->size, file);
		bytes_read += fr->size;
		if (prev == 0) { // первый фрейм цепляем к заголовку
			dst->first = fr;
		}
		else { // остальные - к предыдущим фреймам
			prev->next = fr;
		}
		prev = fr;
	}
	/*printf("\n\n");*/
	return 0;
}


int  write_id3record(struct id3_record* rec, FILE* file) {
	//"ID3" - скорее всего в начале
	// лоховской вариант -- пусть они или в начале или нет
	// size достаточно большой чтобы содкржать много филлеров
	unsigned long bytes_written = 0;
	char id3_label[4] = { 0,0,0,0 };
	unsigned long bytes_read = 0;
	fseek(file, 0, SEEK_SET);
	fwrite("ID3", 1, 3, file);
	// version
	fwrite(rec->version, 2, 1, file);
	// flags
	fwrite(&rec->flags, 1, 1, file);
	// unsync size;
	int sz = longtoBEsyncsafe(rec->size);
	fwrite(&sz, 4, 1, file);
	bytes_written += 10;
	id3_frame* fr = rec->first;
	while (fr != 0) {
		fwrite(fr->id, 1, 4, file);
		sz = longtoBEsyncsafe(fr->size);
		fwrite(&sz, 1, 4, file);
		fwrite(fr->flags, 1, 2, file);
		fwrite(fr->value, 1, fr->size, file);
		bytes_written += 10 + fr->size;
		fr = fr->next;
	}
	// fill 00 fillers untill rec.size bytes written 
	while (bytes_written - 10 < rec->size) {
		fputc('\0', file);
		bytes_written++;
	}
	fclose(file);
	return 0;
}

void printTextFrameValue(id3_frame* fr) {
	int enc = fr->value[0];
	switch (enc) {
	case 0: for (int i = 1; i < fr->size; i++)
		putchar(fr->value[i]);
		break;
		/* TODO: add UTF-8 and UTF-16 encs */
	}

}

/* return >0 if ok and 0 if not found
   id3 - record with frames
   field - tag name
   returns -- string with value or null ptr*/
char* getFieldValue(struct id3_record* id3, const char* field) {
	id3_frame* fr = id3->first;
	while (fr != 0) {
		if (strcmp(fr->id, field) == 0) {
			printf(" %s: ", fr->id);
			if (fr->id[0] == 'T') {
				// text frame, first byte is encoding
				printTextFrameValue(fr);
			}
			else if (fr->id[0] == 'W') {
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
	id3_frame* fr = id3->first;
	while (fr != 0) {
		printf(" %s: ", fr->id);
		if (fr->id[0] == 'T') {
			// text frame, first byte is encoding
			printTextFrameValue(fr);
		}
		else if (fr->id[0] == 'W') {
			printf("%s", fr->value);
		}
		else if (fr->id[0] == 'C') {
			printf("%s", fr->value);
		}
		printf("\n");
		fr = fr->next;
	}
}

/* return >0 if ok and 0 if not found
   id3 - record with frames
   field - tag name
   value - new value
   returns -- 0 if success*/
int setFieldValue(struct id3_record* id3, const char* field, const char* value) {
	id3_frame* fr = id3->first;
	while (fr != 0) {
		if (strcmp(fr->id, field) == 0) break;
		fr = fr->next;
	}
	if (fr == 0) { /* add new frame */
		fr->next = (id3_frame*)malloc(sizeof(id3_frame));
		fr = fr->next;
		strncpy(fr->id, field, 4); /*TODO: add error handling*/
		fr->id[4] = '\0';
		fr->flags[2] = '1';
	}
	if (fr->value != 0) free(fr->value);
	if (fr->id[0] == 'T') {
		fr->value = (char*)malloc(strlen(value) + 1); // +1 for encoding, no '\0'
		fr->value[0] = 0; // ascii encoding
		memcpy(fr->value + 1, value, strlen(value));
		fr->size = strlen(value) + 1;
	}
	else {
		memcpy(fr->value, value, strlen(value) + 1);
		fr->size = strlen(value) + 1;
	}
	/* fix ID3 record size ???   not necessary, size is big enough*/
	/*
	unsigned int size = 0;
	fr = id3->first;
	while (fr != 0) {
		size += 10 + fr->size;
		fr = fr->next;
	}
	id3->size = size;*/
	return 0;
}


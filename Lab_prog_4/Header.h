#ifndef ID3PARSER_HEADER_H
#define ID3PARSER_HEADER_H

struct id3_frame {
	char id[5];
	unsigned long size;
	unsigned char flags[2];
	unsigned char encoding;
	char* value;
	struct id3_frame* next;
};

struct id3_record {
	unsigned char version[2];
	unsigned char flags;
	unsigned long size;
	struct id3_frame* first;
};

struct id3_record* init_empty_id3record();

void free_id3record(struct id3_record*);

int  fill_id3record(struct id3_record* dst, FILE* file);

void show_id3record(struct id3_record* id3);

char* getFieldValue(struct id3_record* id3, const char* field);
int setFieldValue(struct id3_record* id3, const char* field, const char* value);
int  write_id3record(struct id3_record* rec, FILE* file);

int do_show_command(char* filename);
int do_get_command(char* filename, char* fieldname);
int do_set_command(char* filename, char* fieldname, char* value);




#endif 


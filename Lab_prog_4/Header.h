#ifndef ID3PARSER_HEADER_H
#define ID3PARSER_HEADER_H

struct id3_frame {
	char id[5]; /* including trailing zero*/
	unsigned long size; /* 10 bytes + length of value*/
	unsigned char flags[2];
	unsigned char encoding; /* 00-03 ���� ���� �  FF ���� �� ������������*/
	char* value;
	struct id3_frame* next;
};

struct id3_record {
	/* header - total 10 bytes*/
	/* skip "ID3" label */
	/* version - 2 bytes 0x0200 or 0x0300 or 0x0400*/
	unsigned char version[2];
	/* flags - 1 byte*/
	unsigned char flags;
	/* size - 4 bytes*/
	unsigned long size;

	// pointer to frame
	struct id3_frame* first;
};

/* ������� � ������ ������ ������*/
struct id3_record* init_empty_id3record();

/* ������� �� ������ ������ ������*/
void free_id3record(struct id3_record*);

/* ��������� ������ �� �����*/
int  fill_id3record(struct id3_record* dst, FILE* file);

/* ������� �� �� �����*/
void show_id3record(struct id3_record* id3);

/* ����� ����*/
char* getFieldValue(struct id3_record* id3, const char* field);
int setFieldValue(struct id3_record* id3, const char* field, const char* value);
int  write_id3record(struct id3_record* rec, FILE* file);

/* ���������� ������ ���� */
int do_show_command(char* filename);
int do_get_command(char* filename, char* fieldname);
int do_set_command(char* filename, char* fieldname, char* value);




#endif // !ID3PARSER_HEADER_H


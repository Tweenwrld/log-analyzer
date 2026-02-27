#ifndef FILE_READER_H
#define FILE_READER_H
 
#include <stdio.h>
#include "parse.h"
#define BUFFER_SIZE 4096

typedef struct {
	FILE *file;
	char buffer[BUFFER_SIZE];
} FILEREADER;

FileReader* open_file(const char *filename);
char* read_line(FileReader *reader, char *line, size_t max_len);
void close_file(FileReader *reader);

#endif

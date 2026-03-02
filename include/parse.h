#ifndef PARSER_H
#define PARSER_H

#define MAX_MESSAGE_LEN 1024

#define LOG_LEVEL_INFO 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_UNKNOWN -1

typedef struct {
	char timestamp[20];
	int level;
	char message[MAX_MESSAGE_LEN];
}LogEntry;

int parse_log_line(const char *line, LogEntry *entry);


#endif

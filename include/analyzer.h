#ifndef ANALYZER_H
#define ANALYZER_H

#include "parse.h"

typedef struct {
	char message[MAX_MESSAGE_LEN];
	int count;
} LogEntryCount;

typedef LogEntryCount ErrorEntry;

typedef struct {
	int total_lines;
	int info_count;
	int warn_count;
	int error_count;
	int error_count_unique;
	int warn_count_unique;
	LogEntryCount *error_entries;
	LogEntryCount *warn_entries;
	int error_capacity;
	int warn_capacity;
} AnalysisResult;

AnalysisResult* init_analyzer();
void process_log_line(AnalysisResult *result, const LogEntry *entry);
void get_top_errors(const AnalysisResult *result, int top_n, ErrorEntry *top_errors);
void get_top_warnings(const AnalysisResult *result, int top_n, LogEntryCount *top_warnings);
void cleanup_analyzer(AnalysisResult *result);


#endif


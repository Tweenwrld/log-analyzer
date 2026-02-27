#ifndef ANALYZER_H
#define ANALYZER_H

#include "parse.h"
#define MAX_ERROR_MESSAGE 10000

typedef struct {
	char message[MAX_MESSAGE_LEN];
	int count;
} ErrorEntry;

typedef struct {
	int total_line; // Processed 1,234,434 lines;
	int info_count;
	int warn_count;
	int error_count;
	ErrorEntry *error_entries;
	int error_count_unique; // Disk fulll* appears 500 times -> unique count = 1
	int error_capacity;
} AnalysisResult;

AnalysisResult* init_analyzer(); // AnalysisResult *result = init_analyzer();
void process_log_lines(const AnalysisResult *result, int top_n, ErrorEntry *top_errors);
void cleanup_analyzer(AnalysisResult *result);


#endif


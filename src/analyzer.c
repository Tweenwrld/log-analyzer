#include "analyzer.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AnalysisResult* init_analyzer(){
	AnalysisResult *result = malloc(sizeof(AnalysisResult));
	if(!result) return NULL;

	result->total_lines = 0;
	result->info_count = 0;
	result->error_count = 0;
	result->warn_count = 0;
	result->error_count_unique = 0;
	result->warn_count_unique = 0;
	result->error_capacity = 100;
	result->warn_capacity = 100;

	result->error_entries = calloc(result->error_capacity, sizeof(LogEntryCount));
	if(!result->error_entries){
		free(result);
		return NULL;
	}

	result->warn_entries = calloc(result->warn_capacity, sizeof(LogEntryCount));
	if(!result->warn_entries){
		free(result->error_entries);
		free(result);
		return NULL;
	}

	return result;
}

static void add_message(LogEntryCount **entries, int *unique_count, int *capacity, const char *message){
	for (int i = 0; i < *unique_count; i++){
		if(strcmp((*entries)[i].message, message) == 0){
			(*entries)[i].count++;
			return;
		}
	}

	if(*unique_count >= *capacity){
		int new_capacity = (*capacity) * 2;
		LogEntryCount * new_entries = realloc(*entries, new_capacity * sizeof(LogEntryCount));
		if(!new_entries) return;

		*entries = new_entries;
		*capacity = new_capacity;
	}

	snprintf((*entries)[*unique_count].message, MAX_MESSAGE_LEN, "%s", message);

	(*entries)[*unique_count].count = 1;
	(*unique_count)++;
}

static void add_error_message(AnalysisResult *result, const char *message){
	add_message(&result->error_entries, &result->error_count_unique, &result->error_capacity, message);
}

static void add_warn_message(AnalysisResult *result, const char *message){
	add_message(&result->warn_entries, &result->warn_count_unique, &result->warn_capacity, message);
}

void process_log_line(AnalysisResult *result, const LogEntry *entry){
	if(!result || !entry) return;

	switch(entry->level){
		case LOG_LEVEL_INFO:
			result->info_count++;
			break;
		case LOG_LEVEL_WARN:
			result->warn_count++;
			add_warn_message(result, entry->message);
			break;
		case LOG_LEVEL_ERROR:
			result->error_count++;
			add_error_message(result, entry->message);
			break;
	}
}

static void get_top_entries(const LogEntryCount *entries, int unique_count, int top_n, LogEntryCount *top_entries){
	if (!entries || !top_entries || unique_count <= 0 || top_n <= 0) return;

	int n = unique_count < top_n ? unique_count : top_n;
	LogEntryCount *temp = malloc(unique_count * sizeof(LogEntryCount));
	if (!temp) return;

	memcpy(temp, entries, unique_count * sizeof(LogEntryCount));

	for(int i = 0; i < n; i++) {
		int max_idx = i;
		for (int j = i + 1; j < unique_count; j ++) {
			if (temp[j].count > temp[max_idx].count){
				max_idx = j;
			}
		}

		LogEntryCount swap = temp[i];
		temp[i] = temp[max_idx];
		temp[max_idx] = swap;
	}

	memcpy(top_entries, temp, n * sizeof(LogEntryCount));
	free(temp);
}


void get_top_errors(const AnalysisResult *result, int top_n, ErrorEntry *top_errors){
	if (!result || !top_errors) return;
	get_top_entries(result->error_entries, result->error_count_unique, top_n, top_errors);
}

void get_top_warnings(const AnalysisResult *result, int top_n, LogEntryCount *top_warnings){
	if (!result || !top_warnings) return;
	get_top_entries(result->warn_entries, result->warn_count_unique, top_n, top_warnings);
}

void cleanup_analyzer(AnalysisResult *result){
	if(result) {
		if(result->error_entries){
			free(result->error_entries);
		}
		if(result->warn_entries){
			free(result->warn_entries);
		}
		free(result);
	}
}

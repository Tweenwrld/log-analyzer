#include "report.h"
#include "analyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	const char *pattern;
	const char *interpretation;
} KnowledgeEntry;

static const KnowledgeEntry knowledge_map[] = {
	{ "Database", "Frequent database errors suggest connectivity or configuration problems." },
	{ "Timeout", "Timeout errors indicate possible network latency or overloaded services." },
	{ "Disk space", "Low disk space warnings suggest storage capacity issues." },
	{ "Memory", "High memory usage warnings may indicate leaks or insufficient resources." },
	{ "Authentication", "Authentication failures suggest invalid credentials or security misconfiguration." },
	{ "Connection refused", "Connection refused errors point to service availability or firewall issues." }
};

static const int knowledge_map_size = sizeof(knowledge_map) / sizeof(knowledge_map[0]);

void print_summary(const AnalysisResult *result, int top_n){
	if (!result) return;
	if (top_n <= 0) top_n = 10;

	LogEntryCount *top_errors = NULL;
	LogEntryCount *top_warnings = NULL;

	if (result->error_count_unique > 0) {
		top_errors = malloc(top_n * sizeof(LogEntryCount));
		if (top_errors) {
			get_top_errors(result, top_n, top_errors);
		}
	}

	if (result->warn_count_unique > 0) {
		top_warnings = malloc(top_n * sizeof(LogEntryCount));
		if (top_warnings) {
			get_top_warnings(result, top_n, top_warnings);
		}
	}

	printf("Log Summary\n");
	printf("------------------\n");
	printf("Total lines: %d\n", result->total_lines);
	printf("INFO: %d\n", result->info_count);
	printf("WARN: %d\n", result->warn_count);
	printf("ERROR: %d\n\n", result->error_count);

	printf("Top %d Errors:\n", top_n);
	for (int i = 0; i < top_n && i < result->error_count_unique; i++) {
		if (!top_errors) break;
		printf("%d. %s (%d occurrences)\n",
		       i + 1,
		       top_errors[i].message,
		       top_errors[i].count);
	}

	printf("\nTop %d Warnings:\n", top_n);
	for (int i = 0; i < top_n && i < result->warn_count_unique; i++) {
		if (!top_warnings) break;
		printf("%d. %s (%d occurrences)\n",
		       i + 1,
		       top_warnings[i].message,
		       top_warnings[i].count);
	}

	printf("\nInterpretation:\n");
	if (result->error_count > result->warn_count && result->error_count > result->info_count) {
		printf("Errors dominate, suggesting system instability.\n");
	} else if (result->warn_count > result->error_count) {
		printf("Warnings dominate, indicating performance or resource concerns.\n");
	} else {
		printf("INFO messages dominate, system appears mostly healthy.\n");
	}

	for (int i = 0; i < result->error_count_unique; i++) {
		for (int j = 0; j < knowledge_map_size; j++) {
			if (strstr(result->error_entries[i].message, knowledge_map[j].pattern)) {
				printf("- %s (%d occurrences): %s\n",
				       result->error_entries[i].message,
				       result->error_entries[i].count,
				       knowledge_map[j].interpretation);
				break;
			}
		}
	}

	for (int i = 0; i < result->warn_count_unique; i++) {
		for (int j = 0; j < knowledge_map_size; j++) {
			if (strstr(result->warn_entries[i].message, knowledge_map[j].pattern)) {
				printf("- %s (%d occurrences): %s\n",
				       result->warn_entries[i].message,
				       result->warn_entries[i].count,
				       knowledge_map[j].interpretation);
				break;
			}
		}
	}

	free(top_errors);
	free(top_warnings);
}


void print_top_errors(const AnalysisResult * result, int top_n){
	if (!result || top_n <= 0) return;

	if (result->error_count_unique == 0) {
		printf("\nNo errors found.\n");
		return;
	}

	ErrorEntry * top_errors = malloc(top_n * sizeof(ErrorEntry));
	if (!top_errors) return;

	get_top_errors(result, top_n, top_errors);

	printf("\nTop %d Errors:\n", top_n);
	printf("-------------\n");

	for (int i = 0; i < top_n && i < result->error_count_unique; i++) {
		printf("%d. %s (%d occurrences)\n", i + 1, top_errors[i].message, top_errors[i].count);
	}

	free(top_errors);
}

void print_top_warnings(const AnalysisResult *result, int top_n){
	if (!result || top_n <= 0) return;

	if (result->warn_count_unique == 0) {
		printf("\nNo warnings found.\n");
		return;
	}

	LogEntryCount *top_warnings = malloc(top_n * sizeof(LogEntryCount));
	if (!top_warnings) return;

	get_top_warnings(result, top_n, top_warnings);

	printf("\nTop %d Warnings:\n", top_n);
	printf("---------------\n");

	for (int i = 0; i < top_n && i < result->warn_count_unique; i++) {
		printf("%d. %s (%d occurrences)\n", i + 1, top_warnings[i].message, top_warnings[i].count);
	}

	free(top_warnings);
}

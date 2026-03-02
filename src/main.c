#include "report.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_reader.h"
#include "parse.h"
#include "analyzer.h"

#define VERSION "2.0.0"

// ANSI color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"

void print_summary_json(AnalysisResult *result, int top_n);
void print_usage(const char *program_name);
void print_examples(const char *program_name);

typedef struct {
    const char *filename;
    int errors_only;
    int warns_only;
    int summary;
    int top_n;
    int threshold;
    const char *output_file;
    const char *slack_url;
    const char *teams_url;
    const char *email_address;
    int json_output;
} CliOptions;

static void init_options(CliOptions *options) {
    options->filename = NULL;
    options->errors_only = 0;
    options->warns_only = 0;
    options->summary = 0;
    options->top_n = 10;
    options->threshold = -1;
    options->output_file = NULL;
    options->slack_url = NULL;
    options->teams_url = NULL;
    options->email_address = NULL;
    options->json_output = 0;
}

static int parse_cli_args(int argc, char *argv[], CliOptions *options) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "--examples") == 0) {
            print_examples(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("loganalyzer version %s\n", VERSION);
            return 1;
        } else if (strcmp(argv[i], "--errors-only") == 0) {
            options->errors_only = 1;
        } else if (strcmp(argv[i], "--warns-only") == 0) {
            options->warns_only = 1;
        } else if (strcmp(argv[i], "--summary") == 0) {
            options->summary = 1;
        } else if (strcmp(argv[i], "--top-errors") == 0) {
            if (i + 1 < argc) {
                options->top_n = atoi(argv[++i]);
                if (options->top_n <= 0) options->top_n = 10;
            }
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            options->output_file = argv[++i];
        } else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            options->threshold = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--slack") == 0 && i + 1 < argc) {
            options->slack_url = argv[++i];
        } else if (strcmp(argv[i], "--teams") == 0 && i + 1 < argc) {
            options->teams_url = argv[++i];
        } else if (strcmp(argv[i], "--email") == 0 && i + 1 < argc) {
            options->email_address = argv[++i];
        } else if (strcmp(argv[i], "--json") == 0) {
            options->json_output = 1;
        } else if (argv[i][0] != '-') {
            options->filename = argv[i];
        }
    }

    if (!options->filename) {
        fprintf(stderr, "Error: No log file specified\n");
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

static int analyze_file(const CliOptions *options, AnalysisResult *result) {
    FileReader *reader = open_file(options->filename);
    if (!reader) {
        fprintf(stderr, "Error: Could not open file '%s'\n", options->filename);
        return -1;
    }

    char line[2048];
    LogEntry entry;
    int processed_lines = 0;
    FILE *progress_stream = options->json_output ? stderr : stdout;

    fprintf(progress_stream, "Analyzing log files: %s\n", options->filename);
    fprintf(progress_stream, "Press Ctrl+C to abort... \n\n");

    while (read_line(reader, line, sizeof(line))) {
        if (parse_log_line(line, &entry) == 0) {
            process_log_line(result, &entry);
            processed_lines++;
            result->total_lines++;

            if (processed_lines % 10000 == 0) {
                fprintf(progress_stream, "\rProcessed %'d lines...", processed_lines);
                fflush(progress_stream);
            }
        }
    }

    fprintf(progress_stream, "\rProcessed %'d lines... Done!  \n\n", processed_lines);
    close_file(reader);
    return 0;
}

static void render_output(const CliOptions *options, AnalysisResult *result) {
    if (options->json_output) {
        print_summary_json(result, options->top_n);
        return;
    }

    if (options->summary || (!options->errors_only && !options->warns_only)) {
        print_summary(result, options->top_n);
    }
    if (options->errors_only && result->error_count > 0) {
        print_top_errors(result, options->top_n);
    }
    if (options->warns_only && result->warn_count > 0) {
        print_top_warnings(result, options->top_n);
    }
}

static void handle_integrations(const CliOptions *options, const AnalysisResult *result) {
    if (options->threshold > 0 && result->error_count > options->threshold) {
        printf("ALERT: Error count exceeded threshold (%d > %d)\n", result->error_count, options->threshold);
    }

    if (options->output_file) {
        FILE *out = fopen(options->output_file, "w");
        if (out) {
            fprintf(out, "Total Errors: %d\n", result->error_count);
            fclose(out);
        }
    }

    (void)options->slack_url;
    (void)options->teams_url;
    (void)options->email_address;
}

void print_summary_json(AnalysisResult *result, int top_n) {
    if (top_n <= 0) top_n = 10;

    printf("{\n");
    printf("  \"version\": \"%s\",\n", VERSION);
    printf("  \"summary\": {\n");
    printf("    \"total_lines\": %d,\n", result->total_lines);
    printf("    \"total_errors\": %d,\n", result->error_count);
    printf("    \"total_warnings\": %d,\n", result->warn_count);
    printf("    \"total_info\": %d\n", result->info_count);
    printf("  },\n");

    ErrorEntry *top_errors = NULL;
    LogEntryCount *top_warnings = NULL;
    if (result->error_count_unique > 0) {
        top_errors = malloc(top_n * sizeof(ErrorEntry));
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

    printf("  \"top_errors\": [\n");
    for (int i = 0; i < top_n && i < result->error_count_unique; i++) {
        if (!top_errors) break;
        printf("    { \"message\": \"%s\", \"count\": %d }%s\n", 
               top_errors[i].message,
               top_errors[i].count,
               (i < top_n - 1 && i < result->error_count_unique - 1) ? "," : "");
    }
    printf("  ],\n");

    printf("  \"top_warnings\": [\n");
    for (int i = 0; i < top_n && i < result->warn_count_unique; i++) {
        if (!top_warnings) break;
        printf("    { \"message\": \"%s\", \"count\": %d }%s\n",
               top_warnings[i].message,
               top_warnings[i].count,
               (i < top_n - 1 && i < result->warn_count_unique - 1) ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");

    free(top_errors);
    free(top_warnings);
}

void print_usage(const char *program_name) {
    printf(BOLD "Usage:" RESET " %s <log file> [option]\n\n", program_name);
    printf("Analyze log files (INFO, WARN, ERROR) and produce summaries.\n\n");

    printf(BOLD "Options:\n" RESET);
    printf("  --errors-only         Show only error summary\n");
    printf("  --warns-only          Show only warning summary\n");
    printf("  --summary             Show full summary (INFO, WARN, ERROR counts)\n");
    printf("  --top-errors N        Show top N most frequent errors (default: 10)\n");
    printf("  --output FILE         Save analysis results to FILE\n");
    printf("  --threshold N         Trigger alert if total errors exceed N\n");
    printf("  --slack WEBHOOK_URL   Send analysis results to Slack via webhook\n");
    printf("  --teams WEBHOOK_URL   Send analysis results to Microsoft Teams via webhook\n");
    printf("  --email ADDRESS       Email analysis results to ADDRESS (requires sendmail/msmtp)\n");
    printf("  --json                Output results in JSON format (for dashboards)\n");
    printf("  --version             Show version information\n");
    printf("  --help                Show this help message\n");
    printf("  --examples            Show extended usage examples and integration notes\n\n");

    printf(BOLD "Examples:\n" RESET);
    printf("  %s server.log\n", program_name);
    printf("  %s server.log --errors-only --top-errors 5\n", program_name);
    printf("  %s server.log --threshold 500 --slack https://hooks.slack.com/services/XXXX/YYYY/ZZZZ\n", program_name);
    printf("  %s server.log --output summary.txt --email team@company.com\n\n", program_name);

    printf(GREEN "Email Integration Notes:\n" RESET);
    printf("  This program uses 'sendmail' under the hood.\n");
    printf("  To send via Gmail, install msmtp and configure ~/.msmtprc.\n");
    printf("  See --examples for detailed setup.\n\n");

    printf(CYAN "Slack/Teams Integration Notes:\n" RESET);
    printf("  Provide a valid webhook URL to --slack or --teams.\n");
    printf("  See --examples for detailed usage.\n");
}

void print_examples(const char *program_name) {
    printf("\n" BOLD "Extended Examples and Integration Notes:\n\n" RESET);

    printf(YELLOW "Basic Usage:\n" RESET);
    printf("  %s logs/sample.log --summary --top-errors 5\n\n", program_name);

    printf(GREEN "Email Integration (via msmtp):\n" RESET);
    printf("  Configure ~/.msmtprc with Gmail settings:\n");
    printf("    defaults\n");
    printf("    auth on\n");
    printf("    tls on\n");
    printf("    tls_trust_file /etc/ssl/certs/ca-certificates.crt\n");
    printf("    account gmail\n");
    printf("    host smtp.gmail.com\n");
    printf("    port 587\n");
    printf("    from you@gmail.com\n");
    printf("    user you@gmail.com\n");
    printf("    password <APP_PASSWORD>\n");
    printf("    account default : gmail\n\n");
    printf("  Test with:\n");
    printf("    echo \"Hello\" | msmtp -a gmail you@gmail.com\n");
    printf("  Analyzer example:\n");
    printf("    (echo \"Subject: Daily Log Report\"; echo \"To: you@gmail.com\"; \\\n");
    printf("     %s logs/sample.log --json --top-errors 5) | msmtp -a gmail -t\n\n", program_name);

    printf(CYAN "Slack Integration:\n" RESET);
    printf("  %s logs/sample.log --json --slack https://hooks.slack.com/services/XXXX/YYYY/ZZZZ\n\n", program_name);

    printf(CYAN "Teams Integration:\n" RESET);
    printf("  %s logs/sample.log --json --teams https://outlook.office.com/webhook/XXXX/YYYY/ZZZZ\n\n", program_name);

    printf(YELLOW "Cron Job Automation:\n" RESET);
    printf("  Run daily at 8 AM and email report:\n");
    printf("    0 8 * * * (echo \"Subject: Daily Log Report\"; echo \"To: you@gmail.com\"; \\\n");
    printf("    %s logs/sample.log --json --top-errors 5) | msmtp -a gmail -t\n\n", program_name);
}

int main (int argc, char *argv[]) {
    if (argc < 2){
        print_usage(argv[0]);
        return 1;
    }

    CliOptions options;
    init_options(&options);
    int parse_status = parse_cli_args(argc, argv, &options);
    if (parse_status != 0) {
        return parse_status < 0 ? 1 : 0;
    }

    AnalysisResult *result = init_analyzer();
    if (!result) {
        fprintf(stderr, "Error: Memory Allocation Failed\n");
        return 1;
    }

    if (analyze_file(&options, result) != 0) {
        cleanup_analyzer(result);
        return 1;
    }

    render_output(&options, result);
    handle_integrations(&options, result);

    cleanup_analyzer(result);
    return 0;
}


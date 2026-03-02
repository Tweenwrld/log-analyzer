#!/bin/bash
set -euo pipefail

ANALYZER="./loganalyzer.exe"
LOG_DIR="logs"

usage() {
    cat << 'EOF'
Usage:
  ./automation.sh batch-json [log_dir]
  ./automation.sh email-json <log_file> <recipient_email> [top_n]
  ./automation.sh email-html <log_file> <recipient_email> [top_n]

Commands:
  batch-json   Generate *_output.json and *_progress.log for all .log files
  email-json   Send JSON report via msmtp
  email-html   Send HTML formatted report via msmtp

Parameters:
  <recipient_email>   Real email address (e.g., alice@company.com)
                      NOT a placeholder - actual recipient who will receive the report
  [top_n]             Number of top errors/warnings to include (default: 10)

Examples:
  ./automation.sh email-html logs/sample.log alice@gmail.com
  ./automation.sh email-html logs/sample.log team@company.com 20
  ./automation.sh email-json logs/app.log ops@acme.com 15
EOF
}

require_analyzer() {
    if [ ! -x "$ANALYZER" ]; then
        echo "Error: $ANALYZER not found or not executable. Run: make"
        exit 1
    fi
}

extract_json_value() {
    local key="$1"
    local json="$2"
    echo "$json" | grep -o "\"$key\": [0-9]*" | grep -o '[0-9]*' || echo "0"
}

run_batch_json() {
    local dir="${1:-$LOG_DIR}"

    if [ ! -d "$dir" ]; then
        echo "Error: Directory '$dir' not found"
        exit 1
    fi

    require_analyzer
    echo "Generating JSON reports for all log files in $dir..."

    local found=0
    for logfile in "$dir"/*.log; do
        if [ -f "$logfile" ]; then
            found=1
            if [[ "$logfile" =~ _progress\.log$ ]]; then
                continue
            fi

            local filename
            filename=$(basename "$logfile" .log)
            local output_json="$dir/${filename}_output.json"
            local progress_log="$dir/${filename}_progress.log"

            echo "  Processing: $logfile -> $output_json"
            "$ANALYZER" "$logfile" --json > "$output_json" 2> "$progress_log"
        fi
    done

    if [ "$found" -eq 0 ]; then
        echo "No .log files found in $dir"
        return
    fi

    echo "Done. Generated reports in $dir"
}

send_email_json() {
    local log_file="$1"
    local recipient="$2"
    local top_n="${3:-10}"

    if [ ! -f "$log_file" ]; then
        echo "Error: Log file '$log_file' not found"
        exit 1
    fi

    require_analyzer

    local subject="Log Analysis Report - $(basename "$log_file") - $(date +%Y-%m-%d)"

    (
        echo "Subject: $subject"
        echo "To: $recipient"
        echo "Content-Type: application/json"
        echo ""
        "$ANALYZER" "$log_file" --json --top-errors "$top_n" 2>/dev/null
    ) | msmtp -a gmail -t

    echo "Email sent to $recipient"
}

send_email_html() {
    local log_file="$1"
    local recipient="$2"
    local top_n="${3:-10}"

    if [ ! -f "$log_file" ]; then
        echo "Error: Log file '$log_file' not found"
        exit 1
    fi

    require_analyzer

    local subject="Log Analysis Report - $(basename "$log_file") - $(date +%Y-%m-%d)"
    local json_output
    json_output=$("$ANALYZER" "$log_file" --json --top-errors "$top_n" 2>/dev/null)

    local total_lines total_errors total_warnings total_info
    total_lines=$(extract_json_value "total_lines" "$json_output")
    total_errors=$(extract_json_value "total_errors" "$json_output")
    total_warnings=$(extract_json_value "total_warnings" "$json_output")
    total_info=$(extract_json_value "total_info" "$json_output")

    (
        echo "Subject: $subject"
        echo "To: $recipient"
        echo "Content-Type: text/html; charset=UTF-8"
        echo ""
        cat << EOF
<!DOCTYPE html>
<html>
<head>
  <style>
    body { font-family: Arial, sans-serif; line-height: 1.6; color: #333; }
    .container { max-width: 800px; margin: 0 auto; padding: 20px; }
    .header { background: #1f2937; color: white; padding: 20px; border-radius: 8px; }
    .summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 12px; margin: 20px 0; }
    .card { background: #f3f4f6; padding: 14px; border-radius: 6px; border-left: 4px solid #4b5563; }
    .value { font-size: 28px; font-weight: bold; }
    .label { font-size: 12px; color: #6b7280; text-transform: uppercase; }
    pre { background: #f9fafb; padding: 12px; border-radius: 6px; overflow-x: auto; }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h2>Log Analysis Report</h2>
      <p>File: <strong>$(basename "$log_file")</strong></p>
      <p>Generated: $(date '+%Y-%m-%d %H:%M:%S')</p>
    </div>

    <div class="summary">
      <div class="card"><div class="value">$total_lines</div><div class="label">Total Lines</div></div>
      <div class="card"><div class="value">$total_errors</div><div class="label">Errors</div></div>
      <div class="card"><div class="value">$total_warnings</div><div class="label">Warnings</div></div>
      <div class="card"><div class="value">$total_info</div><div class="label">Info</div></div>
    </div>

    <h3>JSON Details</h3>
    <pre>$json_output</pre>
  </div>
</body>
</html>
EOF
    ) | msmtp -a gmail -t

    echo "HTML email sent to $recipient"
}

main() {
    if [ $# -lt 1 ]; then
        usage
        exit 1
    fi

    case "$1" in
        batch-json)
            run_batch_json "${2:-$LOG_DIR}"
            ;;
        email-json)
            if [ $# -lt 3 ]; then
                usage
                exit 1
            fi
            send_email_json "$2" "$3" "${4:-10}"
            ;;
        email-html)
            if [ $# -lt 3 ]; then
                usage
                exit 1
            fi
            send_email_html "$2" "$3" "${4:-10}"
            ;;
        -h|--help|help)
            usage
            ;;
        *)
            echo "Unknown command: $1"
            usage
            exit 1
            ;;
    esac
}

main "$@"

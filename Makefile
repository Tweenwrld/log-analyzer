CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -I./include
TARGET = loganalyzer.exe
SRCDIR = src
OBJDIR = obj
INCDIR = include
LOGDIR = logs

# Configurable parameters for email/automation targets
EMAIL ?= team@company.com
TOP ?= 10
LOG ?= $(LOGDIR)/sample.log

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(LOGDIR))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run analyzer on sample log
run: $(TARGET)
	./$(TARGET) $(LOGDIR)/sample.log

# Generate large test file and analyze
test-large: $(TARGET)
	@echo "Generating large test file..."
	@python3 -c "for i in range(1000000): \
		print(f'2026-01-01 12:00:00 INFO Normal operation line {i}'); \
		print(f'2026-01-01 12:00:00 WARN Low disk space'); \
		print(f'2026-01-01 12:00:00 ERROR Database connection failed') if i % 1000 == 0 else None" \
		> $(LOGDIR)/large_test.log
	./$(TARGET) $(LOGDIR)/large_test.log --json --top-errors 3 > $(LOGDIR)/large_summary.json

# Memory leak check
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) $(LOGDIR)/sample.log

# Generate daily summary (JSON format)
daily-summary: $(TARGET)
	@echo "Generating daily summary (JSON)..."
	@./$(TARGET) $(LOGDIR)/sample.log --json --errors-only --top-errors 5 > $(LOGDIR)/daily_summary.json

# Batch JSON automation via unified script
# Usage: make automate-batch [LOGDIR=logs]
automate-batch: $(TARGET)
	@./automation.sh batch-json $(LOGDIR)

# Send HTML email report via unified script
# Usage: make email-html [EMAIL=user@example.com] [TOP=10] [LOG=logs/sample.log]
email-html: $(TARGET)
	@./automation.sh email-html $(LOG) $(EMAIL) $(TOP)

# Email report (requires msmtp configured)
# Usage: make email-report [EMAIL=user@example.com] [TOP=10] [LOG=logs/sample.log]
email-report: daily-summary
	@echo "Sending email report via automation script..."
	@./automation.sh email-json $(LOG) $(EMAIL) $(TOP)

# Slack alert (requires webhook URL)
slack-alert: daily-summary
	@echo "Sending Slack alert..."
	@curl -X POST -H 'Content-type: application/json' \
		--data "@$(LOGDIR)/daily_summary.json" \
		https://hooks.slack.com/services/XXXX/YYYY/ZZZZ

# Teams alert (requires webhook URL)
teams-alert: daily-summary
	@echo "Sending Teams alert..."
	@curl -H "Content-Type: application/json" \
		-d "@$(LOGDIR)/daily_summary.json" \
		https://outlook.office.com/webhook/XXXX/YYYY/ZZZZ

# Threshold alert (Slack example, JSON payload)
threshold-alert: daily-summary
	@echo "Checking thresholds..."
	@TOTAL_ERRORS=$(jq '.error_count' $(LOGDIR)/daily_summary.json); \
	if [ "$$TOTAL_ERRORS" -gt 500 ]; then \
		curl -X POST -H 'Content-type: application/json' \
		--data "{\"text\":\"🚨 URGENT ALERT: High Error Volume ($$TOTAL_ERRORS)\"}" \
		https://hooks.slack.com/services/XXXX/YYYY/ZZZZ; \
	else \
		curl -X POST -H 'Content-type: application/json' \
		--data "{\"text\":\"✅ Daily Log Report OK ($$TOTAL_ERRORS errors)\"}" \
		https://hooks.slack.com/services/XXXX/YYYY/ZZZZ; \
	fi

# Dashboard export (Grafana/ELK ingestion)
dashboard-export: daily-summary
	@echo "Exporting summary for dashboards..."
	@cp $(LOGDIR)/daily_summary.json /var/monitoring/logs_summary.json

clean:
	rm -f $(OBJDIR)/*.o $(TARGET) $(LOGDIR)/*.txt $(LOGDIR)/*.log $(LOGDIR)/*.json

.PHONY: all clean run test-large valgrind daily-summary automate-batch email-html email-report slack-alert teams-alert threshold-alert dashboard-export

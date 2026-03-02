import random
from datetime import datetime, timedelta

TOTAL_LINES = 10000
START_TIME = datetime(2025, 9, 10, 12, 0, 0)

# Occupation-specific messages
info_messages = [
    # Healthcare
    "Patient check-in completed",
    "Vitals recorded successfully",
    "Lab results uploaded",
    "Prescription issued",
    # Finance
    "Transaction approved",
    "Account balance updated",
    "Loan application processed",
    "Invoice generated",
    # Education
    "Student login successful",
    "Assignment submitted",
    "Lecture recording uploaded",
    "Exam results published",
    # IT/Operations
    "Application running",
    "Cache hit",
    "Background job completed",
    "Worker initialized"
]

warn_messages = [
    # Healthcare
    "High blood pressure detected",
    "Delayed lab result delivery",
    # Finance
    "Unusual transaction pattern detected",
    "Slow response from payment gateway",
    # Education
    "Assignment submission deadline approaching",
    "Low attendance warning",
    # IT/Operations
    "High memory usage detected",
    "Low disk space on /var",
    "Configuration value missing, using default"
]

error_messages = [
    # Healthcare
    "Failed to retrieve patient record",
    "Medical device connection lost",
    # Finance
    "Database connection failed",
    "Timeout while processing transaction",
    "Authentication service unavailable",
    # Education
    "Exam grading system crashed",
    "Failed to upload lecture recording",
    # IT/Operations
    "Connection reset by peer",
    "Failed to write to disk"
]

current_time = START_TIME
with open("logs/sample.log", "w", encoding="utf-8") as f:
    for i in range(TOTAL_LINES):
        r = random.random()
        if r < 0.70:
            level = "INFO"
            msg = random.choice(info_messages)
        elif r < 0.85:
            level = "WARN"
            msg = random.choice(warn_messages)
        else:
            level = "ERROR"
            msg = random.choice(error_messages)
            # Simulate retries: errors repeat more often
            if random.random() < 0.6:
                msg = random.choice(error_messages)

        f.write(f"{current_time:%Y-%m-%d %H:%M:%S} {level} {msg}\n")
        current_time += timedelta(seconds=random.randint(0, 2))

print(f"Generated {TOTAL_LINES} occupation-based log lines -> logs/sample.log")

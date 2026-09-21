#!/bin/bash
# Wrapper that logs all UCI input and output
ENGINE="/Users/bekarysshaimardan/Desktop/chess-engine/build/chess_engine"
LOG_IN="/Users/bekarysshaimardan/Desktop/chess-engine/uci_in.log"
LOG_OUT="/Users/bekarysshaimardan/Desktop/chess-engine/uci_out.log"

echo "=== SESSION $(date) ===" >> "$LOG_IN"
echo "=== SESSION $(date) ===" >> "$LOG_OUT"

# Use named pipes to intercept
$ENGINE 2>>"$LOG_OUT" | while IFS= read -r line; do
    echo "$line" >> "$LOG_OUT"
    echo "$line"
done

# But we also need to log input. Use a different approach:
# We can't easily log both directions with a simple pipe.
# Let's use script or a different approach.

# Actually, let's use a coproc approach

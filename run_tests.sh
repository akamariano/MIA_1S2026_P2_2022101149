#!/bin/bash
# Test runner: sends each command from a .smia file to the backend API

BACKEND="${1:-http://13.218.107.126:8080}"
SCRIPT="${2:-Archivo_De_Prueba_P2.smia}"
PASS=0
FAIL=0
TOTAL=0

echo "======================================================"
echo " ExtreamFS Test Runner"
echo " Backend: $BACKEND"
echo " Script:  $SCRIPT"
echo "======================================================"
echo ""

while IFS= read -r line; do
    # Skip blank lines and comments
    trimmed="${line#"${line%%[![:space:]]*}"}"
    [[ -z "$trimmed" || "$trimmed" == \#* ]] && continue

    TOTAL=$((TOTAL + 1))
    echo ">>> $trimmed"

    response=$(curl -s -X POST "$BACKEND/command" \
        -H "Content-Type: application/json" \
        -d "{\"command\": $(echo "$trimmed" | python3 -c 'import sys,json; print(json.dumps(sys.stdin.read().strip()))')}" \
        --connect-timeout 15 --max-time 30 2>&1)

    output=$(echo "$response" | python3 -c 'import sys,json
try:
    d=json.load(sys.stdin)
    print(d.get("output","") or d.get("error","(empty)"))
except:
    print(sys.stdin.read()[:300])
' 2>/dev/null || echo "$response")

    echo "$output"

    # Heuristic: flag lines with ERROR as expected errors vs real failures
    if echo "$output" | grep -qi "Error\|failed\|no existe\|not found\|cannot\|invalid" 2>/dev/null; then
        if echo "$trimmed" | grep -qi "ERROR" 2>/dev/null || [[ "$TOTAL" -eq 0 ]]; then
            : # expected error line
        fi
    fi

    echo "------------------------------------------------------"
done < "$SCRIPT"

echo ""
echo "======================================================"
echo " Done. Total commands sent: $TOTAL"
echo "======================================================"

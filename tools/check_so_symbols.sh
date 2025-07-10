#!/usr/bin/env bash
set -e

LIBRARY="$1"
EXPECTED_FILE="$2" # ./expected_libmoocore_exports.txt"

# Ensure the library and expected file exist
if [ ! -f "$LIBRARY" ]; then
    echo "❌ Shared library not found: $LIBRARY"
    exit 1
fi

if [ ! -f "$EXPECTED_FILE" ]; then
    echo "❌ Expected symbol list not found: $EXPECTED_FILE"
    exit 1
fi

OSTYPE="${OSTYPE:-$(uname -s)}"
case "$OSTYPE" in
    [Dd]arwin*)
        echo "[macOS] Extracting symbols..."
        EXPORTED=$(nm -P -g --defined-only "$LIBRARY" | tr -s ' ' | sed 's/^_//' | grep ' T ' | cut -d' ' -f1 | sort)
        # Compare both lists
        if ! diff -q <(echo "$EXPORTED") <(sort "$EXPECTED_FILE") &> /dev/null; then
            echo "❌ Exported symbols do not match expected list:"
            DIFF=$(diff -u <(echo "$EXPORTED") <(sort "$EXPECTED_FILE"))
            echo "$DIFF"
            exit 1
        fi
        ;;
    [Ll]inux*)
        echo "[Linux] Extracting symbols..."
        EXPORTED=$(nm -P -g --defined-only "$LIBRARY" | tr -s ' ' | grep ' T ' | cut -d' ' -f1 | sort)
        # Compare both lists
        if ! diff -q <(echo "$EXPORTED") <(sort "$EXPECTED_FILE") &> /dev/null; then
            echo "❌ Exported symbols do not match expected list:"
            DIFF=$(diff -u <(echo "$EXPORTED") <(sort "$EXPECTED_FILE"))
            echo "$DIFF"
            exit 1
        fi
        ;;
    [Ww]indows*|msys*|cygwin*)
        if command -v dumpbin &>/dev/null; then
            echo "[Windows MSVC] Extracting symbols..."
            TMP_ACTUAL=$(mktemp)
            dumpbin /EXPORTS "$LIBRARY" |
                while read -r line; do
                    if [[ "$line" =~ ^[[:space:]]*[0-9]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+(.+) ]]; then
                        symbol="${BASH_REMATCH[1]}"
                        undname "$symbol"
                    fi
                done | sort > "$TMP_ACTUAL"
            EXPORTED=$(cat $TMP_ACTUAL)
            rm -f $TMP_ACTUAL
        elif command -v nm &>/dev/null; then
            echo "[Windows MinGW] Extracting symbols..."
            EXPORTED=$(nm -P -g --defined-only "$LIBRARY" | tr -s ' ' | sed 's/^_//' | grep ' T ' | cut -d' ' -f1 | sort)
            echo Exported: "$EXPORTED"
        else
            echo "No supported symbol tool found on Windows."
            exit 1
        fi
        MISSING=$(comm -23  <(sort "$EXPECTED_FILE") <(echo "$EXPORTED"))
        # Compare both lists
        if [ -n "$MISSING" ]; then
            echo "❌ Exported symbols do not match expected list:"
            echo $MISSING
            exit 1
        fi
        ;;
    *)
        echo "Unsupported platform: $OSTYPE"
        exit 1
        ;;
esac

echo "✅ Exported symbols of $LIBRARY match expected list in $EXPECTED_FILE"
exit 0

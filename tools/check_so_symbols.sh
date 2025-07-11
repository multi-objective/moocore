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
        if ! diff -q <(echo "$EXPORTED") "${EXPECTED_FILE}" &> /dev/null; then
            echo "❌ Exported symbols do not match expected list:"
            diff -u <(echo "$EXPORTED") "$EXPECTED_FILE"
            exit 1
        fi
        ;;
    [Ll]inux*)
        echo "[Linux] Extracting symbols..."
        EXPORTED=$(nm -P -g --defined-only "$LIBRARY" | tr -s ' ' | grep ' T ' | cut -d' ' -f1 | sort)
        # Compare both lists
        if ! diff -q <(echo "$EXPORTED") "$EXPECTED_FILE" &> /dev/null; then
            echo "❌ Exported symbols do not match expected list:"
            diff -u <(echo "$EXPORTED") "$EXPECTED_FILE"
            exit 1
        fi
        ;;
    [Ww]indows*|msys*|cygwin*)
        TMP_ACTUAL=$(mktemp)
        if command -v dumpbin &>/dev/null; then
            echo "[Windows MSVC] Extracting symbols..."
            dumpbin /EXPORTS "$LIBRARY" |
                while read -r line; do
                    if [[ "$line" =~ ^[[:space:]]*[0-9]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+[0-9A-Fa-f]+[[:space:]]+(.+) ]]; then
                        symbol="${BASH_REMATCH[1]}"
                        undname "$symbol"
                    fi
                done | sort > "${TMP_ACTUAL}"
        elif command -v nm &>/dev/null; then
            if command -v gcc-nm &>/dev/null; then
                echo "[Windows MinGW gcc-nm] Extracting symbols..."
                NM=gcc-nm
            elif command -v llvm-nm &>/dev/null; then
                echo "[Windows LLVM llvm-nm] Extracting symbols..."
                NM=llvm-nm
            else
                echo "[Windows nm] Extracting symbols..."
                NM=nm
            fi
            $NM -P -g --defined-only "$LIBRARY" | tr -s ' ' | sed 's/^_//' | grep ' T ' | cut -d' ' -f1 | sort > "${TMP_ACTUAL}"
            cat "$TMP_ACTUAL"
        else
            echo "No supported symbol tool found on Windows."
            exit 1
        fi
        cat "${EXPECTED_FILE}"
        MISSING=$(comm -23  "${EXPECTED_FILE}" "${TMP_ACTUAL}")
        # Compare both lists
        if [ -n "$MISSING" ]; then
            echo "❌ Exported symbols of $LIBRARY do NOT match expected list in $EXPECTED_FILE :"
            echo $MISSING
            diff -u "${EXPECTED_FILE}" "${TMP_ACTUAL}"
            rm -f "$TMP_ACTUAL"
            exit 1
        fi
        rm -f "$TMP_ACTUAL"
        ;;
    *)
        echo "Unsupported platform: $OSTYPE"
        exit 1
        ;;
esac

echo "✅ Exported symbols of $LIBRARY match expected list in $EXPECTED_FILE"
exit 0

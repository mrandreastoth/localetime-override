#!/bin/bash

# Define paths
LIB_PATH="$PWD/liblocaletime_override.so"
USR_TEST_CONF="$PWD/test_usr_localetime-override.conf"
SYS_TEST_CONF="$PWD/test_sys_localetime-override.conf"
USR_CONF="$HOME/.config/localetime-override.conf"
SYS_CONF="/etc/localetime-override.conf"
CACHE_FILE="/tmp/localetime_cache.dat"

# Backup paths
USR_BACKUP="$USR_CONF.bak"
SYS_BACKUP="$SYS_CONF.bak"

# Log file
LOG_FILE="localetime_test_results.log"

# Ensure cleanup runs on EXIT (finally-like behavior)
cleanup() {
    echo "🔹 Restoring original configurations..."
    [[ -f "$USR_BACKUP" ]] && mv "$USR_BACKUP" "$USR_CONF" || rm -f "$USR_CONF"
    [[ -f "$SYS_BACKUP" ]] && sudo mv "$SYS_BACKUP" "$SYS_CONF" || sudo rm -f "$SYS_CONF"
    rm -f "$CACHE_FILE"
    echo "✅ Cleanup complete."
}
trap cleanup EXIT

# Ensure the override library exists
if [[ ! -f "$LIB_PATH" ]]; then
    echo "❌ Error: Override library ($LIB_PATH) not found. Compile it first."
    exit 1
fi

# Backup existing conf files
echo "🔹 Backing up existing configuration files..."
[[ -f "$USR_CONF" ]] && cp "$USR_CONF" "$USR_BACKUP"
[[ -f "$SYS_CONF" ]] && sudo cp "$SYS_CONF" "$SYS_BACKUP"

# Copy test config files
echo "🔹 Applying test configurations..."
mkdir -p "$HOME/.config"
cp "$USR_TEST_CONF" "$USR_CONF"
sudo cp "$SYS_TEST_CONF" "$SYS_CONF"

# **Verify the destination config files before running tests**
echo "🔹 Verifying configuration files before running tests..."
ls -l "$USR_CONF" "$SYS_CONF"
echo "🔹 Contents of user configuration file ($USR_CONF):"
cat "$USR_CONF"
echo "🔹 Contents of system configuration file ($SYS_CONF):"
sudo cat "$SYS_CONF"

# Enable debugging
export LOCALETIME_OVERRIDE_DEBUG=1

# Test cases
TEST_CASES=(
    # Full weekday names (%A)
    "2024-01-01 | %A | usr_Monday"       "User override for full weekday name (Monday)"
    "2024-01-02 | %A | sys_Tuesday"      "User missing, system present → use system"
    "2024-01-03 | %A | usr_Wednesday"    "User override for full weekday name (Wednesday)"
    "2024-01-04 | %A | usr_Thursday"     "User override for full weekday name (Thursday)"
    "2024-01-05 | %A | (locale default)" "User & system missing → Locale fallback"
    "2024-01-06 | %A | sys_Saturday"     "User missing, system present → use system"
    "2024-01-07 | %A | usr_Sunday"       "User override for full weekday name (Sunday)"

    # Abbreviated weekday names (%a)
    "2024-01-01 | %a | uMo"              "User override for abbreviated weekday (Mon)"
    "2024-01-02 | %a | uTu"              "User override for abbreviated weekday (Tue)"
    "2024-01-03 | %a | sWe"              "User missing, system present → use system"
    "2024-01-04 | %a | (locale default)" "User & system missing → Locale fallback"
    "2024-01-05 | %a | sFr"              "User missing, system present → use system"
    "2024-01-06 | %a | uSa"              "User override for abbreviated weekday (Sat)"
    "2024-01-07 | %a | sSu"              "User missing, system present → use system"

    # Full month names (%B)
    "2024-01-01 | %B | sys_January"      "User missing, system present → use system"
    "2024-02-01 | %B | usr_February"     "User override for full month name (February)"
    "2024-03-01 | %B | usr_March"        "User override for full month name (March)"
    "2024-04-01 | %B | sys_April"        "User empty, fallback to system"
    "2024-05-01 | %B | sys_May"          "User missing, system present → use system"
    "2024-06-01 | %B | sys_June"         "User missing, system present → use system"
    "2024-07-01 | %B | usr_July"         "User override for full month name (July)"
    "2024-08-01 | %B | sys_August"       "User missing, system present → use system"
    "2024-09-01 | %B | sys_September"    "User missing, system present → use system"
    "2024-10-01 | %B | usr_October"      "User override for full month name (October)"
    "2024-11-01 | %B | sys_November"     "User missing, system present → use system"
    "2024-12-01 | %B | (locale default)" "User & system missing → Locale fallback"
)

# Log test results
echo "📝 Running tests..." > "$LOG_FILE"

# Run test cases
run_tests() {
    local EXPECT_FAILURE=$1
    for ((i=0; i<${#TEST_CASES[@]}; i+=2)); do
        TEST="${TEST_CASES[i]}"
        DESCRIPTION="${TEST_CASES[i+1]}"

        DATE="${TEST% | * | *}"
        FORMAT="${TEST#* | }"
        FORMAT="${FORMAT% | *}"
        EXPECTED="${TEST##* | }"

        ACTUAL=$(LD_PRELOAD="$LIB_PATH" date -d "$DATE" +"$FORMAT")

        echo "🔹 $DESCRIPTION"

        if [[ "$EXPECTED" == "(locale default)" ]]; then
            if [[ "$ACTUAL" =~ sys_.* || "$ACTUAL" =~ usr_.* ]]; then
                if [[ $EXPECT_FAILURE -eq 1 ]]; then
                    echo "⚠️  EXPECTED FAILURE: $DATE $FORMAT → Got: $ACTUAL" | tee -a "$LOG_FILE"
                else
                    echo "❌ FAILED: $DATE $FORMAT → Expected: Locale default, Got: $ACTUAL" | tee -a "$LOG_FILE"
                    exit 1
                fi
            else
                echo "✅ PASSED: $DATE $FORMAT → $ACTUAL (Locale default)" | tee -a "$LOG_FILE"
            fi
        else
            if [[ "$ACTUAL" == "$EXPECTED" ]]; then
                echo "✅ PASSED: $DATE $FORMAT → $ACTUAL" | tee -a "$LOG_FILE"
            else
                if [[ $EXPECT_FAILURE -eq 1 ]]; then
                    echo "⚠️  EXPECTED FAILURE: $DATE $FORMAT → Got: $ACTUAL" | tee -a "$LOG_FILE"
                else
                    echo "❌ FAILED: $DATE $FORMAT → Expected: $EXPECTED, Got: $ACTUAL" | tee -a "$LOG_FILE"
                    exit 1
                fi
            fi
        fi
    done
}

# Run initial tests
run_tests

# Cache-related tests: Modify files, re-run full test suite
echo "📝 Running cache tests..." >> "$LOG_FILE"

CACHE_ACTIONS=(
    "Touch user config"         "touch $USR_CONF"              "0"
    "Delete system config"      "sudo rm -f $SYS_CONF"         "1"
    "Recreate user config"      "cp $USR_TEST_CONF $USR_CONF"  "0"
    "Delete cache"              "rm -f $CACHE_FILE"            "0"
    "Delete user config"        "rm -f $USR_CONF"              "1"
    "Delete both configs"       "rm -f $USR_CONF && sudo rm -f $SYS_CONF" "1"
    "Recreate system config"    "sudo cp $SYS_TEST_CONF $SYS_CONF" "0"
)

for ((i=0; i<${#CACHE_ACTIONS[@]}; i+=3)); do
    ACTION_DESC="${CACHE_ACTIONS[i]}"
    ACTION_CMD="${CACHE_ACTIONS[i+1]}"
    EXPECT_FAILURE="${CACHE_ACTIONS[i+2]}" # Now we explicitly set this per test!

    echo "🔄 Cache test: $ACTION_DESC"
    eval "$ACTION_CMD"

    run_tests "$EXPECT_FAILURE"
done

echo "📜 Test results saved to $LOG_FILE"
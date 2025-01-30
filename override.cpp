#include <dlfcn.h>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <ctime>
#include <langinfo.h>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <format>

// TODO: Make thread-safe and re-entrant!!!
//       Need to then add tests for this as well!!
// TODO: Only strftime() hook has been tested, but tests are
//       *required* for nl_langinfo() as well!!! 
// TODO: Do we enforce three characters for short days/months???
//       If we do this, then we will need to add tests for this as well!!!
// TODO: Review tryLoadCachedData() log messages mentioning conf
//       files when it should know nothing about such files!!!

// Function prototype for original functions
using orig_strftime_t = size_t (*)(char *, size_t, const char *, const struct tm *);
using orig_nl_langinfo_t = char *(*)(nl_item);

// English weekday and month names for correct lookup
const std::unordered_map<int, std::string> ENGLISH_DAYS = {
    {0, "Sunday"}, {1, "Monday"}, {2, "Tuesday"}, {3, "Wednesday"},
    {4, "Thursday"}, {5, "Friday"}, {6, "Saturday"}
};

const std::unordered_map<int, std::string> ENGLISH_MONTHS = {
    {0, "January"}, {1, "February"}, {2, "March"}, {3, "April"},
    {4, "May"}, {5, "June"}, {6, "July"}, {7, "August"},
    {8, "September"}, {9, "October"}, {10, "November"}, {11, "December"}
};

// Debug logging function
void debug_log(const std::string &message) {
    if (getenv("LOCALETIME_OVERRIDE_DEBUG")) {
        std::cerr << "[LOCALETIME_OVERRIDE_DEBUG] " << message << std::endl;
    }
}

// Convert string to lowercase (case-insensitive lookup support)
std::string toLower(const std::string &str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return lower_str;
}

// File paths for override configs
const std::string user_config_path = getenv("HOME") + std::string("/.config/localetime-override.conf");
const std::string system_config_path = "/etc/localetime-override.conf";

// Timestamps and sizes for config change detection
time_t last_user_mtime = 0, last_system_mtime = 0;
off_t last_user_size = 0, last_system_size = 0;

// Single dictionary for overrides
std::unordered_map<std::string, std::string> overrides;

const uint32_t CACHE_MAGIC_HEADER = 0x4C544F43; // "LTOC" (LocalTimeOverrideCache)
const uint32_t CACHE_MAGIC_FOOTER = 0x464F4F54; // "FOOT" (Footer marker)

bool tryLoadCachedData() 
{
    std::ifstream cache_file("/tmp/localetime_cache.dat", std::ios::binary);

    if (!cache_file) 
    {
        debug_log("🔶 Warning: No cache found. Reloading from config files.");
        return false;
    }

    // 🔍 Read and verify header
    uint32_t file_header = 0;
    cache_file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));

    if (file_header != CACHE_MAGIC_HEADER) 
    {
        debug_log("❌ Cache corruption detected! Invalid header. Reloading from config files.");
        return false;
    }

    // 📝 Load cached file metadata
    time_t cached_user_mtime, cached_system_mtime;
    off_t cached_user_size, cached_system_size;

    cache_file.read(reinterpret_cast<char*>(&cached_user_mtime), sizeof(cached_user_mtime));
    cache_file.read(reinterpret_cast<char*>(&cached_user_size), sizeof(cached_user_size));
    cache_file.read(reinterpret_cast<char*>(&cached_system_mtime), sizeof(cached_system_mtime));
    cache_file.read(reinterpret_cast<char*>(&cached_system_size), sizeof(cached_system_size));

    // 🚀 Get current file metadata
    struct stat user_stat{}, system_stat{};
    time_t new_user_mtime = 0, new_system_mtime = 0;
    off_t new_user_size = 0, new_system_size = 0;

    if (stat(user_config_path.c_str(), &user_stat) == 0) 
    {
        new_user_mtime = user_stat.st_mtime;
        new_user_size = user_stat.st_size;
    }

    if (stat(system_config_path.c_str(), &system_stat) == 0) 
    {
        new_system_mtime = system_stat.st_mtime;
        new_system_size = system_stat.st_size;
    }

    debug_log(std::format("📄 Checking cache validity... User: (mtime={}, size={}) → (mtime={}, size={})", 
                          cached_user_mtime, cached_user_size, new_user_mtime, new_user_size));
    debug_log(std::format("📄 Checking cache validity... System: (mtime={}, size={}) → (mtime={}, size={})", 
                          cached_system_mtime, cached_system_size, new_system_mtime, new_system_size));

    // ✅ If nothing has changed, use cached dictionary
    if (cached_user_mtime == new_user_mtime && cached_user_size == new_user_size &&
        cached_system_mtime == new_system_mtime && cached_system_size == new_system_size) 
    {
        size_t dict_size = 0;
        cache_file.read(reinterpret_cast<char*>(&dict_size), sizeof(dict_size));

        overrides.clear();
        for (size_t i = 0; i < dict_size; ++i) 
        {
            size_t key_len = 0, value_len = 0;

            cache_file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
            std::string key(key_len, '\0');
            cache_file.read(&key[0], key_len);

            cache_file.read(reinterpret_cast<char*>(&value_len), sizeof(value_len));
            std::string value(value_len, '\0');
            cache_file.read(&value[0], value_len);

            overrides[key] = value;
        }

        // 🔍 Read and verify footer
        uint32_t file_footer = 0;
        cache_file.read(reinterpret_cast<char*>(&file_footer), sizeof(file_footer));

        if (file_footer != CACHE_MAGIC_FOOTER) 
        {
            debug_log("🔄 Cache data invalid, using latest config.");
            return false;
        }

        debug_log("✅ Loaded dictionary from cache. Config files unchanged.");
        return true;
    }

    debug_log("🔄 Config files changed. Reloading from disk...");
    return false; // Signals that we must reload from config files
}

void saveCachedData() 
{
    std::ofstream cache_file("/tmp/localetime_cache.dat", std::ios::binary);

    if (!cache_file) 
    {
        debug_log("🔶 Warning: Failed to save cache. System will function, but performance may be affected.");
        return;
    }

    // 📝 Write header
    cache_file.write(reinterpret_cast<const char*>(&CACHE_MAGIC_HEADER), sizeof(CACHE_MAGIC_HEADER));

    // 📝 Write file timestamps and sizes
    cache_file.write(reinterpret_cast<const char*>(&last_user_mtime), sizeof(last_user_mtime));
    cache_file.write(reinterpret_cast<const char*>(&last_user_size), sizeof(last_user_size));
    cache_file.write(reinterpret_cast<const char*>(&last_system_mtime), sizeof(last_system_mtime));
    cache_file.write(reinterpret_cast<const char*>(&last_system_size), sizeof(last_system_size));

    // 📝 Write dictionary size
    size_t dict_size = overrides.size();
    cache_file.write(reinterpret_cast<const char*>(&dict_size), sizeof(dict_size));

    // 📝 Write dictionary key-value pairs
    for (const auto &entry : overrides) 
    {
        size_t key_len = entry.first.size();
        size_t value_len = entry.second.size();

        cache_file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        cache_file.write(entry.first.data(), key_len);
        cache_file.write(reinterpret_cast<const char*>(&value_len), sizeof(value_len));
        cache_file.write(entry.second.data(), value_len);
    }

    // 📝 Write footer
    cache_file.write(reinterpret_cast<const char*>(&CACHE_MAGIC_FOOTER), sizeof(CACHE_MAGIC_FOOTER));

    debug_log("✅ Cached override dictionary & file stats.");
}

void tryRefreshOverrides() {
    debug_log("🔹 Attempting to refresh override configurations...");

    // 🔄 Try to load from cache first
    if (tryLoadCachedData()) {
        debug_log("✅ Using cached overrides. No need to reload from files.");
        return;
    }

    // 🚀 Get latest file stats
    struct stat user_stat{}, system_stat{};
    time_t new_user_mtime = 0, new_system_mtime = 0;
    off_t new_user_size = 0, new_system_size = 0;

    if (stat(user_config_path.c_str(), &user_stat) == 0) {
        new_user_mtime = user_stat.st_mtime;
        new_user_size = user_stat.st_size;
    }

    if (stat(system_config_path.c_str(), &system_stat) == 0) {
        new_system_mtime = system_stat.st_mtime;
        new_system_size = system_stat.st_size;
    }

    // 🛑 If file stats haven’t changed, skip reloading
    if (new_user_mtime == last_user_mtime && new_user_size == last_user_size &&
        new_system_mtime == last_system_mtime && new_system_size == last_system_size) {
        debug_log("✅ Config files unchanged. Using cached dictionary.");

        return;
    }

    // 🔄 Reload the dictionary from config files
    std::ifstream user_config(user_config_path);
    std::ifstream system_config(system_config_path);
    overrides.clear();

    auto loadConfig = [](std::ifstream &file, bool is_user_config) {
        std::string line;
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n")); // Trim leading whitespace

            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            size_t equal_pos = line.find('=');

            if (equal_pos == std::string::npos || equal_pos == 0) {
                continue;
            }

            std::string key = line.substr(0, equal_pos);
            std::string value = line.substr(equal_pos + 1);

            key = toLower(key);
            value.erase(0, value.find_first_not_of(" \t\r\n")); // Trim leading whitespace
            value.erase(value.find_last_not_of(" \t\r\n") + 1); // Trim trailing whitespace

            if (!value.empty()) {
                if (is_user_config || overrides.find(key) == overrides.end()) {
                    overrides[key] = value;
                }
            }
        }
    };

    if (user_config.is_open()) {
        loadConfig(user_config, true);
    }

    if (system_config.is_open()) {
        loadConfig(system_config, false);
    }

    debug_log("✅ Finished loading overrides.");

    // ✅ Update cached timestamps and dictionary
    last_user_mtime = new_user_mtime;
    last_user_size = new_user_size;
    last_system_mtime = new_system_mtime;
    last_system_size = new_system_size;
    saveCachedData();
}

// Apply overrides to localized names
std::string tryApplyOverride(const std::string &key, const std::string &original_value, bool is_short) {
    tryRefreshOverrides(); // Ensure latest overrides are used

    debug_log(std::format("🔹 Trying to apply override for key: [{}] (Short: {})", key, is_short ? "Yes" : "No"));

    bool has_leading_space = (!original_value.empty() && original_value[0] == ' ');

    // Use the correct prefix to prevent clashes
    std::string lookup_key = toLower(key);
    std::string prefixed_key = (is_short ? "s_" : "l_") + lookup_key;

    debug_log(std::format("🔹 Looking up override for normalized key: [{}]", prefixed_key));

    // Try exact match with the prefix
    auto override_entry = overrides.find(prefixed_key);

    if (override_entry != overrides.end()) {
        std::string result = override_entry->second;
        debug_log(std::format("✅ Override found: [{}] → [{}]", prefixed_key, result));

        return has_leading_space ? " " + result : result;
    }

    debug_log(std::format("🔶 No override found for: [{}], returning original value: [{}]", prefixed_key, original_value));
    return original_value;
}

std::string lookupAndOverride(int key_index, bool is_short, bool is_day, const std::string &original_value) {
    std::string key = is_day ? ENGLISH_DAYS.at(key_index) : ENGLISH_MONTHS.at(key_index);

    if (is_short) {
        key = key.substr(0, 3);
    }

    debug_log(std::format("Determined lookup key: [{}] (Short: {})", key, is_short ? "Yes" : "No"));
    
    std::string result = tryApplyOverride(key, original_value, is_short);
    debug_log(std::format("Overriding with: [{}] (Length: {})", result, result.length()));
    
    return result;
}

void applyOverride(std::string &result, const std::string &format, int key_index, bool is_short, bool is_day) {
    std::string key = is_day ? ENGLISH_DAYS.at(key_index) : ENGLISH_MONTHS.at(key_index);

    if (is_short) {
        key = key.substr(0, 3);
    }

    debug_log(std::format("Determined lookup key: [{}] (Short: {})", key, is_short ? "Yes" : "No"));

    std::string overridden = tryApplyOverride(key, result, is_short);
    debug_log(std::format("Overriding with: [{}] (Length: {})", overridden, overridden.length()));

    result = overridden;
}

// Hooked strftime function
extern "C" size_t strftime(char *s, size_t max, const char *format, const struct tm *tm) {
    static orig_strftime_t original_strftime = nullptr;

    if (!original_strftime) {
        original_strftime = (orig_strftime_t)dlsym(RTLD_NEXT, "strftime");
    }

    std::string original_value(1024, '\0');
    size_t original_len = original_strftime(original_value.data(), original_value.size(), format, tm);
    original_value.resize(original_len);  // Trim to actual length

    debug_log(std::format("Original strftime() returned: [{}] (Length: {})", original_value, original_len));

    std::string result = original_value;

    if (strstr(format, "%A")) {
        applyOverride(result, format, tm->tm_wday, false, true);
    } else if (strstr(format, "%a")) {
        applyOverride(result, format, tm->tm_wday, true, true);
    } else if (strstr(format, "%B")) {
        applyOverride(result, format, tm->tm_mon, false, false);
    } else if (strstr(format, "%b")) {
        applyOverride(result, format, tm->tm_mon, true, false);
    }

    size_t len = std::min(result.length(), max - 1);
    strncpy(s, result.c_str(), len);
    s[len] = '\0';

    return len;
}

// Hooked nl_langinfo function
extern "C" char *nl_langinfo(nl_item item) {
    static orig_nl_langinfo_t original_nl_langinfo = nullptr;

    if (!original_nl_langinfo) {
        original_nl_langinfo = (orig_nl_langinfo_t)dlsym(RTLD_NEXT, "nl_langinfo");
    }

    static std::string buffer;
    buffer = original_nl_langinfo(item);
    
    debug_log(std::format("Original nl_langinfo() returned: [{}]", buffer));

    bool is_day = false, is_short = false;
    int key_index = -1;

    if (item >= DAY_1 && item <= DAY_7) {
        key_index = item - DAY_1;
        is_day = true;
        is_short = false;
    } else if (item >= ABDAY_1 && item <= ABDAY_7) {
        key_index = item - ABDAY_1;
        is_day = true;
        is_short = true;
    } else if (item >= MON_1 && item <= MON_12) {
        key_index = item - MON_1;
        is_day = false;
        is_short = false;
    } else if (item >= ABMON_1 && item <= ABMON_12) {
        key_index = item - ABMON_1;
        is_day = false;
        is_short = true;
    } else {
        return buffer.data();
    }

    applyOverride(buffer, "", key_index, is_short, is_day);

    debug_log(std::format("Overriding nl_langinfo() with: [{}]", buffer));
    return buffer.data();
}
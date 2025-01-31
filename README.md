# localetime-override

## Current Status: **Proof of Concept Only**

### ⚠️ WARNING: THIS PROJECT IS INCOMPLETE ⚠️

- The current implementation **only handles a small subset of date format specifiers** (i.e., `%A`, `%a`, `%B`, `%b`).
- The complete format specification **is not yet implemented**.
- This project **should not** be considered a complete solution at this stage.
- It is intended as a **technical proof of concept**, demonstrating the feasibility of overriding localized date strings.

### ⚠️ SYSTEM-LEVEL INSTALLATION MAY CAUSE DEADLOCKS AND KERNEL PANICS ⚠️

- **Installing this at the system level (via `/etc/ld.so.preload`) can lead to severe issues, including:**
  - Deadlocks when modifying system files.
  - Kernel panic on boot, rendering the system unbootable.
  - Inability to access or modify certain files due to unexpected dependencies.
- **Use this software at your own risk.** The author takes **no liability** for any damage caused.

---

## Introduction

### The Fundamental Flaw in UNIX/Linux Locales

On UNIX/Linux systems, locale settings are used to define how dates, times, and other language-specific elements are displayed. However, there is a **fundamental flaw** in the way locales work: they assume **"location is language."**

For example, if a user configures their system to use UK English (`en_GB.UTF-8`) but sets their timezone to Hungary (`Europe/Budapest`), they may expect their system to display **everything in English** while keeping the correct time for Hungary. However, due to the way locales work, the system **mixes English UI elements with Hungarian month and weekday names in formatted dates.**

This happens because the locale system in UNIX/Linux embeds the short and long names of days and months **as part of the locale definition itself**, tying them to the regional settings rather than allowing independent control over language.

---

## The Goal of `localetime-override`

The purpose of `localetime-override` is to **decouple language from location**, allowing users to override the localized names of weekdays and months **without affecting other locale settings like number formats or currency symbols.**

Although the current implementation relies on **configuration files**, which might seem similar to defining custom UNIX/Linux locales, the **ultimate goal** is to provide **dynamic language lookups** based on user preference alone. This implementation is just the first building block toward that solution.

---

## Existing Solutions and Their Limitations

Several existing approaches attempt to solve this issue, but they all have significant drawbacks:

### 1. **Custom Locales**
   - **Pros:** Provides full control over localized elements.
   - **Cons:** Requires users to manually generate and maintain a custom locale file (`locale-gen`). This is cumbersome, requires system privileges, and must be recompiled every time changes are made.

### 2. **Environment Variable Overrides (`LANG`, `LC_TIME`, etc.)**
   - **Pros:** Easy to set up and modify per-user.
   - **Cons:** Affects all locale-dependent elements (e.g., numbers, currency, collation), not just date and time formatting. Does not provide fine-grained control over individual elements.

### 3. **Locale-Specific Patches**
   - **Pros:** Can be integrated into system-wide locale management.
   - **Cons:** Requires modification of system files and can break after system updates. Not portable across different distributions.

### 4. **Using Different Language for UI and Time Formatting**
   - **Pros:** Works with some applications that allow custom locale settings.
   - **Cons:** Not all applications respect mixed settings. Some applications force a single locale, meaning elements like timestamps in logs can still be incorrect.

### **Why `localetime-override`?**
This project provides **the best trade-off** by allowing runtime **interception and modification** of localized time elements without requiring system-wide changes or recompilation of locale data. It is dynamic, flexible, and can be configured per user or system-wide.

---

## How `localetime-override` Works

`localetime-override` is implemented as a **shared library** (`liblocaletime_override.so`) that hooks into the system's `strftime()` and `nl_langinfo()` functions via **LD_PRELOAD**.

- When an application calls `strftime()` to format a date, the library intercepts the call and **replaces the weekday/month names** based on user-defined overrides.
- When an application calls `nl_langinfo()` to retrieve localized names, the library intercepts the call and **substitutes the results** dynamically.
- The library caches configuration data to minimize performance overhead and ensure efficient lookups.

### Example Configuration File

Users can define their custom overrides in two configuration files:
- **User-specific:** `~/.config/localetime-override.conf`
- **System-wide:** `/etc/localetime-override.conf`

Example `localetime-override.conf`:
```
l_monday = Mon
l_tuesday = Tue
l_wednesday = Wed
l_thursday = Thu
l_friday = Fri
l_saturday = Sat
l_sunday = Sun

s_january = Jan
s_february = Feb
s_march = Mar
s_april = Apr
s_may = May
s_june = Jun
s_july = Jul
s_august = Aug
s_september = Sep
s_october = Oct
s_november = Nov
s_december = Dec
```

---

## Installation

To install `localetime-override` at a system-wide level (**⚠ Not recommended due to stability issues**):

```sh
sudo ./install_localetime_override.sh
```

To install it for a single user:
```sh
export LD_PRELOAD=/path/to/liblocaletime_override.so
```

### Uninstallation

To **remove** the library from the system:

```sh
sudo ./uninstall_localetime_override.sh
```

To temporarily disable the override for a session:

```sh
unset LD_PRELOAD
```

---

## Current Issues and Limitations

- **Deadlocks and Kernel Panics**  
  Installing `localetime-override` globally can cause **system deadlocks and boot failures**. This issue occurs because certain system services rely on `strftime()` for logging, leading to recursive calls that the current implementation does not handle properly.

- **Re-entrancy Issues**  
  The library **is not yet fully re-entrant**, meaning that if `strftime()` calls itself in a nested way, it can cause undefined behavior.

- **Cache Handling Needs More Testing**  
  The caching mechanism works, but it **has not been stress-tested for corruption scenarios**. A dedicated test should be added to verify how the library reacts to a corrupted cache.

- **Lack of GUI**  
  A planned feature is a **GUI-based locale manager**, similar to what Windows provides, for easier customization of locale elements.

---

## Future Plans

- **Improve stability to prevent deadlocks and boot failures.**
- **Ensure full re-entrancy and thread-safety.**
- **Expand support for additional locale elements beyond date formatting.**
- **Develop a GUI tool for easier configuration.**
- **Enhance caching logic to detect and recover from corruption automatically.**

---

## License

This project is licensed under the MIT License.

---

## Author

Developed by **Andreas Toth**.


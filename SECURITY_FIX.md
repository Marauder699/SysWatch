# Command Injection Vulnerability Fix

## Identified Issue

GitHub CodeQL detected a command injection vulnerability (CWE-78, CWE-88) in [src/storage_info.c](src/storage_info.c#L329).

### Description
The code was reading storage device names from `/sys/block/` via `fgets()` and using them directly in shell commands via `popen()` without validation. A potential attacker could have injected malicious commands.

### Vulnerable Code
```c
// storage_name comes from fgets() and is used directly in popen()
snprintf(partition_pattern, sizeof(partition_pattern),
         "df -B 1M | grep '/dev/%s' | ...", storage_name);
FILE *df_fp = popen(partition_pattern, "r");
```

## Implemented Solution

### Added Validation Function
A `is_safe_storage_name()` function was added to validate device names:

```c
static bool is_safe_storage_name(const char *name) {
    if (name == NULL || name[0] == '\0') {
        return false;
    }
    
    size_t len = strlen(name);
    if (len == 0 || len > 31) {
        return false;
    }
    
    // Only accepts: letters, digits, hyphens, underscores
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        if (!isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    return true;
}
```

### Validation Application
The device name is now validated before any usage:

```c
while (fgets(storage_name, sizeof(storage_name), fp) != NULL) {
    // Remove newline
    size_t len = strlen(storage_name);
    if (len > 0 && storage_name[len-1] == '\n') {
        storage_name[len-1] = '\0';
    }
    
    // SECURITY: Validate name to prevent command injection
    if (!is_safe_storage_name(storage_name)) {
        continue;  // Skip suspicious names
    }
    
    // ... rest of code ...
}
```

## Validation Tests

### Accepted Names ✓
- `nvme0n1`
- `sda`, `sdb`, `sdc`
- `mmcblk0`
- `hda`

### Rejected Names ✗
- `sdb; rm -rf /` (command injection)
- `sdc$(echo pwned)` (command substitution)
- `mmc'blk0` (malicious quotes)
- Any name containing shell special characters

## Impact on Functionality

✅ **No negative impact**: Functionality remains intact as all legitimate Linux storage device names contain only alphanumeric characters, hyphens and underscores.

## Status

✅ **Fixed and tested**
- Compilation successful
- Functional validation confirmed
- Injection attempts successfully blocked

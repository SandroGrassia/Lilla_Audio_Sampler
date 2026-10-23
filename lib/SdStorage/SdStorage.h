#pragma once

#include <SD.h>

class SdStorage
{
public:
    FLASHMEM static bool Begin(); // Initialize SD and migrate/hide an existing firmware directory before opening files.
    FLASHMEM static bool Ensure(); // Create and hide the firmware directory on an initialized SD with no open files.
private:
    FLASHMEM static bool Hide(); // Update and verify only the directory attributes, then remount to discard stale caches.
    static bool Hide_entry(uint8_t *entry, size_t bytes, bool exfat); // Validate a directory entry set and add Hidden/System while preserving other metadata.
};

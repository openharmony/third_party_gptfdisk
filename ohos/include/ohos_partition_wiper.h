/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef OHOS_PARTITION_WIPER_H
#define OHOS_PARTITION_WIPER_H

#include <cstdint>
#include <vector>

// Forward declaration; full type brought in only by the .cpp via diskio.h.
class DiskIO;

// --wipe-partitions mode. sgdisk is non-interactive and has no libblkid, so
// there is no signature-detection "auto" mode; the caller picks never/always.
enum class WipeMode {
    WIPE_NEVER = 0,
    WIPE_ALWAYS = 1,
};

struct WipeRange {
    uint64_t firstLBA;
    uint64_t lastLBA;
};

// Tracks partition LBA ranges created by -n/-N so their filesystem signature
// areas can be zeroed before/after the GPT is saved. All mutating calls are
// unconditional (independent of whether wiping is enabled); the decision to
// actually wipe is made once, at save time, from the parsed mode.
class OhosPartitionWiper final {
public:
    OhosPartitionWiper() = default;
    ~OhosPartitionWiper() = default;

    // Record a freshly created partition's LBA span.
    void RememberRange(uint64_t firstLBA, uint64_t lastLBA);

    // Drop a previously remembered span (e.g. the partition was deleted in the
    // same invocation). No-op if the span is not currently tracked.
    void ForgetRange(uint64_t firstLBA, uint64_t lastLBA);

    // Drop all tracked spans (e.g. table cleared/zapped/overwritten by backup).
    void Clear(void);

    // Zero the first and last 1 MiB of every tracked span. Returns false on
    // I/O failure. An empty list is a no-op success.
    bool WipeAll(DiskIO *disk, uint32_t blockSize) const;

private:
    std::vector<WipeRange> ranges_;
};

// Parse a --wipe-partitions mode token ("never"/"always", case insensitive).
// On success sets `out` and returns true; on an unrecognized value prints a
// diagnostic and returns false. A nullptr mode yields WIPE_NEVER.
bool OhosParseWipeMode(const char *mode, WipeMode &out);

// Apply --wipe-partitions before SaveGPTData: parse `wipeMode` once more
// (it may appear after the device name) and, if WIPE_ALWAYS, zero the
// signature areas of every range tracked by `wiper`. Wiping BEFORE (not
// after) SaveGPTData ensures the signature is gone before DiskSync fires
// the uevent that makes udev/udisks2 rescan -- otherwise blkid could see
// the stale signature and auto-mount before the wipe runs. I/O failure
// only logs a warning here; the caller still proceeds to SaveGPTData so
// the user's -n takes effect (a stale signature is recoverable; a missing
// partition table change is not).
// No-op for nullptr/never. Pulled out of GPTDataCL::DoOptions to keep that
// function's nesting depth within the coding-standard limit.
void OhosWipePartitions(const char *wipeMode, OhosPartitionWiper &wiper,
                        DiskIO *disk, uint32_t blockSize);

#endif // OHOS_PARTITION_WIPER_H

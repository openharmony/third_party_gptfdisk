/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ohos_partition_wiper.h"

#include <iostream>
#include <string>

#include "diskio.h"
#include "support.h"

namespace {
// Amount of data to zero at each end of a created partition. Matches the
// 1 MiB convention used by util-linux fdisk --wipe.
const uint64_t WIPE_PARTITION_BYTES = UINT64_C(1024) * UINT64_C(1024);

uint64_t Min64(uint64_t one, uint64_t two) {
   return (one < two) ? one : two;
} // Min64()

// Zero `numSectors` starting at `startSector` on `disk`, writing in chunks no
// larger than the caller-owned scratch buffer `zeroes` (which holds chunkBytes
// bytes of zeros). The buffer is reused across calls so we avoid a per-call
// heap allocation and zero-fill of ~1 MiB.
int ZeroSectors(DiskIO *disk, uint64_t startSector, uint64_t numSectors,
                uint32_t blockSize, std::vector<unsigned char> &zeroes) {
   uint64_t chunkSectors, thisPass;
   int bytesToWrite;

   if (numSectors == 0)
      return 1;

   if (blockSize == 0)
      blockSize = SECTOR_SIZE;

   chunkSectors = zeroes.size() / blockSize;
   if (chunkSectors == 0)
      chunkSectors = 1;

   while (numSectors > 0) {
      thisPass = Min64(numSectors, chunkSectors);
      bytesToWrite = static_cast<int>(thisPass * blockSize);

      if (!disk->Seek(startSector)) {
         std::cerr << "Unable to seek to sector " << startSector
                   << " while wiping partition signatures.\n";
         return 0;
      } // if

      if (disk->Write(&zeroes[0], bytesToWrite) != bytesToWrite) {
         std::cerr << "Unable to write zeroes at sector " << startSector
                   << " while wiping partition signatures.\n";
         return 0;
      } // if

      startSector += thisPass;
      numSectors -= thisPass;
   } // while

   return 1;
} // ZeroSectors()
} // namespace

void OhosPartitionWiper::RememberRange(uint64_t firstLBA, uint64_t lastLBA) {
   WipeRange range;
   range.firstLBA = firstLBA;
   range.lastLBA = lastLBA;
   ranges_.push_back(range);
} // OhosPartitionWiper::RememberRange()

void OhosPartitionWiper::ForgetRange(uint64_t firstLBA, uint64_t lastLBA) {
   std::vector<WipeRange>::iterator iter = ranges_.begin();

   while (iter != ranges_.end()) {
      if ((iter->firstLBA == firstLBA) && (iter->lastLBA == lastLBA))
         iter = ranges_.erase(iter);
      else
         ++iter;
   } // while
} // OhosPartitionWiper::ForgetRange()

void OhosPartitionWiper::Clear(void) {
   ranges_.clear();
} // OhosPartitionWiper::Clear()

bool OhosPartitionWiper::WipeAll(DiskIO *disk, uint32_t blockSize) const {
   uint64_t wipeSectors, length, frontSectors, backSectors, backStart;

   if (ranges_.empty())
      return true;

   if (blockSize == 0)
      blockSize = SECTOR_SIZE;

   wipeSectors = (WIPE_PARTITION_BYTES + blockSize - 1) / blockSize;
   if (wipeSectors == 0)
      wipeSectors = 1;

   // One reusable zeroed scratch buffer (~1 MiB, block-aligned), shared across
   // every ZeroSectors() call so we don't allocate and zero-fill it per end.
   std::vector<unsigned char> zeroes(static_cast<size_t>(wipeSectors * blockSize), 0);

   if (!disk->OpenForWrite()) {
      std::cerr << "Unable to open disk for wiping partition signatures.\n";
      return false;
   } // if

   for (size_t i = 0; i < ranges_.size(); i++) {
      if (ranges_[i].lastLBA < ranges_[i].firstLBA) {
         std::cerr << "Internal error: invalid partition wipe range.\n";
         return false;
      } // if

      length = ranges_[i].lastLBA - ranges_[i].firstLBA + 1;
      frontSectors = Min64(length, wipeSectors);

      if (!ZeroSectors(disk, ranges_[i].firstLBA, frontSectors, blockSize, zeroes))
         return false;

      if (length > frontSectors) {
         backSectors = Min64(length - frontSectors, wipeSectors);
         backStart = ranges_[i].lastLBA - backSectors + 1;
         if (!ZeroSectors(disk, backStart, backSectors, blockSize, zeroes))
            return false;
      } // if

      std::cout << "Wiped first/last " << (WIPE_PARTITION_BYTES / 1024)
                << " KiB of sectors " << ranges_[i].firstLBA << "-"
                << ranges_[i].lastLBA << ".\n";
   } // for

   return true;
} // OhosPartitionWiper::WipeAll()

bool OhosParseWipeMode(const char *mode, WipeMode &out) {
   out = WipeMode::WIPE_NEVER;
   if (mode == NULL)
      return true;

   std::string value = ToLower(mode);
   if (value == "never") {
      out = WipeMode::WIPE_NEVER;
      return true;
   } // if

   if (value == "always") {
      out = WipeMode::WIPE_ALWAYS;
      return true;
   } // if

   std::cerr << "Invalid --wipe-partitions mode '" << mode
             << "'; expected 'never' or 'always'.\n";
   return false;
} // OhosParseWipeMode()

/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef OHOS_MBR_HELPER_H
#define OHOS_MBR_HELPER_H

#include <cstdint>
#include <string>

#include "basicmbr.h"

// MBR operation result codes
enum class MbrResult {
    SUCCESS = 0,
    ERROR_UNKNOWN = 1,
    ERROR_READ_FAILED = 2,
    ERROR_WRITE_FAILED = 3,
    ERROR_INVALID_ARGUMENT = 4,
    ERROR_NOT_SUPPORT_PART = 5,
    ERROR_GPT_PART = 6
};

#define MIN_MBR_PARTS 1
#define MIN_TYPE_CODE 0x01
#define MAX_TYPE_CODE 0xFF

// MBR partition type code modifier class
class OhosMbrHelper final {
public:
    OhosMbrHelper();
    ~OhosMbrHelper();

    // Load MBR data
    MbrResult LoadMbrData(const std::string &device);

    // Read MBR data from device
    MbrResult ReadMbrFromDevice();

    // Validate MBR data
    MbrResult ValidateMbrData();

    // Change partition type code
    // partNum: Partition number (1-128)
    // typeCode: Type code (0x01-0xFF)
    MbrResult ChangePartitionType(int partNum, uint8_t typeCode);

    // Save MBR data to disk
    MbrResult SaveMbrData();

    // Display MBR partition table
    void DisplayMBRData();

private:
    BasicMBRData *mbrData_; // MBR data object pointer (initialized in constructor)
    std::string device_;
    bool loaded_;
};

#endif // OHOS_MBR_HELPER_H

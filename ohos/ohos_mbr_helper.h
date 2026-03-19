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

#include <string>
#include <cstdint>

// MBR operation result codes
enum class MbrResult {
    SUCCESS = 0,
    ERROR_INVALID_DEVICE = 1,
    ERROR_READ_FAILED = 2,
    ERROR_WRITE_FAILED = 3,
    ERROR_INVALID_PARTITION = 4,
    ERROR_INVALID_TYPECODE = 5,
    ERROR_NOT_MBR_DISK = 6,
    ERROR_GPT_DISK = 7,
    ERROR_EMPTY_PARTITION = 8,
    ERROR_UNKNOWN = 100
};

// MBR partition type code modifier class
class OhosMbrHelper {
public:
    OhosMbrHelper();
    ~OhosMbrHelper();

    // Load MBR data
    MbrResult LoadMbrData(const std::string& device);

    // Validate device path
    MbrResult ValidateDevice(const std::string& device);

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

    // Get error message
    std::string GetLastError() const;

private:
    void* mbrData_;  // BasicMBRData object pointer
    std::string lastError_;
    std::string device_;
    bool loaded_;

    // Set error message
    void SetError(const std::string& error);
};

#endif // OHOS_MBR_HELPER_H
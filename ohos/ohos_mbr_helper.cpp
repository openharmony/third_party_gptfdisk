/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ohos_mbr_helper.h"
#include "../basicmbr.h"
#include <iostream>
#include <sstream>
#include <iomanip>

OhosMbrHelper::OhosMbrHelper()
    : mbrData_(nullptr), loaded_(false) {
    mbrData_ = new BasicMBRData();
}

OhosMbrHelper::~OhosMbrHelper() {
    if (mbrData_ != nullptr) {
        delete static_cast<BasicMBRData*>(mbrData_);
        mbrData_ = nullptr;
    }
}

MbrResult OhosMbrHelper::LoadMbrData(const std::string& device) {
    MbrResult result = ValidateDevice(device);
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    device_ = device;
    result = ReadMbrFromDevice();
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    result = ValidateMbrData();
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);
    mbr->MakeItLegal();
    loaded_ = true;

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ValidateDevice(const std::string& device) {
    if (device.empty()) {
        SetError("Device path is empty");
        return MbrResult::ERROR_INVALID_DEVICE;
    }
    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ReadMbrFromDevice() {
    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);

    if (!mbr->ReadMBRData(device_)) {
        SetError("Failed to read MBR data from device: " + device_);
        return MbrResult::ERROR_READ_FAILED;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ValidateMbrData() {
    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);
    MBRValidity validity = mbr->GetValidity();

    if (validity == gpt || validity == hybrid) {
        SetError("Device appears to be a GPT disk, not MBR");
        return MbrResult::ERROR_GPT_DISK;
    }

    if (validity == invalid) {
        SetError("Invalid MBR data on device");
        return MbrResult::ERROR_NOT_MBR_DISK;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ChangePartitionType(int partNum, uint8_t typeCode) {
    // Check if MBR data is loaded
    if (!loaded_) {
        SetError("MBR data not loaded. Call LoadMbrData() first.");
        return MbrResult::ERROR_READ_FAILED;
    }

    // Validate partition number range (1-128)
    if (partNum < 1 || partNum > 128) {
        std::ostringstream oss;
        oss << "Invalid partition number " << partNum << " (must be 1-128)";
        SetError(oss.str());
        return MbrResult::ERROR_INVALID_PARTITION;
    }

    // Validate range before conversion to prevent truncation
    if (typeCode < 0x01 || typeCode > 0xFF) {
        std::ostringstream oss;
        oss << "Invalid typecode " << typeCode << " (must be 0x01 - 0xFF)";
        SetError(oss.str());
        return MbrResult::ERROR_INVALID_TYPECODE;
    }

    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);

    // Check if partition exists and is not empty
    if (mbr->GetLength(partNum - 1) == 0) {
        std::ostringstream oss;
        oss << "Partition " << partNum << " is empty or does not exist";
        SetError(oss.str());
        return MbrResult::ERROR_EMPTY_PARTITION;
    }

    // Set partition type code
    int result = mbr->SetPartType(partNum - 1, typeCode);
    if (result == 0) {
        std::ostringstream oss;
        oss << "Failed to set type code for partition " << partNum;
        SetError(oss.str());
        return MbrResult::ERROR_UNKNOWN;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::SaveMbrData() {
    if (!loaded_) {
        SetError("MBR data not loaded. Call LoadMbrData() first.");
        return MbrResult::ERROR_READ_FAILED;
    }

    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);

    if (!mbr->WriteMBRData()) {
        SetError("Failed to write MBR data to device: " + device_);
        return MbrResult::ERROR_WRITE_FAILED;
    }

    mbr->DiskSync();
    return MbrResult::SUCCESS;
}

std::string OhosMbrHelper::GetLastError() const {
    return lastError_;
}

void OhosMbrHelper::DisplayMBRData() {
    if (!loaded_) {
        std::cerr << "MBR data not loaded. Call LoadMbrData() first." << std::endl;
        return;
    }

    BasicMBRData* mbr = static_cast<BasicMBRData*>(mbrData_);

    std::cout << "MBR Partition Table:" << std::endl;
    std::cout << "  #  Boot  Start Sector    End Sector    Type Code    Size" << std::endl;

    for (int i = 0; i < 4; i++) {
        uint64_t length = mbr->GetLength(i);
        if (length > 0) {
            uint8_t status = mbr->GetStatus(i);
            uint8_t typeCode = mbr->GetType(i);
            uint64_t firstSector = mbr->GetFirstSector(i);

            std::cout << "  " << (i + 1) << "  ";
            std::cout << (status == 0x80 ? " *   " : "     ");
            std::cout << std::setw(12) << firstSector << "    ";
            std::cout << std::setw(12) << (firstSector + length - 1) << "    ";
            std::cout << "0x" << std::setfill('0') << std::setw(2)
	              << std::hex << static_cast<int>(typeCode) << std::dec;
            std::cout << std::setfill(' ') << "    ";
            std::cout << (length * 512 / (1024 * 1024)) << " MB" << std::endl;
        }
    }
}

void OhosMbrHelper::SetError(const std::string& error) {
    lastError_ = error;
    std::cerr << error << std::endl;
}
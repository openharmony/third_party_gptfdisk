/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ohos_mbr_helper.h"

#include <iomanip>
#include <iostream>

OhosMbrHelper::OhosMbrHelper() : mbrData_(nullptr), device_(""), loaded_(false)
{
    mbrData_ = new (std::nothrow) BasicMBRData();
    if (mbrData_ == nullptr) {
        std::cerr << "Failed to allocate memory for MBR data" << std::endl;
        loaded_ = false;
    }
}

OhosMbrHelper::~OhosMbrHelper()
{
    if (mbrData_ != nullptr) {
        delete mbrData_;
        mbrData_ = nullptr;
    }
}

MbrResult OhosMbrHelper::LoadMbrData(const std::string &device)
{
    // mbrData_ is initialized in constructor, no need to check here
    device_ = device;

    MbrResult result = ReadMbrFromDevice();
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    result = ValidateMbrData();
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    mbrData_->MakeItLegal();
    loaded_ = true;

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ReadMbrFromDevice()
{
    if (!mbrData_->ReadMBRData(device_)) {
        std::cerr << "Failed to read MBR data from device: " << device_ << std::endl;
        return MbrResult::ERROR_READ_FAILED;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ValidateMbrData()
{
    MBRValidity validity = mbrData_->GetValidity();

    if (validity == gpt || validity == hybrid) {
        std::cerr << "Device appears to be a GPT disk, not MBR" << std::endl;
        return MbrResult::ERROR_GPT_PART;
    }

    if (validity == invalid) {
        std::cerr << "Invalid MBR data on device" << std::endl;
        return MbrResult::ERROR_NOT_SUPPORT_PART;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::ChangePartitionType(int partNum, uint8_t typeCode)
{
    // Check if MBR data is loaded
    if (!loaded_) {
        std::cerr << "MBR data not loaded. Call LoadMbrData() first." << std::endl;
        return MbrResult::ERROR_READ_FAILED;
    }

    // Validate partition number range (1-128)
    if (partNum < MIN_MBR_PARTS || partNum > MAX_MBR_PARTS) {
        std::cerr << "Invalid partition number (must be 1-128): " << partNum << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    // Validate range before conversion to prevent truncation
    if (typeCode < MIN_TYPE_CODE || typeCode > MAX_TYPE_CODE) {
        std::cerr << "Invalid typecode (must be 0x01-0xFF): 0x" << std::hex << static_cast<int>(typeCode) << std::dec
                  << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    // Check if partition exists and is not empty
    if (mbrData_->GetLength(partNum - 1) == 0) {
        std::cerr << "Partition " << partNum << " is empty or does not exist" << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    // Set partition type code
    int result = mbrData_->SetPartType(partNum - 1, typeCode);
    if (result == 0) {
        std::cerr << "Failed to set type code for partition: " << partNum << std::endl;
        return MbrResult::ERROR_UNKNOWN;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosMbrHelper::SaveMbrData()
{
    if (!loaded_) {
        std::cerr << "MBR data not loaded. Call LoadMbrData() first." << std::endl;
        return MbrResult::ERROR_READ_FAILED;
    }

    if (!mbrData_->WriteMBRData()) {
        std::cerr << "Failed to write MBR data to device: " << device_ << std::endl;
        return MbrResult::ERROR_WRITE_FAILED;
    }

    mbrData_->DiskSync();
    return MbrResult::SUCCESS;
}

void OhosMbrHelper::DisplayMBRData()
{
    if (!loaded_) {
        std::cerr << "MBR data not loaded. Call LoadMbrData() first." << std::endl;
        return;
    }

    // Reuse existing DisplayMBRData implementation from basicmbr.cc
    // This provides more comprehensive MBR information including disk size,
    // disk identifier, and detailed partition information
    mbrData_->DisplayMBRData();
}

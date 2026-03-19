/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ohos_fixparts.h"
#include "../support.h"
#include "../basicmbr.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <getopt.h>

OhosFixparts::OhosFixparts() {
}

OhosFixparts::~OhosFixparts() {
}

MbrResult OhosFixparts::ParseArgs(int argc, char* argv[], OhosFixpartsArgs& args) {
    int opt;
    int option_index = 0;

    static struct option long_options[] = {
        {"print",     no_argument,       nullptr, 'p'},
        {"typecode",  required_argument, nullptr, 't'},
        {"help",      no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    while ((opt = getopt_long(argc, argv, "pt:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                args.printMBR = true;
                break;
            case 't': {
                MbrResult result = ParseOption(optarg, args.partitionNum, args.typeCode);
                if (result != MbrResult::SUCCESS) {
                    return result;
                }
                break;
            }
            case 'h':
                args.showHelp = true;
                ShowHelp(argv[0]);
                return MbrResult::SUCCESS;
            default:
                ShowHelp(argv[0]);
                return MbrResult::ERROR_UNKNOWN;
        }
    }

    // Parse device path (remaining argument)
    if (optind < argc) {
        args.device = argv[optind];
    }

    // Check for extra arguments (should be exactly one device path)
    if (optind + 1 < argc) {
        std::cerr << "Error: Too many device paths specified" << std::endl;
        ShowHelp(argv[0]);
        return MbrResult::ERROR_INVALID_DEVICE;
    }

    // Validate device path
    if (args.device.empty()) {
        std::cerr << "Device path is required" << std::endl;
        ShowHelp(argv[0]);
        return MbrResult::ERROR_INVALID_DEVICE;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosFixparts::ParseOption(const std::string& option, int& partitionNum, uint8_t& typeCode) {

    // Parse partition number and type code
    std::string paramStr = option;
    size_t colonPos = paramStr.find(':');

    if (colonPos == std::string::npos || colonPos == 0 || colonPos == paramStr.length() - 1) {
        std::cerr << "Invalid option format (expected partnum:typecode)" << std::endl;
        return MbrResult::ERROR_INVALID_PARTITION;
    }

    std::string partNumStr = paramStr.substr(0, colonPos);
    std::string typeCodeStr = paramStr.substr(colonPos + 1);

    // Validate partition number - manual validation without exceptions
    if (partNumStr.empty()) {
        std::cerr << "Invalid partition number (must be a number)" << std::endl;
        return MbrResult::ERROR_INVALID_PARTITION;
    }

    // Check if all characters are digits
    for (char c : partNumStr) {
        if (!isdigit(c)) {
            std::cerr << "Invalid partition number (must be a pure number)" << std::endl;
            return MbrResult::ERROR_INVALID_PARTITION;
        }
    }

    // Convert to integer (safe because we validated all characters are digits)
    std::istringstream iss(partNumStr);
    iss >> partitionNum;

    if (iss.fail() || !iss.eof()) {
        std::cerr << "Invalid partition number (must be a number)" << std::endl;
        return MbrResult::ERROR_INVALID_PARTITION;
    }

    if (partitionNum < 1 || partitionNum > 128) {
        std::cerr << "Invalid partition number (must be 1-128)" << std::endl;
        return MbrResult::ERROR_INVALID_PARTITION;
    }

    // Validate and parse type code
    if (!ParseTypeCode(typeCodeStr, typeCode)) {
        std::cerr << "Invalid type code (must be 0x01-0xFF)" << std::endl;
        return MbrResult::ERROR_INVALID_TYPECODE;
    }

    return MbrResult::SUCCESS;
}

bool OhosFixparts::ParseTypeCode(const std::string& str, uint8_t& code) {
    std::istringstream iss(str);
    int value;

    // Check if string starts with 0x/0X or contains only hex digits
    bool isHex = false;
    if (str.find("0x") == 0 || str.find("0X") == 0) {
        isHex = true;
    } else {
        // Check if all characters are valid hex digits (0-9, a-f, A-F)
        for (char c : str) {
            if (!isxdigit(c)) {
                isHex = false;
                break;
            }
            isHex = true;
        }
    }

    if (isHex) {
        iss >> std::hex >> value;
    } else {
        iss >> std::dec >> value;
    }

    if (iss.fail()) {
        return false;
    }

    // Validate range before conversion to prevent truncation
    if (value < 0x01 || value > 0xFF) {
        return false;
    }

    code = static_cast<uint8_t>(value);
    return true;
}

int OhosFixparts::Run(const OhosFixpartsArgs& args) {
    MbrResult result = helper_.LoadMbrData(args.device);
    if (result != MbrResult::SUCCESS) {
        std::cerr << "Failed to load MBR data: " << helper_.GetLastError() << std::endl;
        return static_cast<int>(result);
    }

    // Print mode: display MBR partition table
    if (args.printMBR) {
        helper_.DisplayMBRData();
        return 0;
    }

    // Modify mode: change partition type code
    std::cout << "Changing partition " << args.partitionNum
              << " type code to 0x" << std::hex
              << static_cast<int>(args.typeCode) << std::dec << std::endl;

    result = helper_.ChangePartitionType(args.partitionNum, args.typeCode);
    if (result != MbrResult::SUCCESS) {
        std::cerr << "Failed to change partition type: " << helper_.GetLastError() << std::endl;
        return static_cast<int>(result);
    }

    result = helper_.SaveMbrData();
    if (result != MbrResult::SUCCESS) {
        std::cerr << "Failed to save MBR data: " << helper_.GetLastError() << std::endl;
        return static_cast<int>(result);
    }

    std::cout << "Partition type code changed successfully." << std::endl;
    return 0;
}

void OhosFixparts::ShowHelp(const char* programName) {
    std::cout << "MBR Partition Type Code Modify" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage (Print Mode):" << std::endl;
    std::cout << "  " << programName << " -p <device>" << std::endl;
    std::cout << "  " << programName << " --print <device>" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage (Modify Mode):" << std::endl;
    std::cout << "  " << programName << " -t partnum:typecode <device>" << std::endl;
    std::cout << "  " << programName << " --typecode partnum:typecode <device>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -p, --print                Print MBR partition table" << std::endl;
    std::cout << "  -t partnum:typecode        Change partition type code" << std::endl;
    std::cout << "  -h, --help                 Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " -p /dev/sda" << std::endl;
    std::cout << "  " << programName << " -t 1:0x07 /dev/sda" << std::endl;
    std::cout << "  " << programName << " -t 2:0x83 /dev/sda" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    OhosFixparts fixparts;
    OhosFixpartsArgs args;

    MbrResult result = fixparts.ParseArgs(argc, argv, args);
    if (result != MbrResult::SUCCESS) {
        return static_cast<int>(result);
    }

    // If help was requested, exit successfully
    if (args.showHelp) {
        return 0;
    }

    return fixparts.Run(args);
}
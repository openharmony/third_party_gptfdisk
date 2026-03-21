/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "ohos_fixparts.h"
#include "basicmbr.h"
#include "support.h"
#include <cstring>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

OhosFixparts::OhosFixparts() {}

OhosFixparts::~OhosFixparts() {}

MbrResult OhosFixparts::ParseArgs(int argc, char *argv[], OhosFixpartsArgs &args)
{
    int opt;
    int option_index = 0;
    int typecodeCount = 0; // Count of -t options

    static struct option long_options[] = {{"print", no_argument, nullptr, 'p'},
                                           {"typecode", required_argument, nullptr, 't'},
                                           {"help", no_argument, nullptr, 'h'},
                                           {nullptr, 0, nullptr, 0}};

    while ((opt = getopt_long(argc, argv, "pt:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                args.printMBR = true;
                break;
            case 't': {
                typecodeCount++;
                if (typecodeCount > 1) {
                    std::cerr << "Error: Only one -t option is supported" << std::endl;
                    ShowHelp(argv[0]);
                    return MbrResult::ERROR_INVALID_ARGUMENT;
                }
                MbrResult result = ParseOption(optarg, args.partitionNum, args.typeCode);
                if (result != MbrResult::SUCCESS) {
                    return result;
                }
                args.hasTypeCode = true;
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

    // Validate device path
    if (args.device.empty()) {
        std::cerr << "Device path is required" << std::endl;
        ShowHelp(argv[0]);
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosFixparts::ParseOption(const std::string &option, int &partitionNum, uint8_t &typeCode)
{
    // Parse partition number and type code
    std::string paramStr = option;
    size_t colonPos = paramStr.find(':');

    if (colonPos == std::string::npos || colonPos == 0 || colonPos == paramStr.length() - 1) {
        std::cerr << "Invalid option format (expected partnum:typecode): " << option.c_str() << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    std::string partNumStr = paramStr.substr(0, colonPos);
    std::string typeCodeStr = paramStr.substr(colonPos + 1);

    MbrResult result = ParsePartNum(partNumStr, partitionNum);
    if (result != MbrResult::SUCCESS) {
        return result;
    }

    // Validate and parse type code
    if (!ParseTypeCode(typeCodeStr, typeCode)) {
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    return MbrResult::SUCCESS;
}

MbrResult OhosFixparts::ParsePartNum(const std::string &str, int &partitionNum)
{
    // Validate partition number - manual validation without exceptions
    if (str.empty()) {
        std::cerr << "Invalid partition number: empty string" << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    // Check if all characters are digits
    for (char c : str) {
        if (!isdigit(c)) {
            std::cerr << "Invalid partition number (must be a pure number): " << c << std::endl;
            return MbrResult::ERROR_INVALID_ARGUMENT;
        }
    }

    // Convert to integer (safe because we validated all characters are digits)
    std::istringstream iss(str);
    iss >> partitionNum;

    if (iss.fail() || !iss.eof()) {
        std::cerr << "Parse partNum fail: fail=" << iss.fail() << ", eof=" << iss.eof() << ", str: " << str.c_str()
                  << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    if (partitionNum < MIN_MBR_PARTS || partitionNum > MAX_MBR_PARTS) {
        std::cerr << "Invalid partition number (must be 1-128): " << str.c_str() << std::endl;
        return MbrResult::ERROR_INVALID_ARGUMENT;
    }

    return MbrResult::SUCCESS;
}

bool OhosFixparts::ParseTypeCode(const std::string &str, uint8_t &code)
{
    unsigned int value = 0;

    if (!IsHex(str)) {
        std::cerr << "Invalid hex format: " << str.c_str() << std::endl;
        return false;
    }

    // Parse hex value using std::istringstream (type-safe, no sscanf)
    std::istringstream iss(str);
    iss >> std::hex >> value;

    if (iss.fail() || !iss.eof()) {
        std::cerr << "Parse typecode fail: fail=" << iss.fail() << ", eof=" << iss.eof() << ", str: " << str.c_str()
                  << std::endl;
        return false;
    }

    // Validate range before conversion to prevent truncation
    if (value < MIN_TYPE_CODE || value > MAX_TYPE_CODE) {
        std::cerr << "Invalid type code (must be 0x01-0xFF): 0x" << std::hex << value << std::dec << std::endl;
        return false;
    }

    code = static_cast<uint8_t>(value);
    return true;
}

int OhosFixparts::Run(const OhosFixpartsArgs &args)
{
    MbrResult result = helper_.LoadMbrData(args.device);
    if (result != MbrResult::SUCCESS) {
        return static_cast<int>(result);
    }

    // Print mode: display MBR partition table
    if (args.printMBR) {
        helper_.DisplayMBRData();
    }

    // Modify mode: change partition type code
    if (args.hasTypeCode) {
        std::cout << "Changing partition " << args.partitionNum << " type code to 0x" << std::hex
                  << static_cast<int>(args.typeCode) << std::dec << std::endl;

        result = helper_.ChangePartitionType(args.partitionNum, args.typeCode);
        if (result != MbrResult::SUCCESS) {
            return static_cast<int>(result);
        }

        result = helper_.SaveMbrData();
        if (result != MbrResult::SUCCESS) {
            return static_cast<int>(result);
        }

        std::cout << "Partition type code changed successfully." << std::endl;
    }

    return 0;
}

void OhosFixparts::ShowHelp(const char *programName)
{
    std::cout << "Usage : " << programName << " [OPTION...] <device>" << std::endl;
    std::cout << "  -p, --print                Print MBR partition table" << std::endl;
    std::cout << "  -t partnum:typecode        Change partition type code" << std::endl;
    std::cout << "  -h, --help                 Show this help message" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
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

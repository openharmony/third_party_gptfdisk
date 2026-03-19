/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef OHOS_FIXPARTS_H
#define OHOS_FIXPARTS_H

#include "ohos_mbr_helper.h"
#include <string>

// Command line arguments structure
struct OhosFixpartsArgs {
    std::string device;        // Device path
    bool printMBR;            // Print MBR partition table
    bool showHelp;            // Show help information
    int partitionNum;         // Partition number
    uint8_t typeCode;         // Type code

    OhosFixpartsArgs()
        : printMBR(false), showHelp(false), partitionNum(-1), typeCode(0) {}
};

// Main program class
class OhosFixparts {
public:
    OhosFixparts();
    ~OhosFixparts();

    // Parse command line arguments
    MbrResult ParseArgs(int argc, char* argv[], OhosFixpartsArgs& args);

    // Execute partition type code modification
    int Run(const OhosFixpartsArgs& args);

    // Show help information
    void ShowHelp(const char* programName);

private:
    OhosMbrHelper helper_;

    // Parse hexadecimal type code
    bool ParseTypeCode(const std::string& str, uint8_t& code);

    // Parse option string (format: partnum:typecode) and validate all parameters
    MbrResult ParseOption(const std::string& option, int& partitionNum, uint8_t& typeCode);
};

#endif // OHOS_FIXPARTS_H
//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include "utils/logger.h"

class Logger::LoggerImpl {
public:
    LoggerImpl() : mOutput(std::cout) {
    }
    std::ostream& mOutput;
};

Logger::Logger():
    mImpl(new LoggerImpl()) {
}

Logger::~Logger() = default;

Logger& Logger::Instance() {
    static Logger logger;
    return logger;
}

void Logger::Info(std::string source, std::string message, Verbosity) {
    mImpl->mOutput << "[REPORT_INFO] <" << source << "> :\t" << message << std::endl;
}

void Logger::Error(std::string source, std::string message) {
    mImpl->mOutput << "[REPORT_ERROR] <" << source << "> :\t" << message << std::endl;
}

std::string Logger::ToDecString(const uint64_t value)
{
    return(std::to_string(value));
}

std::string Logger::ToHexString(const uint64_t value)
{
    std::stringstream strStr;
    strStr << "0x" << std::setfill('0') << std::setw(16) << &std::uppercase << &std::hex << value;
    return(strStr.str());
}

std::string Logger::ToHexString(const uint16_t value, const uint16_t chars)
{
    std::stringstream strStr;
    strStr << "0x" << std::setfill('0') << std::setw(chars) << &std::uppercase << &std::hex << value;
    return(strStr.str());
}

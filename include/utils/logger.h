//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

#pragma once

#include <memory>
#include <ostream>

class Logger final {
    Logger();
    ~Logger();

    class LoggerImpl;
    std::unique_ptr<LoggerImpl> mImpl;
public:
    enum class Level : uint8_t {
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    enum class Verbosity : uint8_t {
        NONE,
        LOW,
        MEDIUM,
        HIGH
    };

    static Logger& Instance();

    void Info(std::string source, std::string message, Verbosity v = Verbosity::NONE);
    void Error(std::string source, std::string message);

    std::string ToHexString(const uint16_t value, const uint16_t chars);
    std::string ToHexString(const uint64_t value);
    std::string ToDecString(const uint64_t value);
};

#define REPORT_INFO(source, message, verbosity) \
    Logger::Instance().Info(source, message, Logger::Verbosity::NONE)

#define REPORT_ERROR(source, message) \
    Logger::Instance().Error(source, message)

#define REPORT_HEX_STR(...) \
    Logger::Instance().ToHexString(__VA_ARGS__)

#define REPORT_DEC_STR(value) \
    Logger::Instance().ToDecString(value)


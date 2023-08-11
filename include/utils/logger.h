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

	void Info(const char* source, const char* message, Verbosity v = Verbosity::NONE);
	void Error(const char* source, const char* message);
};

#define REPORT_INFO(x) \
	do { Logger::Instance().Info("bitCompactor", x, Logger::Verbosity::NONE); } while(false);

#define REPORT_ERROR(x) \
	do { Logger::Instance().Error("bitCompactor", x); } while (false);

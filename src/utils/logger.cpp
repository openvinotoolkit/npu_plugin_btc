//
// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache 2.0
//

//

#include <iostream>
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

void Logger::Info(const char* source, const char* message, Verbosity) {
	mImpl->mOutput << "[REPORT_INFO] <" << source << "> :\t" << message << std::endl;
}

void Logger::Error(const char* source, const char* message) {
	mImpl->mOutput << "[REPORT_ERROR] <" << source << "> :\t" << message << std::endl;
}


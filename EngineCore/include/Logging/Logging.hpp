#pragma once

#include <cstdio>
#include <vector>
#include <map>
//#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <memory>

#include "PlatformSemantics.hpp"

namespace Logging
{
	enum class LogLevel
	{
		Debug3 = 0,
		Debug2,
		Debug1,
		Info,
		Warning,
		Error,
		Exception
	};

	struct LoggerInternalMapping
	{
		LogLevel min_level;
		FILE* handle;
		bool hasStartedLogging;
		bool useColors;
	};

	// TODO: Thread unsafe!!!
	class CMEP_EXPORT Logger
	{
	private:
		std::vector<LoggerInternalMapping*> outputs;
		std::map<int16_t, std::string> threadid_name_map;

	public:
		Logger() {}
		~Logger() {};

		void AddOutputHandle(LogLevel min_level, FILE* handle, bool useColors = false);
		void MapCurrentThreadToName(std::string name);

		void StartLog(LogLevel level);
		void Log(const char* format, ...);
		void StopLog();

		void SimpleLog(LogLevel level, const char* format, ...);
	};
}
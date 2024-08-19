#pragma once

#include "PlatformSemantics.hpp"

#include <cstdio>
#include <memory>
#include <string>

namespace Logging
{
	enum class LogLevel : uint8_t
	{
		Debug3	  = 0,
		Debug2	  = 1,
		Debug1	  = 2,
		Info	  = 3,
		Warning	  = 4,
		Error	  = 5,
		Exception = 6
	};
} // namespace Logging

namespace Logging
{
	struct LoggerInternalState;
}

#ifdef ENGINELOGGING_LIBRARY_IMPLEMENTATION
#	include <map>
#	include <mutex>
#	include <vector>

namespace Logging
{
	struct LoggerInternalMapping
	{
		LogLevel min_level;
		FILE* handle;
		bool has_started_logging;
		bool use_colors;
	};

	struct LoggerInternalState
	{
		std::vector<LoggerInternalMapping> outputs;
		std::map<uint16_t, std::string> threadid_name_map;
		std::mutex thread_mutex;
	};

} // namespace Logging
#endif

namespace Logging
{
	class CMEP_EXPORT_CLASS Logger
	{
	private:
		LoggerInternalState* state;

	public:
		CMEP_EXPORT Logger();
		CMEP_EXPORT ~Logger();

		void CMEP_EXPORT AddOutputHandle(LogLevel min_level, FILE* handle, bool use_colors = false);
		void CMEP_EXPORT MapCurrentThreadToName(std::string name);

		void CMEP_EXPORT StartLog(LogLevel level);
		void CMEP_EXPORT Log(const char* format, ...);
		void CMEP_EXPORT StopLog();

		void CMEP_EXPORT SimpleLog(LogLevel level, const char* format, ...);
	};

	class SupportsLogging
	{
	public:
		using logger_t = std::shared_ptr<Logging::Logger>;

		SupportsLogging() = delete;
		SupportsLogging(logger_t with_logger) : logger(std::move(with_logger))
		{
		}

		[[nodiscard]] logger_t GetLogger()
		{
			return logger;
		}

	protected:
		logger_t logger;
	};

} // namespace Logging

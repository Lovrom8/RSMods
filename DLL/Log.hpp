#pragma once

#include "Windows.h"
#include <iostream>
#include <string>

/// If changing this file, be sure to do a clean build, else some changes might not go into effect.

#ifndef _LOG_INIT
#define _LOG_INIT Log LOG // Use log in place of cout or cerr, to help consolidate logging.
#endif;

#ifndef _LOG_HEAD
#define _LOG_HEAD LOG << LOG.GetHeader() // Use to start the log message, as it will prepend a timestamp and level of the log message.
#endif

/// <summary>
/// Classification of the logs level, whether the log is just a debug message or an error.
/// </summary>
enum class LogLevel
{
	Debug,
	Info,
	Warning,
	Error
};

namespace LogSettings
{
	/// <summary>
	/// The time that we have that we consider "boot time" of Rocksmith.
	/// All timestamps in the log are based off of this value.
	/// </summary>
	inline clock_t startupTime;

	/// <summary>
	/// Log level to default to, in-case we need to add more values in the future.
	/// </summary>
	inline LogLevel defaultLogLevel = LogLevel::Info;
}

struct Log
{
	enum LogLevel level = LogSettings::defaultLogLevel;

	/// <summary>
	/// Log a message to both cout (debug console) and cerr (log file).
	/// </summary>
	/// <returns>Pointer to ostream to support chaining.</returns>
	template <typename T>
	std::ostream& operator<<(const T message)
	{
		std::cout << message;
		std::cerr << message;

		return std::cout;
	}

	/// <summary>
	/// Replaces std::endl.
	/// </summary>
	std::string endl()
	{
		return "\r\n";
	}

	/// <summary>
	/// Get timestamp since boot, and log level label.
	/// </summary>
	std::string GetHeader()
	{
		std::string header;

		// Get the timestamp from starting the game to now.
		double timeSinceStart = (double)(clock() - LogSettings::startupTime) / CLOCKS_PER_SEC;
		header += std::to_string(timeSinceStart);
		
		// Remove trailing zeros.
		header.erase(header.find_last_not_of('0') + 1, std::string::npos);

		header += " ";

		// Append the log level
		switch (level)
		{
			case (LogLevel::Debug):
			{
				header += "[DEBUG]";
				break;
			}
			
			case (LogLevel::Info):
			{
				header += "[INFO]";
				break;
			}

			case (LogLevel::Warning):
			{
				header += "[WARNING]";
				break;
			}

			case (LogLevel::Error):
			{
				header += "[ERROR]";
				break;
			}

			default:
			{
				header += "[UNKNOWN]";
				break;
			}
		}

		header += " ";
		return header;
	}
};
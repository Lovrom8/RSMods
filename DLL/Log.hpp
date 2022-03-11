#pragma once

#include "Windows.h"
#include <iostream>
#include <string>

/// If changing this file, be sure to do a clean build, else some changes might not go into effect.

#ifdef _DEBUG
#ifndef _LOG_INIT
#define _LOG_INIT Log LOG; Log LOG_DEV; LOG_DEV.isConsole = true // Use log in place of cout or cerr, to help consolidate logging.
#endif

#ifndef _LOG_NOHEAD
#define _LOG_NOHEAD(X) LOG << X; LOG_DEV << X // Used to append to an existing log message. Will not add timestamp nor level of log.
#endif

#ifndef _LOG
#define _LOG(X) LOG << LOG.GetHeader() << X; LOG_DEV << LOG_DEV.GetHeader() << X // Log a specific message to the console (on debug builds) and the log file.
#endif

#ifndef _LOG_SETLEVEL
#define _LOG_SETLEVEL(X) LOG.level = X; LOG_DEV.level = X // Sets the logging level of both the file LOG and console LOG.
#endif
#else

#ifndef _LOG_INIT
#define _LOG_INIT Log LOG // Use log in place of cout or cerr, to help consolidate logging.
#endif

#ifndef _LOG_NOHEAD
#define _LOG_NOHEAD(X) try { LOG << X; } catch (const std::exception& e) { std::cerr << X; }// Used to append to an existing log message. Will not add timestamp nor level of log.
#endif

#ifndef _LOG
#define _LOG(X) try { LOG << LOG.GetHeader() << X; } catch ( const std::exception& e) { std::cerr << X; }  // Log a specific message to the log file.
#endif

#ifndef _LOG_SETLEVEL
#define _LOG_SETLEVEL(X) LOG.level = X // Sets the logging level of the file LOG.
#endif

#endif

/// <summary>
/// Classification of the logs level, whether the log is just a debug message or an error.
/// </summary>
enum class LogLevel : unsigned char
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

	bool isConsole = false;

	/// <summary>
	/// Log a message to both cout (debug console) and cerr (log file).
	/// </summary>
	/// <returns>Pointer to ostream to support chaining.</returns>
	template <typename T>
	std::ostream& operator<<(const T message)
	{
		if (isConsole)
		{
			std::cout << message;
			return std::cout;
		}
		else {
			std::cerr << message;
			return std::cerr;
		}
	}

	/// <summary>
	/// Get timestamp since boot, and log level label.
	/// </summary>
	std::string GetHeader()
	{
		std::string header;

		// Debug console doesn't need header information, as it will just cause a bunch of wasted space.
		if (isConsole)
		{
			return header;
		}

		// Get the timestamp from starting the game to now.
		double timeSinceStart = (double)(clock() - LogSettings::startupTime) / CLOCKS_PER_SEC;
		header += std::to_string(timeSinceStart);
		
		// Remove trailing zeros.
		header.erase(header.size() - 4, std::string::npos);

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
	};
};
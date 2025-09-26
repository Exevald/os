#pragma once

#include <string>

struct ProcessInfo
{
	pid_t pid = 0;
	std::string processName;
	std::string user;
	unsigned long long privateMemKb = 0;
	unsigned long long sharedMemKb = 0;
	int threads = 0;
	std::string cmdline;
	double cpuUsagePercent = 0.0;

	[[nodiscard]] unsigned long long GetTotalMemoryUsage() const
	{
		return privateMemKb + sharedMemKb;
	}
};
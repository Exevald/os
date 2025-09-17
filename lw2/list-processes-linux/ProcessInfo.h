#pragma once

#include <string>

struct ProcessInfo
{
	pid_t pid = 0;
	std::string comm;
	std::string user;
	unsigned long long privateMem = 0;
	unsigned long long sharedMem = 0;
	int threads = 0;
	std::string cmdline;
	double cpuPercent = 0.0;

	unsigned long long totalMem() const
	{
		return privateMem + sharedMem;
	}
};
#pragma once

#include "ProcessInfo.h"

#include <unordered_map>
#include <vector>

class CpuMonitor
{
public:
	static void MeasureCpuUsage(std::vector<ProcessInfo>& processes);

private:
	struct CpuJiffies
	{
		unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
		unsigned long long total() const
		{
			return user + nice + system + idle + iowait + irq + softirq + steal;
		}
	};

	static CpuJiffies ReadSystemCpuJiffies();
	static bool ReadProcessJiffies(pid_t pid, unsigned long long& utime, unsigned long long& stime);
};
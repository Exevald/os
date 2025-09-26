#include "CpuMonitor.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace
{
struct CpuJiffies
{
	unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
	[[nodiscard]] unsigned long long GetTotalCpuUsage() const
	{
		return user + nice + system + idle + iowait + irq + softirq + steal;
	}
};

CpuJiffies ReadSystemCpuJiffies()
{
	std::ifstream file("/proc/stat");
	if (!file.is_open())
	{
		return {};
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.find("cpu ") == 0)
		{
			CpuJiffies j;
			// вынести в stringstream
			sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
				&j.user, &j.nice, &j.system, &j.idle, &j.iowait, &j.irq, &j.softirq, &j.steal);
			return j;
		}
	}
	return {};
}

bool ReadProcessJiffies(pid_t pid, unsigned long long& utime, unsigned long long& stime)
{
	std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
	std::ifstream file(statPath);
	if (!file.is_open())
	{
		return false;
	}

	std::string content;
	std::getline(file, content);
	if (content.empty())
	{
		return false;
	}

	size_t openBracket = content.find('(');
	size_t closeBracket = content.rfind(')');
	if (openBracket == std::string::npos || closeBracket == std::string::npos)
	{
		return false;
	}

	std::string afterCloseBracket = content.substr(closeBracket + 2);
	std::istringstream iss(afterCloseBracket);
	std::vector<std::string> tokens;
	std::string token;
	while (iss >> token)
	{
		tokens.push_back(token);
	}

	if (tokens.size() < 14)
	{
		return false;
	}

	try
	{
		utime = std::stoull(tokens[12]);
		stime = std::stoull(tokens[13]);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
} // namespace

std::unordered_map<pid_t, double> CpuMonitor::GetCpuUsage(const std::vector<pid_t>& processesIds)
{
	std::unordered_map<pid_t, std::pair<unsigned long long, unsigned long long>> firstJiffies;
	const CpuJiffies firstSystemJiffies = ReadSystemCpuJiffies();

	for (auto& pid : processesIds)
	{
		unsigned long long stime;
		if (unsigned long long utime; ReadProcessJiffies(pid, utime, stime))
		{
			firstJiffies[pid] = { utime, stime };
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	const CpuJiffies secondSystemJiffies = ReadSystemCpuJiffies();
	unsigned long long systemDelta = secondSystemJiffies.GetTotalCpuUsage() - firstSystemJiffies.GetTotalCpuUsage();
	const auto cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
	if (systemDelta == 0)
	{
		systemDelta = 1;
	}

	std::unordered_map<pid_t, double> processCpuUsageMap;
	for (auto& pid : processesIds)
	{
		if (firstJiffies.contains(pid))
		{
			const unsigned long long utime1 = firstJiffies[pid].first;
			const unsigned long long stime1 = firstJiffies[pid].second;

			unsigned long long stime2;
			if (unsigned long long utime2; ReadProcessJiffies(pid, utime2, stime2))
			{
				const unsigned long long procDelta = (utime2 - utime1) + (stime2 - stime1);
				processCpuUsageMap[pid] = static_cast<double>(procDelta) * 100.0 * static_cast<double>(cpuCount) / static_cast<double>(systemDelta);
			}
		}
	}

	return processCpuUsageMap;
}
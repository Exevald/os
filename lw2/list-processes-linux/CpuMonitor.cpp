#include "CpuMonitor.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

CpuMonitor::CpuJiffies CpuMonitor::ReadSystemCpuJiffies()
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
			sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
				&j.user, &j.nice, &j.system, &j.idle, &j.iowait, &j.irq, &j.softirq, &j.steal);
			return j;
		}
	}
	return {};
}

bool CpuMonitor::ReadProcessJiffies(pid_t pid, unsigned long long& utime, unsigned long long& stime)
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

	size_t openParen = content.find('(');
	size_t closeParen = content.rfind(')');
	if (openParen == std::string::npos || closeParen == std::string::npos)
	{
		return false;
	}

	std::string afterParen = content.substr(closeParen + 2);
	std::istringstream iss(afterParen);
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

void CpuMonitor::MeasureCpuUsage(std::vector<ProcessInfo>& processes)
{
	std::unordered_map<pid_t, std::pair<unsigned long long, unsigned long long>> firstJiffies;
	const CpuJiffies firstSystemJiffies = ReadSystemCpuJiffies();

	for (auto& proc : processes)
	{
		unsigned long long stime;
		if (unsigned long long utime; ReadProcessJiffies(proc.pid, utime, stime))
		{
			firstJiffies[proc.pid] = { utime, stime };
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));

	const CpuJiffies secondSystemJiffies = ReadSystemCpuJiffies();
	unsigned long long systemDelta = secondSystemJiffies.total() - firstSystemJiffies.total();
	const int cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
	if (systemDelta == 0)
	{
		systemDelta = 1;
	}

	for (auto& proc : processes)
	{
		if (firstJiffies.contains(proc.pid))
		{
			const unsigned long long utime1 = firstJiffies[proc.pid].first;
			const unsigned long long stime1 = firstJiffies[proc.pid].second;

			unsigned long long stime2;
			if (unsigned long long utime2; ReadProcessJiffies(proc.pid, utime2, stime2))
			{
				const unsigned long long procDelta = (utime2 - utime1) + (stime2 - stime1);
				proc.cpuPercent = static_cast<double>(procDelta) * 100.0 * cpuCount / static_cast<double>(systemDelta);
			}
		}
	}
}
#include "ProcessPresenter.h"

#include <iomanip>
#include <iostream>

namespace
{
std::string FormatSize(unsigned long long sizeKib)
{
	if (sizeKib < 1024)
	{
		return std::to_string(sizeKib) + " KiB";
	}
	const double mib = static_cast<double>(sizeKib) / 1024.0;
	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << mib << " MiB";
	return ss.str();
}
} // namespace

void ProcessPresenter::PrintProcessTable(const std::vector<ProcessInfo>& processes)
{
	std::cout << std::left
			  << std::setw(8) << "PID"
			  << std::setw(45) << "COMM"
			  << std::setw(20) << "USER"
			  << std::setw(12) << "PRIVATE"
			  << std::setw(12) << "SHARED"
			  << std::setw(8) << "%CPU"
			  << std::setw(8) << "THREADS"
			  << "CMD" << std::endl;

	for (const auto& proc : processes)
	{
		std::cout << std::setw(8) << proc.pid
				  << std::setw(45) << proc.processName
				  << std::setw(20) << proc.user
				  << std::setw(12) << FormatSize(proc.privateMemKb)
				  << std::setw(12) << FormatSize(proc.sharedMemKb)
				  << std::setw(8) << std::fixed << std::setprecision(1) << proc.cpuUsagePercent
				  << std::setw(8) << proc.threads
				  << proc.cmdline << std::endl;
	}
}

void ProcessPresenter::PrintSummary(const std::vector<ProcessInfo>& processes)
{
	unsigned long long totalPrivate = 0, totalShared = 0;
	const std::size_t totalCount = processes.size();

	for (const auto& proc : processes)
	{
		totalPrivate += proc.privateMemKb;
		totalShared += proc.sharedMemKb;
	}

	std::cout << std::endl
			  << "All processes count: " << totalCount << std::endl;
	std::cout << "PRIVATE memory usage: " << FormatSize(totalPrivate) << std::endl;
	std::cout << "SHARED memory usage: " << FormatSize(totalShared) << std::endl;
	std::cout << "TOTAL memory usage: " << FormatSize(totalPrivate + totalShared) << std::endl;
}
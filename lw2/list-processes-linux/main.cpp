#include "CpuMonitor.h"
#include "ProcessScanner.h"

#include <algorithm>

int main()
{
	std::vector<ProcessInfo> processes = ProcessScanner::CollectAllProcesses();
	CpuMonitor::MeasureCpuUsage(processes);

	std::ranges::sort(processes, [](const ProcessInfo& a, const ProcessInfo& b) {
		return a.totalMem() > b.totalMem();
	});

	ProcessScanner::PrintProcessTable(processes);
	ProcessScanner::PrintSummary(processes);

	return 0;
}
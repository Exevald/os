#include "CpuMonitor.h"
#include "ProcessPresenter.h"
#include "ProcessScanner.h"

#include <algorithm>

int main()
{
	std::vector<ProcessInfo> processes = ProcessScanner::CollectAllProcesses();
	std::vector<pid_t> processesIds;
	processesIds.reserve(processes.size());
	std::ranges::transform(processes, std::back_inserter(processesIds),
		[](const ProcessInfo& p) { return p.pid; });

	auto processCpuUsageMap = CpuMonitor::GetCpuUsage(processesIds);
	for (auto& proc : processes)
	{
		if (processCpuUsageMap.contains(proc.pid))
		{
			proc.cpuUsagePercent = processCpuUsageMap[proc.pid];
		}
		else
		{
			proc.cpuUsagePercent = 0.0;
		}
	}

	std::ranges::sort(processes, [](const ProcessInfo& a, const ProcessInfo& b) {
		return a.GetTotalMemoryUsage() > b.GetTotalMemoryUsage();
	});

	ProcessPresenter::PrintProcessTable(processes);
	ProcessPresenter::PrintSummary(processes);

	return 0;
}
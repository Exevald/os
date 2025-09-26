#pragma once

#include "ProcessInfo.h"

#include <vector>

namespace ProcessScanner
{

std::vector<pid_t> GetAllProcessesIds();
ProcessInfo GetProcessInfo(pid_t pid);
std::vector<ProcessInfo> CollectAllProcesses();
}; // namespace ProcessScanner
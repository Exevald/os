#pragma once

#include <unordered_map>
#include <vector>

namespace CpuMonitor
{
std::unordered_map<pid_t, double> GetCpuUsage(const std::vector<pid_t>& processesIds);
};
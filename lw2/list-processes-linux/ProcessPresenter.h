#pragma once
#include "ProcessInfo.h"

#include <vector>

namespace ProcessPresenter
{
void PrintProcessTable(const std::vector<ProcessInfo>& processes);
void PrintSummary(const std::vector<ProcessInfo>& processes);
} // namespace ProcessPresenter
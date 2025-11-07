#pragma once

#include "OSHandler.h"

#include <functional>
#include <vector>

class MockOSHandler final : public OSHandler
{
public:
	int callCount = 0;
	std::vector<std::tuple<uint32_t, Access, PageFaultReason>> calls;
	std::function<bool(VirtualMemory& vm, uint32_t virtualPageNumber, Access access, PageFaultReason reason)> nextAction;

	MockOSHandler()
	{
		nextAction = [](VirtualMemory&, uint32_t, Access, PageFaultReason) { return false; };
	}

	bool OnPageFault(VirtualMemory& vm, uint32_t virtualPageNumber, Access access, PageFaultReason reason) override
	{
		callCount++;
		calls.emplace_back(virtualPageNumber, access, reason);
		return nextAction(vm, virtualPageNumber, access, reason);
	}

	void SetReturn(bool value)
	{
		nextAction = [value](VirtualMemory&, uint32_t, Access, PageFaultReason) { return value; };
	}

	void Reset()
	{
		callCount = 0;
		calls.clear();
		SetReturn(false);
	}

	void OnSuccessfulAccess(uint32_t virtualPageNumber, Access access) override
	{

	}
};

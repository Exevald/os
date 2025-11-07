#pragma once

#include "OSHandler.h"
#include "PhysicalMemory.h"
#include "SwapManager.h"

#include <list>
#include <map>

struct ReferenceEntry
{
	uint32_t vpn;
	Access access;
	uint64_t timestamp;
};

struct PageInfo
{
	uint32_t virtualPageNumber = 0;
	uint32_t physicalFrameNumber = 0;
	bool isPresent = false;
	uint32_t swapSlot = 0;
	uint32_t loadTime = 0;
};

class MyOS final : public OSHandler
{
public:
	MyOS(PhysicalMemory& pm, SwapManager& sm, VMSupervisorContext& context);

	bool OnPageFault(VirtualMemory& vm, uint32_t vpn, Access access, PageFaultReason reason) override;
	void OnSuccessfulAccess(uint32_t virtualPageNumber, Access access) override;

	[[nodiscard]] uint32_t GetPageFaultCount() const;

private:
	uint32_t WSClock_SelectVictimFrame();
	void WSClock_AdvanceClock();

	uint32_t AllocateFreeFrame();
	void EvictPage(uint32_t vpn, uint32_t pfn);
	void LoadPage(uint32_t vpn, uint32_t pfn);

	PhysicalMemory& m_physicalMemory;
	SwapManager& m_swapManager;
	VMSupervisorContext& m_supervisorContext;

	std::map<uint32_t, PageInfo> m_pageTableInfo;
	std::vector<uint32_t> m_freeFrames;
	std::vector<uint32_t> m_frameToVpn;

	std::list<uint32_t> m_wsClockList;
	std::list<uint32_t>::iterator m_clockHand;
	static constexpr uint32_t WORKING_SET_LIMIT = 5000;

	uint32_t m_pageFaults = 0;
	uint32_t m_swapWrites = 0;
	uint32_t m_swapReads = 0;
	uint64_t m_timestamp = 0;
	bool m_inPageFaultHandler = false;

	std::vector<ReferenceEntry> m_referenceString;
};
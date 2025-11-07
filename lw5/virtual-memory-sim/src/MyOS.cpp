#include "MyOS.h"

MyOS::MyOS(PhysicalMemory& pm, SwapManager& sm, VMSupervisorContext& context)
	: m_physicalMemory(pm)
	, m_swapManager(sm)
	, m_supervisorContext(context)
{
}

bool MyOS::OnPageFault(VirtualMemory& vm, uint32_t vpn, Access access, PageFaultReason reason)
{
	m_pageFaults++;

	if (m_inPageFaultHandler)
	{
		throw std::runtime_error("Kernel Panic: Double Fault detected!");
	}
	m_inPageFaultHandler = true;

	try
	{
		if (reason != PageFaultReason::NotPresent)
		{
			m_inPageFaultHandler = false;
			throw std::runtime_error("Access violation or misaligned access!");
		}

		uint32_t pfn;
		if (!m_freeFrames.empty())
		{
			pfn = AllocateFreeFrame();
		}
		else
		{
			pfn = WSClock_SelectVictimFrame();

			const uint32_t victimVpn = m_frameToVpn[pfn];
			EvictPage(victimVpn, pfn);
		}

		LoadPage(vpn, pfn);
		VMSupervisorContext& supervisor = m_supervisorContext;
		const uint32_t pteAddress = supervisor.GetPageTableAddress() + (vpn * sizeof(uint32_t));

		PTE pte;
		try
		{
			pte.raw = supervisor.Read32(pteAddress);
		}
		catch (...)
		{
		}

		pte.SetFrame(pfn);
		pte.SetPresent(true);
		pte.SetAccessed(false);
		pte.SetDirty(false);
		pte.SetUser(true);
		pte.SetWritable(true);
		pte.SetNX(false);

		supervisor.Write32(pteAddress, pte.raw);

		PageInfo& newPageInfo = m_pageTableInfo[vpn];
		newPageInfo.virtualPageNumber = vpn;
		newPageInfo.physicalFrameNumber = pfn;
		newPageInfo.isPresent = true;
		newPageInfo.loadTime = m_timestamp;
		m_frameToVpn[pfn] = vpn;
	}
	catch (const std::exception&)
	{
		m_inPageFaultHandler = false;
		throw;
	}

	m_inPageFaultHandler = false;
	return true;
}

void MyOS::OnSuccessfulAccess(uint32_t virtualPageNumber, Access access)
{
}

uint32_t MyOS::GetPageFaultCount() const
{
	return m_pageFaults;
}

uint32_t MyOS::WSClock_SelectVictimFrame()
{
	while (true)
	{
		const uint32_t pfn = *m_clockHand;
		uint32_t vpn = m_frameToVpn[pfn];

		VMSupervisorContext& supervisor = m_supervisorContext;
		const uint32_t pteAddress = supervisor.GetPageTableAddress() + (vpn * sizeof(uint32_t));
		PTE pte;
		pte.raw = supervisor.Read32(pteAddress);

		PageInfo& pageInfo = m_pageTableInfo[vpn];
		const uint64_t age = m_timestamp - pageInfo.loadTime;

		if (pte.IsAccessed())
		{
			pte.SetAccessed(false);
			supervisor.Write32(pteAddress, pte.raw);
			pageInfo.loadTime = m_timestamp;
		}
		else
		{
			if (age > WORKING_SET_LIMIT)
			{
				WSClock_AdvanceClock();
				return pfn;
			}
		}

		WSClock_AdvanceClock();
	}
}

void MyOS::WSClock_AdvanceClock()
{
	++m_clockHand;
	if (m_clockHand == m_wsClockList.end())
	{
		m_clockHand = m_wsClockList.begin();
	}
}

uint32_t MyOS::AllocateFreeFrame()
{
	return 0;
}

void MyOS::EvictPage(uint32_t vpn, uint32_t pfn)
{
	const VMSupervisorContext& supervisor = m_supervisorContext;
	const uint32_t pteAddress = supervisor.GetPageTableAddress() + (vpn * sizeof(uint32_t));
	PTE pte;
	pte.raw = supervisor.Read32(pteAddress);

	if (pte.IsDirty())
	{
		if (m_pageTableInfo[vpn].swapSlot == 0)
		{
			m_pageTableInfo[vpn].swapSlot = m_swapManager.AllocateSlot(vpn);
		}
		m_swapManager.WritePage(m_pageTableInfo[vpn].swapSlot, pfn);
		m_swapWrites++;
	}

	pte.SetPresent(false);
	pte.SetDirty(false);
	pte.SetAccessed(false);
	supervisor.Write32(pteAddress, pte.raw);

	m_pageTableInfo[vpn].isPresent = false;
}

void MyOS::LoadPage(uint32_t vpn, uint32_t pfn)
{
	if (m_pageTableInfo.contains(vpn) && m_pageTableInfo[vpn].swapSlot != 0)
	{
		m_swapManager.ReadPage(m_pageTableInfo[vpn].swapSlot, pfn);
		m_swapReads++;
	}
	else
	{
		const uint32_t physicalAddress = pfn * 4096;
		for (uint32_t i = 0; i < 4096 / sizeof(uint32_t); ++i)
		{
			m_physicalMemory.Write32(physicalAddress + i * sizeof(uint32_t), 0);
		}

		if (m_pageTableInfo[vpn].swapSlot == 0)
		{
			m_pageTableInfo[vpn].swapSlot = m_swapManager.AllocateSlot(vpn);
		}
	}
}
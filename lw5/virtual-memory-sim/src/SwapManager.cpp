#include "SwapManager.h"

#include <fstream>
#include <utility>

SwapManager::SwapManager(VMSupervisorContext& vmContext, std::filesystem::path  path)
	: m_vmContext(vmContext)
	, m_path(std::move(path))
{
}

uint32_t SwapManager::AllocateSlot(uint32_t virtualPageNumber)
{
	uint32_t slotId;
	if (!m_freeSlots.empty())
	{
		slotId = m_freeSlots.back();
		m_freeSlots.pop_back();
	}
	else
	{
		slotId = m_nextSlotId++;
	}

	m_vpnToSlot[virtualPageNumber] = slotId;
	m_slotToVpn[slotId] = virtualPageNumber;
	return slotId;
}

void SwapManager::FreeSlot(uint32_t virtualPageNumber)
{
	if (m_vpnToSlot.contains(virtualPageNumber))
	{
		const uint32_t slotId = m_vpnToSlot.at(virtualPageNumber);
		m_vpnToSlot.erase(virtualPageNumber);
		m_slotToVpn.erase(slotId);
		m_freeSlots.push_back(slotId);
	}
}

void SwapManager::ReadPage(uint32_t slot, uint32_t virtualPageNumber)
{
}

void SwapManager::WritePage(uint32_t slot, uint32_t virtualPageNumber)
{
}

std::fstream SwapManager::OpenSwapFile(std::ios_base::openmode mode) const
{
	std::fstream fs(m_path, mode | std::ios::binary);
	if (!fs.is_open())
	{
		throw std::runtime_error("Failed to open swap file: " + m_path.string());
	}
	return fs;
}
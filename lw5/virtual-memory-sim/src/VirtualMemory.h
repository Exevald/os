#pragma once

#include "OSHandler.h"
#include "PhysicalMemory.h"

#include <stdexcept>

class PageFault final : public std::runtime_error
{
public:
	const uint32_t virtualPageNumber;
	const Access access;
	const PageFaultReason reason;
	PageFault(uint32_t vpN, Access acc, PageFaultReason r)
		: runtime_error("Page Fault occurred")
		, virtualPageNumber(vpN)
		, access(acc)
		, reason(r)
	{
	}
};

class VirtualMemory
{
public:
	explicit VirtualMemory(PhysicalMemory& physicalMem, OSHandler& handler);

	void SetPageTableAddress(uint32_t physicalAddress);
	[[nodiscard]] uint32_t GetPageTableAddress() const noexcept;

	uint8_t Read8(uint32_t address, Privilege privilege, bool execute = false);
	uint16_t Read16(uint32_t address, Privilege privilege, bool execute = false);
	uint32_t Read32(uint32_t address, Privilege privilege, bool execute = false);
	uint64_t Read64(uint32_t address, Privilege privilege, bool execute = false);

	void Write8(uint32_t address, uint8_t value, Privilege privilege);
	void Write16(uint32_t address, uint16_t value, Privilege privilege);
	void Write32(uint32_t address, uint32_t value, Privilege privilege);
	void Write64(uint32_t address, uint64_t value, Privilege privilege);

private:
	[[nodiscard]] uint32_t TranslateAndCheck(uint32_t virtualAddress, Access access, Privilege privilege, bool needToExecute) const;

	template <typename T>
	T AccessMemory(uint32_t virtualAddress, Privilege privilege, Access access, bool execute = false, T value = T{});

	static constexpr uint32_t PAGE_SIZE = 4096;
	static constexpr uint32_t VADDR_PAGENUM_SHIFT = 12;
	static constexpr uint32_t VADDR_OFFSET_MASK = 0x00000FFF;
	static constexpr uint32_t VADDR_VPN_MASK = 0xFFFFF000;

	PhysicalMemory& m_physicalMemory;
	OSHandler& m_osHandler;
	uint32_t m_pageTableAddress = 0;
};

template <typename T>
T VirtualMemory::AccessMemory(uint32_t virtualAddress, Privilege privilege, Access access, bool execute, T value)
{
	for (int retry = 0; retry < 2; ++retry)
	{
		try
		{
			if (virtualAddress % sizeof(T) != 0)
			{
				const uint32_t virtualPageNumber = (virtualAddress & VADDR_VPN_MASK) >> VADDR_PAGENUM_SHIFT;
				throw PageFault(virtualPageNumber, access, PageFaultReason::MisalignedAccess);
			}

			const uint32_t address = TranslateAndCheck(virtualAddress, access, privilege, execute);
			switch (access)
			{
			case Access::Read: {
				if constexpr (sizeof(T) == 1)
				{
					return static_cast<T>(m_physicalMemory.Read8(address));
				}
				if constexpr (sizeof(T) == 2)
				{
					return static_cast<T>(m_physicalMemory.Read16(address));
				}
				if constexpr (sizeof(T) == 4)
				{
					return static_cast<T>(m_physicalMemory.Read32(address));
				}
				if constexpr (sizeof(T) == 8)
				{
					return static_cast<T>(m_physicalMemory.Read64(address));
				}
				break;
			}
			case Access::Write: {
				if constexpr (sizeof(T) == 1)
				{
					m_physicalMemory.Write8(address, static_cast<uint8_t>(value));
				}
				if constexpr (sizeof(T) == 2)
				{
					m_physicalMemory.Write16(address, static_cast<uint16_t>(value));
				}
				if constexpr (sizeof(T) == 4)
				{
					m_physicalMemory.Write32(address, static_cast<uint32_t>(value));
				}
				if constexpr (sizeof(T) == 8)
				{
					m_physicalMemory.Write64(address, static_cast<uint64_t>(value));
				}
				break;
			}
			}
			return value;
		}
		catch (const PageFault& pf)
		{
			if (m_osHandler.OnPageFault(*this, pf.virtualPageNumber, pf.access, pf.reason))
			{
				if (retry == 0)
				{
					continue;
				}
			}
			throw;
		}
	}
	throw std::runtime_error("Failed after two Page Fault retries");
}
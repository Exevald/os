#include "PhysicalMemory.h"

#include <stdexcept>

PhysicalMemory::PhysicalMemory(PhysicalMemoryConfig cfg)
	: m_size(cfg.numFrames * cfg.frameSize)
{
	m_data.resize(m_size, 0);
}

uint32_t PhysicalMemory::GetSize() const noexcept
{
	return m_size;
}

uint8_t PhysicalMemory::Read8(uint32_t address) const
{
	CheckBoundsAndAlignment(address, 1);
	return m_data[address];
}

uint16_t PhysicalMemory::Read16(uint32_t address) const
{
	CheckBoundsAndAlignment(address, 2);
	return *reinterpret_cast<const uint16_t*>(&m_data[address]);
}

uint32_t PhysicalMemory::Read32(uint32_t address) const
{
	CheckBoundsAndAlignment(address, 4);
	return *reinterpret_cast<const uint32_t*>(&m_data[address]);
}

uint64_t PhysicalMemory::Read64(uint32_t address) const
{
	CheckBoundsAndAlignment(address, 8);
	return *reinterpret_cast<const uint64_t*>(&m_data[address]);
}

void PhysicalMemory::Write8(uint32_t address, uint8_t value)
{
	CheckBoundsAndAlignment(address, 1);
	m_data[address] = value;
}

void PhysicalMemory::Write16(uint32_t address, uint16_t value)
{
	CheckBoundsAndAlignment(address, 2);
	*reinterpret_cast<uint16_t*>(&m_data[address]) = value;
}

void PhysicalMemory::Write32(uint32_t address, uint32_t value)
{
	CheckBoundsAndAlignment(address, 4);
	*reinterpret_cast<uint32_t*>(&m_data[address]) = value;
}

void PhysicalMemory::Write64(uint32_t address, uint64_t value)
{
	CheckBoundsAndAlignment(address, 8);
	*reinterpret_cast<uint64_t*>(&m_data[address]) = value;
}

void PhysicalMemory::CheckBoundsAndAlignment(uint32_t address, size_t size) const
{
	if (address + size > m_size)
	{
		throw std::out_of_range("Physical access out of range");
	}
	if (address % size != 0)
	{
		throw std::invalid_argument("Misaligned access");
	}
}

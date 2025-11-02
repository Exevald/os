#pragma once

#include <cstdint>
#include <vector>

struct PhysicalMemoryConfig
{
	uint32_t numFrames = 1024;
	uint32_t frameSize = 4096;
};

class PhysicalMemory
{
public:
	explicit PhysicalMemory(PhysicalMemoryConfig cfg);

	[[nodiscard]] uint32_t GetSize() const noexcept;

	[[nodiscard]] uint8_t Read8(uint32_t address) const;
	[[nodiscard]] uint16_t Read16(uint32_t address) const;
	[[nodiscard]] uint32_t Read32(uint32_t address) const;
	[[nodiscard]] uint64_t Read64(uint32_t address) const;

	void Write8(uint32_t address, uint8_t value);
	void Write16(uint32_t address, uint16_t value);
	void Write32(uint32_t address, uint32_t value);
	void Write64(uint32_t address, uint64_t value);

private:
	void CheckBoundsAndAlignment(uint32_t address, size_t size) const;

	std::vector<uint8_t> m_data;
	const uint32_t m_size;
};

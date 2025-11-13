#pragma once

#include <cassert>
#include <list>
#include <memory>
#include <shared_mutex>
#include <utility>

struct MemoryBlock
{
	size_t size;
	uintptr_t startAddress;

	explicit MemoryBlock(uintptr_t start, size_t size)
		: startAddress(start)
		, size(size)
	{
	}
};

class MemoryManager
{
public:
	explicit MemoryManager(void* start, size_t size) noexcept;

	MemoryManager(const MemoryManager&) = delete;
	MemoryManager& operator=(const MemoryManager&) = delete;

	void* Allocate(size_t size, size_t align = sizeof(std::max_align_t)) noexcept;

	void Free(void* address) noexcept;

private:
	mutable std::shared_mutex m_managerMutex;
	void* m_memoryStartAddress;
	size_t m_size;
	std::list<MemoryBlock> m_data;
};
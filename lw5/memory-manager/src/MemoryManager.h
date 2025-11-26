#pragma once

#include <cassert>
#include <cstddef>
#include <shared_mutex>

class MemoryManager
{
public:
	struct BlockHeader
	{
		BlockHeader* prev;
		BlockHeader* next;
		size_t size;

		static constexpr size_t FLAG_FREE = 1;

		[[nodiscard]] size_t GetSize() const;
		[[nodiscard]] bool IsFree() const;
		void SetSize(size_t newSize, bool isFree);
		static BlockHeader* GetHeaderFromDataPtr(void* dataPtr);
	};

	explicit MemoryManager(void* start, size_t size) noexcept;

	MemoryManager(const MemoryManager&) = delete;
	MemoryManager& operator=(const MemoryManager&) = delete;

	void* Allocate(size_t size, size_t align = sizeof(std::max_align_t)) noexcept;
	void Free(void* address) noexcept;

private:
	void RemoveFromFreeList(BlockHeader* block);
	void AddToFreeList(BlockHeader* block);
	void Coalesce(BlockHeader* block);

	std::shared_mutex m_managerMutex;
	void* m_memoryStartAddress;
	size_t m_size;
	BlockHeader* m_freeBlocks;
};
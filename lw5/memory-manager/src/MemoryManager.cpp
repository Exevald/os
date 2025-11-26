#include "MemoryManager.h"

#include <algorithm>

MemoryManager::MemoryManager(void* start, size_t size) noexcept
	: m_size(size)
	, m_memoryStartAddress(start)
	, m_freeBlocks(nullptr)
{
	if (!start || size == 0)
	{
		return;
	}

	auto startAddr = reinterpret_cast<uintptr_t>(start);
	uintptr_t alignedStart = (startAddr + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
	size_t alignedSize = size - (alignedStart - startAddr);

	if (alignedSize < sizeof(BlockHeader))
	{
		return;
	}
	if (alignedSize & 1)
	{
		alignedSize--;
	}

	auto* first = reinterpret_cast<BlockHeader*>(alignedStart);
	first->prev = nullptr;
	first->next = nullptr;
	first->SetSize(alignedSize, true);

	m_freeBlocks = first;
}

void* MemoryManager::Allocate(size_t size, size_t align) noexcept
{
	if (align == 0 || (align & (align - 1)) != 0)
	{
		return nullptr;
	}
	align = std::max(align, alignof(std::max_align_t));

	std::unique_lock lock(m_managerMutex);
	BlockHeader* current = m_freeBlocks;
	while (current != nullptr)
	{
		uintptr_t headerEnd = reinterpret_cast<uintptr_t>(current) + sizeof(BlockHeader);
		uintptr_t alignedData = (headerEnd + align - 1) & ~(align - 1);
		size_t payloadOffset = alignedData - reinterpret_cast<uintptr_t>(current);
		size_t totalRequired = payloadOffset + size;

		if (totalRequired & 1)
		{
			totalRequired++;
		}

		if (current->GetSize() >= totalRequired)
		{
			BlockHeader* allocatedBlock = current;
			RemoveFromFreeList(allocatedBlock);

			void* dataPtr = reinterpret_cast<void*>(alignedData);
			size_t usedSize = totalRequired;

			if (usedSize < allocatedBlock->GetSize())
			{
				auto* newFreeBlock = reinterpret_cast<BlockHeader*>(reinterpret_cast<uintptr_t>(allocatedBlock) + usedSize);
				newFreeBlock->prev = nullptr;
				newFreeBlock->next = nullptr;
				size_t newFreeSize = allocatedBlock->GetSize() - usedSize;
				newFreeBlock->SetSize(newFreeSize, true);
				AddToFreeList(newFreeBlock);
			}

			allocatedBlock->SetSize(usedSize, false);
			return dataPtr;
		}

		current = current->next;
	}

	return nullptr;
}

void MemoryManager::Free(void* address) noexcept
{
	if (!address)
	{
		return;
	}

	std::unique_lock lock(m_managerMutex);

	BlockHeader* freedBlock = BlockHeader::GetHeaderFromDataPtr(address);
	freedBlock->SetSize(freedBlock->GetSize(), true);

	freedBlock->prev = nullptr;
	freedBlock->next = nullptr;
	AddToFreeList(freedBlock);

	Coalesce(freedBlock);
}

void MemoryManager::RemoveFromFreeList(BlockHeader* block)
{
	if (block->prev)
	{
		block->prev->next = block->next;
	}
	else
	{
		m_freeBlocks = block->next;
	}

	if (block->next)
	{
		block->next->prev = block->prev;
	}
}

void MemoryManager::AddToFreeList(BlockHeader* block)
{
	block->next = m_freeBlocks;
	block->prev = nullptr;
	if (m_freeBlocks)
	{
		m_freeBlocks->prev = block;
	}
	m_freeBlocks = block;
}

void MemoryManager::Coalesce(BlockHeader* block)
{
	uintptr_t blockEnd = reinterpret_cast<uintptr_t>(block) + block->GetSize();
	uintptr_t memEnd = reinterpret_cast<uintptr_t>(m_memoryStartAddress) + m_size;

	if (blockEnd < memEnd)
	{
		if (blockEnd % alignof(BlockHeader) == 0)
		{
			auto* next = reinterpret_cast<BlockHeader*>(blockEnd);
			if (next->IsFree())
			{
				RemoveFromFreeList(next);
				block->SetSize(block->GetSize() + next->GetSize(), true);
			}
		}
	}

	BlockHeader* prev = nullptr;
	BlockHeader* cur = m_freeBlocks;
	while (cur != nullptr)
	{
		if (cur != block)
		{
			uintptr_t curEnd = reinterpret_cast<uintptr_t>(cur) + cur->GetSize();
			if (curEnd == reinterpret_cast<uintptr_t>(block))
			{
				prev = cur;
				break;
			}
		}
		cur = cur->next;
	}

	if (prev && prev->IsFree())
	{
		RemoveFromFreeList(block);
		prev->SetSize(prev->GetSize() + block->GetSize(), true);
	}
}

size_t MemoryManager::BlockHeader::GetSize() const
{
	return size & ~FLAG_FREE;
}

bool MemoryManager::BlockHeader::IsFree() const
{
	return (size & FLAG_FREE) != 0;
}

void MemoryManager::BlockHeader::SetSize(size_t newSize, bool isFree)
{
	assert((newSize & FLAG_FREE) == 0);
	size = newSize | (isFree ? FLAG_FREE : 0);
}

MemoryManager::BlockHeader* MemoryManager::BlockHeader::GetHeaderFromDataPtr(void* dataPtr)
{
	return reinterpret_cast<BlockHeader*>(reinterpret_cast<uintptr_t>(dataPtr) - sizeof(BlockHeader));
}
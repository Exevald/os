#include "MemoryManager.h"

MemoryManager::MemoryManager(void* start, size_t size) noexcept
	: m_size(size)
	, m_memoryStartAddress(start)
{
	if (!start || size == 0)
	{
		return;
	}

	auto startAddress = reinterpret_cast<uintptr_t>(start);
	uintptr_t alignedStart = (startAddress + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
	size_t alignedSize = size - (alignedStart - startAddress);

	if (alignedSize >= sizeof(size_t) + 1)
	{
		m_data.emplace_back(alignedStart, alignedSize);
	}
}

void* MemoryManager::Allocate(size_t size, size_t align) noexcept
{
	if (align == 0 || (align & (align - 1)) != 0)
	{
		return nullptr;
	}
	align = std::max(align, alignof(std::max_align_t));

	std::unique_lock lock(m_managerMutex);

	size_t requiredSize = sizeof(size_t) + align - 1 + size;

	for (auto it = m_data.begin(); it != m_data.end(); ++it)
	{
		auto& block = *it;
		if (block.size >= requiredSize)
		{
			uintptr_t blockStart = block.startAddress;
			uintptr_t blockEnd = blockStart + block.size;
			uintptr_t headerPos = blockStart;
			uintptr_t dataAreaStart = headerPos + sizeof(size_t);
			uintptr_t alignedStart = (dataAreaStart + align - 1) & ~(align - 1);
			uintptr_t endOfUserData = alignedStart + size;

			if (endOfUserData <= blockEnd)
			{
				size_t totalUsedSize = endOfUserData - blockStart;
				*reinterpret_cast<size_t*>(headerPos) = totalUsedSize;

				if (totalUsedSize < block.size)
				{
					uintptr_t remainingStart = blockStart + totalUsedSize;
					size_t remainingSize = block.size - totalUsedSize;

					m_data.emplace_back(remainingStart, remainingSize);
				}

				m_data.erase(it);
				return reinterpret_cast<void*>(alignedStart);
			}
		}
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

	auto convertedAddress = reinterpret_cast<uintptr_t>(address);
	uintptr_t headerPos = convertedAddress - sizeof(size_t);
	size_t totalBlockSize = *reinterpret_cast<size_t*>(headerPos);

	MemoryBlock newFreeBlock(headerPos, totalBlockSize);

	auto insertionPos = std::upper_bound(m_data.begin(), m_data.end(), newFreeBlock,
		[](const MemoryBlock& a, const MemoryBlock& b) {
			return a.startAddress < b.startAddress;
		});
	auto inserted = m_data.insert(insertionPos, newFreeBlock);

	if (inserted != m_data.begin())
	{
		auto previous = std::prev(inserted);
		uintptr_t previousEndAddress = previous->startAddress + previous->size;
		if (previousEndAddress == inserted->startAddress)
		{
			previous->size += inserted->size;
			inserted = m_data.erase(inserted);
			if (inserted == m_data.end())
			{
				return;
			}
		}
		else
		{
			++inserted;
		}
	}
	else
	{
		++inserted;
	}

	if (inserted != m_data.end())
	{
		auto current = std::prev(inserted);
		uintptr_t currentEndAddress = current->startAddress + current->size;
		if (inserted != m_data.end() && currentEndAddress == inserted->startAddress)
		{
			current->size += inserted->size;
			m_data.erase(inserted);
		}
	}
}

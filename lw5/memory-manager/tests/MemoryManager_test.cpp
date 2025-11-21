#include "MemoryManager.h"

#include <gtest/gtest.h>
#include <random>
#include <thread>

class MemoryManagerTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		buffer_size = 1024;
		buffer = std::make_unique<char[]>(buffer_size);
	}

	void TearDown() override
	{
		buffer.reset();
	}

	std::unique_ptr<char[]> buffer;
	size_t buffer_size{};
};

TEST_F(MemoryManagerTest, BasicAllocation)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr = mm.Allocate(100);
	ASSERT_NE(ptr, nullptr);

	EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % alignof(std::max_align_t), 0);

	mm.Free(ptr);
}

TEST_F(MemoryManagerTest, Alignment)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr = mm.Allocate(50, 16);
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 16, 0);

	mm.Free(ptr);
}

TEST_F(MemoryManagerTest, MultipleAllocations)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr1 = mm.Allocate(100);
	void* ptr2 = mm.Allocate(200);
	void* ptr3 = mm.Allocate(50);

	ASSERT_NE(ptr1, nullptr);
	ASSERT_NE(ptr2, nullptr);
	ASSERT_NE(ptr3, nullptr);

	EXPECT_NE(ptr1, ptr2);
	EXPECT_NE(ptr1, ptr3);
	EXPECT_NE(ptr2, ptr3);

	mm.Free(ptr1);
	mm.Free(ptr2);
	mm.Free(ptr3);
}

TEST_F(MemoryManagerTest, AllocationWithInsufficientSpace)
{
	MemoryManager mm(buffer.get(), 100);

	void* ptr1 = mm.Allocate(80);
	void* ptr2 = mm.Allocate(80);

	ASSERT_NE(ptr1, nullptr);
	ASSERT_EQ(ptr2, nullptr);

	mm.Free(ptr1);
}

TEST_F(MemoryManagerTest, FreeCoalescing)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr1 = mm.Allocate(100);
	void* ptr2 = mm.Allocate(100);
	void* ptr3 = mm.Allocate(100);

	ASSERT_NE(ptr1, nullptr);
	ASSERT_NE(ptr2, nullptr);
	ASSERT_NE(ptr3, nullptr);

	mm.Free(ptr2);
	mm.Free(ptr1);
	mm.Free(ptr3);
}

TEST_F(MemoryManagerTest, ZeroAllocation)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr = mm.Allocate(0);
	(void)ptr;
}

TEST_F(MemoryManagerTest, InvalidAlignment)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr = mm.Allocate(100, 3);
	ASSERT_EQ(ptr, nullptr);
}

TEST_F(MemoryManagerTest, NullptrFree)
{
	MemoryManager mm(buffer.get(), buffer_size);

	mm.Free(nullptr);
}

TEST_F(MemoryManagerTest, ReallocateAfterFree)
{
	MemoryManager mm(buffer.get(), buffer_size);

	void* ptr1 = mm.Allocate(100);
	ASSERT_NE(ptr1, nullptr);

	mm.Free(ptr1);

	void* ptr2 = mm.Allocate(100);
	ASSERT_NE(ptr2, nullptr);

	mm.Free(ptr2);
}

class ThreadSafetyTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		buffer_size = 1024 * 100;
		buffer = std::make_unique<char[]>(buffer_size);
	}

	void TearDown() override
	{
		buffer.reset();
	}

	std::unique_ptr<char[]> buffer;
	size_t buffer_size{};
};

TEST_F(ThreadSafetyTest, ConcurrentAllocationsAndFrees)
{
	MemoryManager mm(buffer.get(), buffer_size);
	const int numThreads = 4;
	const int operationsPerThread = 200;

	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> threadAllocations(numThreads);

	for (int i = 0; i < numThreads; ++i)
	{
		threads.emplace_back([&mm, &threadAllocations, i]() {
			std::vector<void*>& allocations = threadAllocations[i];
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> allocDis(1, 100);
			std::uniform_int_distribution<> opDis(0, 1);

			for (int j = 0; j < operationsPerThread; ++j)
			{
				if (opDis(gen) == 0)
				{
					size_t size = allocDis(gen);
					void* ptr = mm.Allocate(size);
					if (ptr)
					{
						allocations.push_back(ptr);
						memset(ptr, 0xFF, size);
					}
				}
				else
				{
					if (!allocations.empty())
					{
						size_t idx = allocDis(gen) % allocations.size();
						void* ptr = allocations[idx];
						allocations[idx] = allocations.back();
						allocations.pop_back();
						mm.Free(ptr);
					}
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	for (auto& allocations : threadAllocations)
	{
		for (void* ptr : allocations)
		{
			mm.Free(ptr);
		}
	}
}
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

	// Freeing nullptr should not crash
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

TEST_F(ThreadSafetyTest, ConcurrentAllocations)
{
	MemoryManager mm(buffer.get(), buffer_size);
	const int num_threads = 4;
	const int allocations_per_thread = 100;

	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> thread_allocations(num_threads);

	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&mm, &thread_allocations, i]() {
			std::vector<void*>& allocations = thread_allocations[i];
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(1, 100);

			for (int j = 0; j < allocations_per_thread; ++j)
			{
				size_t size = dis(gen);
				void* ptr = mm.Allocate(size);
				if (ptr)
				{
					allocations.push_back(ptr);
					memset(ptr, 0xFF, size);
				}
			}
		});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	for (auto& allocations : thread_allocations)
	{
		for (void* ptr : allocations)
		{
			mm.Free(ptr);
		}
	}
}

TEST_F(ThreadSafetyTest, ConcurrentAllocationsAndFrees)
{
	MemoryManager mm(buffer.get(), buffer_size);
	const int num_threads = 4;
	const int operations_per_thread = 200;

	std::vector<std::thread> threads;
	std::vector<std::vector<void*>> thread_allocations(num_threads);

	for (int i = 0; i < num_threads; ++i)
	{
		threads.emplace_back([&mm, &thread_allocations, i]() {
			std::vector<void*>& allocations = thread_allocations[i];
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> alloc_dis(1, 100);
			std::uniform_int_distribution<> op_dis(0, 1);

			for (int j = 0; j < operations_per_thread; ++j)
			{
				if (op_dis(gen) == 0)
				{
					size_t size = alloc_dis(gen);
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
						size_t idx = alloc_dis(gen) % allocations.size();
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

	for (auto& allocations : thread_allocations)
	{
		for (void* ptr : allocations)
		{
			mm.Free(ptr);
		}
	}
}
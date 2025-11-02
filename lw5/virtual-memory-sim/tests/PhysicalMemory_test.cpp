#include "MockOSHandler.h"
#include "PhysicalMemory.h"
#include "VirtualMemory.h"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

class PhysicalMemoryTest : public testing::Test
{
protected:
	PhysicalMemoryConfig cfg{ 4, 4096 };
	PhysicalMemory pm{ cfg };
	const uint32_t TOTAL_SIZE = 4 * 4096;
};

TEST_F(PhysicalMemoryTest, BasicReadWrite)
{
	pm.Write8(0, 0xAA);
	ASSERT_EQ(pm.Read8(0), 0xAA);

	pm.Write16(4, 0xBBCC);
	ASSERT_EQ(pm.Read16(4), 0xBBCC);

	pm.Write32(8, 0xDEADBEEF);
	ASSERT_EQ(pm.Read32(8), 0xDEADBEEF);

	pm.Write64(16, 0x1122334455667788ULL);
	ASSERT_EQ(pm.Read64(16), 0x1122334455667788ULL);
}

TEST_F(PhysicalMemoryTest, BoundaryCheck)
{
	ASSERT_THROW(auto _ = pm.Read8(TOTAL_SIZE), std::out_of_range);
	ASSERT_THROW(auto _ = pm.Read16(TOTAL_SIZE - 1), std::out_of_range);
	ASSERT_THROW(auto _ = pm.Read64(TOTAL_SIZE - 4), std::out_of_range);

	ASSERT_THROW(pm.Write32(TOTAL_SIZE, 0), std::out_of_range);
}

TEST_F(PhysicalMemoryTest, AlignmentCheck)
{
	ASSERT_THROW(auto _ = pm.Read16(1), std::invalid_argument);
	ASSERT_THROW(pm.Write16(1, 0x1234), std::invalid_argument);
	ASSERT_THROW(auto _ = pm.Read32(2), std::invalid_argument);
	ASSERT_THROW(auto _ = pm.Read64(4), std::invalid_argument);
}

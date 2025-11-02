#include "OSHandler.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>

TEST(PTETest, FrameNumberManipulation)
{
	PTE pte;

	pte.SetFrame(0x12345);
	ASSERT_EQ(pte.GetFrame(), 0x12345);
	ASSERT_EQ(pte.raw & 0xFFF, 0);

	pte.SetFrame(0x7FFFF);
	ASSERT_EQ(pte.GetFrame(), 0x7FFFF);
}

TEST(PTETest, FlagsManipulation)
{
	PTE pte;

	pte.SetPresent(true);
	ASSERT_TRUE(pte.IsPresent());
	pte.SetPresent(false);
	ASSERT_FALSE(pte.IsPresent());

	pte.SetWritable(true);
	ASSERT_TRUE(pte.IsWritable());
	pte.SetWritable(false);
	ASSERT_FALSE(pte.IsWritable());

	pte.SetUser(true);
	ASSERT_TRUE(pte.IsUser());
	pte.SetUser(false);
	ASSERT_FALSE(pte.IsUser());

	pte.SetAccessed(true);
	ASSERT_TRUE(pte.IsAccessed());
	pte.SetAccessed(false);
	ASSERT_FALSE(pte.IsAccessed());

	pte.SetDirty(true);
	ASSERT_TRUE(pte.IsDirty());
	pte.SetDirty(false);
	ASSERT_FALSE(pte.IsDirty());

	pte.SetNX(true);
	ASSERT_TRUE(pte.IsNX());
	pte.SetNX(false);
	ASSERT_FALSE(pte.IsNX());
}

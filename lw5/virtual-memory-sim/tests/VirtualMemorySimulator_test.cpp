#include "MockOSHandler.h"
#include "OSHandler.h"
#include "PhysicalMemory.h"
#include "VirtualMemory.h"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <stdexcept>
#include <tuple>

class VirtualMemoryTest : public testing::Test
{
protected:
	static constexpr uint32_t FRAME_SIZE = 4096;
	static constexpr uint32_t FRAME_COUNT = 16;
	static constexpr uint32_t DATA_FRAME = 1;
	static constexpr uint32_t VIRTUAL_ADDR_BASE = 0x1000;
	static constexpr uint32_t VIRTUAL_ADDR = VIRTUAL_ADDR_BASE + 0x120;
	static constexpr uint32_t PAGE_ADDR = (DATA_FRAME * FRAME_SIZE) + 0x120;
	static constexpr uint32_t VIRTUAL_PAGE_NUMBER = VIRTUAL_ADDR_BASE / FRAME_SIZE;

	PhysicalMemory physicalMemory{ PhysicalMemoryConfig{ FRAME_COUNT, FRAME_SIZE } };
	MockOSHandler handler;
	VirtualMemory virtualMemory{ physicalMemory, handler };

	void SetUp() override
	{
		virtualMemory.SetPageTableAddress(0);

		physicalMemory.Write32(PAGE_ADDR, 0xCAFEBABE);
		handler.Reset();
	}

	[[nodiscard]] PTE ReadPTE(uint32_t virtualPageNumber) const
	{
		const uint32_t pteAddress = virtualMemory.GetPageTableAddress() + (virtualPageNumber * sizeof(uint32_t));
		PTE pte;
		pte.raw = physicalMemory.Read32(pteAddress);
		return pte;
	}

	void SetupPTE(uint32_t virtualPageNumber, bool present, bool rw, bool us, bool nx, uint32_t frame)
	{
		PTE pte;
		pte.SetFrame(frame);
		pte.SetPresent(present);
		pte.SetWritable(rw);
		pte.SetUser(us);
		pte.SetNX(nx);
		const uint32_t pteAddress = virtualMemory.GetPageTableAddress() + (virtualPageNumber * sizeof(uint32_t));
		physicalMemory.Write32(pteAddress, pte.raw);
	}
};

TEST_F(VirtualMemoryTest, SuccessfulTranslationAndFlagsUpdate)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, true, false, DATA_FRAME);

	ASSERT_FALSE(ReadPTE(VIRTUAL_PAGE_NUMBER).IsAccessed());
	ASSERT_FALSE(ReadPTE(VIRTUAL_PAGE_NUMBER).IsDirty());

	ASSERT_EQ(virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, false), 0xCAFEBABE);

	const PTE pteAfterRead = ReadPTE(VIRTUAL_PAGE_NUMBER);
	ASSERT_TRUE(pteAfterRead.IsAccessed());
	ASSERT_FALSE(pteAfterRead.IsDirty());

	virtualMemory.Write32(VIRTUAL_ADDR, 0xFEEDDEAD, Privilege::User);

	ASSERT_EQ(physicalMemory.Read32(PAGE_ADDR), 0xFEEDDEAD);

	const PTE pteAfterWrite = ReadPTE(VIRTUAL_PAGE_NUMBER);
	ASSERT_TRUE(pteAfterWrite.IsAccessed());
	ASSERT_TRUE(pteAfterWrite.IsDirty());
}

TEST_F(VirtualMemoryTest, PageFault_NotPresent)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, false, true, true, false, DATA_FRAME);

	handler.SetReturn(false);

	ASSERT_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, false); }, PageFault);

	ASSERT_EQ(handler.callCount, 1);
	ASSERT_EQ(std::get<0>(handler.calls[0]), VIRTUAL_PAGE_NUMBER);
	ASSERT_EQ(std::get<2>(handler.calls[0]), PageFaultReason::NotPresent);
}

TEST_F(VirtualMemoryTest, PageFault_WriteToReadOnly)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, false, true, false, DATA_FRAME);

	handler.SetReturn(false);

	ASSERT_THROW({ virtualMemory.Write32(VIRTUAL_ADDR, 0x1234, Privilege::User); }, PageFault);

	ASSERT_EQ(handler.callCount, 1);
	ASSERT_EQ(std::get<2>(handler.calls[0]), PageFaultReason::WriteToReadOnly);
}

TEST_F(VirtualMemoryTest, PageFault_ExecOnNX)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, true, true, DATA_FRAME);

	handler.SetReturn(false);

	ASSERT_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, true); }, PageFault);

	ASSERT_EQ(handler.callCount, 1);
	ASSERT_EQ(std::get<2>(handler.calls[0]), PageFaultReason::ExecOnNX);
}

TEST_F(VirtualMemoryTest, PageFault_UserToSupervisor)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, false, false, DATA_FRAME);

	handler.SetReturn(false);

	ASSERT_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, false); }, PageFault);
	ASSERT_EQ(handler.callCount, 1);
	ASSERT_EQ(std::get<2>(handler.calls[0]), PageFaultReason::UserAccessToSupervisor);

	handler.Reset();
	ASSERT_NO_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::Supervisor, false); });
	ASSERT_EQ(handler.callCount, 0);
}

TEST_F(VirtualMemoryTest, PageFault_MisalignedAccess)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, true, false, DATA_FRAME);

	ASSERT_THROW({ virtualMemory.Read32(VIRTUAL_ADDR + 1, Privilege::User, false); }, PageFault);
	ASSERT_EQ(handler.callCount, 1);
	ASSERT_EQ(std::get<2>(handler.calls[0]), PageFaultReason::MisalignedAccess);
	ASSERT_EQ(std::get<0>(handler.calls[0]), VIRTUAL_PAGE_NUMBER);
}

TEST_F(VirtualMemoryTest, PageFault_RetryLogicSuccess)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, false, true, true, false, DATA_FRAME);

	handler.nextAction = [this](VirtualMemory&, uint32_t vpn, Access, PageFaultReason) {
		SetupPTE(vpn, true, true, true, false, DATA_FRAME);
		return true;
	};

	ASSERT_EQ(virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, false), 0xCAFEBABE);
	ASSERT_EQ(handler.callCount, 1);
	ASSERT_TRUE(ReadPTE(VIRTUAL_PAGE_NUMBER).IsPresent());
}

TEST_F(VirtualMemoryTest, PageFault_RetryLogicFail)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, false, true, true, false, DATA_FRAME);

	handler.SetReturn(false);

	ASSERT_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::User, false); }, PageFault);
	ASSERT_EQ(handler.callCount, 1);
}

TEST_F(VirtualMemoryTest, SupervisorIgnoresNX)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, true, true, DATA_FRAME);

	ASSERT_NO_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::Supervisor, true); });
}

TEST_F(VirtualMemoryTest, SupervisorIgnoresUS)
{
	SetupPTE(VIRTUAL_PAGE_NUMBER, true, true, false, false, DATA_FRAME);

	ASSERT_NO_THROW({ virtualMemory.Read32(VIRTUAL_ADDR, Privilege::Supervisor, false); });
}

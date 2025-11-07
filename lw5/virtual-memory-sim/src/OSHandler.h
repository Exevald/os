#pragma once

#include <cstdint>

class VirtualMemory;

enum class Access
{
	Read,
	Write,
};

enum class PageFaultReason
{
	NotPresent,
	WriteToReadOnly,
	ExecOnNX,
	UserAccessToSupervisor,
	PhysicalAccessOutOfRange,
	MisalignedAccess,
};

enum class Privilege
{
	User,
	Supervisor
};

class OSHandler
{
public:
	virtual ~OSHandler() = default;
	virtual bool OnPageFault(VirtualMemory& vm, uint32_t virtualPageNumber, Access access, PageFaultReason reason) = 0;
	virtual void OnSuccessfulAccess(uint32_t virtualPageNumber, Access access) = 0;
};

/*
* 31                        12 11 10  9  8  7  6  5  4  3  2  1  0
+--+-------------------------+--+--+--+--+--+--+--+--+--+--+--+--+
|NX frame number (20 bits)   |  |  |  |  |  |D |A |  |  |US|RW|P |
+--+-------------------------+--+--+--+--+--+--+--+--+--+--+--+--+
*/
struct PTE
{
	uint32_t raw = 0;

	static constexpr uint32_t P = 1u << 0;
	static constexpr uint32_t RW = 1u << 1;
	static constexpr uint32_t US = 1u << 2;
	static constexpr uint32_t A = 1u << 5;
	static constexpr uint32_t D = 1u << 6;
	static constexpr uint32_t NX = 1u << 31;

	static constexpr uint32_t FRAME_SHIFT = 12;
	static constexpr uint32_t FRAME_MASK_WITHOUT_NX = 0x7FFFF000u;

	[[nodiscard]] uint32_t GetFrame() const { return (raw & FRAME_MASK_WITHOUT_NX) >> FRAME_SHIFT; }

	void SetFrame(uint32_t fn) { raw = (raw & ~FRAME_MASK_WITHOUT_NX) | ((fn << FRAME_SHIFT) & FRAME_MASK_WITHOUT_NX); }

	[[nodiscard]] bool IsPresent() const { return raw & P; }
	void SetPresent(bool v) { raw = v ? (raw | P) : (raw & ~P); }

	[[nodiscard]] bool IsWritable() const { return raw & RW; }
	void SetWritable(bool v) { raw = v ? (raw | RW) : (raw & ~RW); }

	[[nodiscard]] bool IsUser() const { return raw & US; }
	void SetUser(bool v) { raw = v ? (raw | US) : (raw & ~US); }

	[[nodiscard]] bool IsAccessed() const { return raw & A; }
	void SetAccessed(bool v) { raw = v ? (raw | A) : (raw & ~A); }

	[[nodiscard]] bool IsDirty() const { return raw & D; }
	void SetDirty(bool v) { raw = v ? (raw | D) : (raw & ~D); }

	[[nodiscard]] bool IsNX() const { return raw & NX; }
	void SetNX(bool v) { raw = v ? (raw | NX) : (raw & ~NX); }
};
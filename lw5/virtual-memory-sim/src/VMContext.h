#pragma once

#include "VirtualMemory.h"

class VMContext
{
public:
	[[nodiscard]] uint8_t Read8(uint32_t address, bool execute = false) const;
	[[nodiscard]] uint16_t Read16(uint32_t address, bool execute = false) const;
	[[nodiscard]] uint32_t Read32(uint32_t address, bool execute = false) const;
	[[nodiscard]] uint64_t Read64(uint32_t address, bool execute = false) const;
	void Write8(uint32_t address, uint8_t value) const;
	void Write16(uint32_t address, uint16_t value) const;
	void Write32(uint32_t address, uint32_t value) const;
	void Write64(uint32_t address, uint64_t value) const;

protected:
	explicit VMContext(VirtualMemory& vm, Privilege privilege);
	[[nodiscard]] VirtualMemory& GetVM() const noexcept;

private:
	VirtualMemory* m_vm;
	Privilege m_privilege;
};

class VMUserContext : public VMContext
{
public:
	explicit VMUserContext(VirtualMemory& vm);
};

class VMSupervisorContext : public VMContext
{
public:
	explicit VMSupervisorContext(VirtualMemory& vm);

	void SetPageTableAddress(uint32_t physicalAddress) const;
	[[nodiscard]] uint32_t GetPageTableAddress() const noexcept;
};
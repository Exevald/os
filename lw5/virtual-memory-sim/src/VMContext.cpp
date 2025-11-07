#include "VMContext.h"

uint8_t VMContext::Read8(uint32_t address, bool execute) const
{
	return GetVM().Read8(address, m_privilege, execute);
}

uint16_t VMContext::Read16(uint32_t address, bool execute) const
{
	return GetVM().Read16(address, m_privilege, execute);
}

uint32_t VMContext::Read32(uint32_t address, bool execute) const
{
	return GetVM().Read32(address, m_privilege, execute);
}

uint64_t VMContext::Read64(uint32_t address, bool execute) const
{
	return GetVM().Read64(address, m_privilege, execute);
}

void VMContext::Write8(uint32_t address, uint8_t value) const
{
	GetVM().Write8(address, value, m_privilege);
}

void VMContext::Write16(uint32_t address, uint16_t value) const
{
	GetVM().Write16(address, value, m_privilege);
}

void VMContext::Write32(uint32_t address, uint32_t value) const
{
	GetVM().Write32(address, value, m_privilege);
}

void VMContext::Write64(uint32_t address, uint64_t value) const
{
	GetVM().Write64(address, value, m_privilege);
}

VMContext::VMContext(VirtualMemory& vm, Privilege privilege)
	: m_vm(&vm)
	, m_privilege(privilege)
{
}

VirtualMemory& VMContext::GetVM() const noexcept
{
	return *m_vm;
}

VMUserContext::VMUserContext(VirtualMemory& vm)
	: VMContext{ vm, Privilege::User }
{
}

VMSupervisorContext::VMSupervisorContext(VirtualMemory& vm)
	: VMContext{ vm, Privilege::Supervisor }
{
}

void VMSupervisorContext::SetPageTableAddress(uint32_t physicalAddress) const
{
	GetVM().SetPageTableAddress(physicalAddress);
}

uint32_t VMSupervisorContext::GetPageTableAddress() const noexcept
{
	return GetVM().GetPageTableAddress();
}
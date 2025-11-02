#include "VirtualMemory.h"

VirtualMemory::VirtualMemory(PhysicalMemory& physicalMem, OSHandler& handler)
    : m_physicalMemory(physicalMem)
    , m_osHandler(handler)
{
}

void VirtualMemory::SetPageTableAddress(uint32_t physicalAddress)
{
    if (physicalAddress % PAGE_SIZE != 0)
    {
       throw std::invalid_argument("Page Table Address must be frame aligned (4096 bytes)");
    }
    m_pageTableAddress = physicalAddress;
}

uint32_t VirtualMemory::GetPageTableAddress() const noexcept
{
    return m_pageTableAddress;
}

uint8_t VirtualMemory::Read8(uint32_t address, Privilege privilege, bool execute)
{
    return AccessMemory<uint8_t>(address, privilege, Access::Read, execute);
}

uint16_t VirtualMemory::Read16(uint32_t address, Privilege privilege, bool execute)
{
    return AccessMemory<uint16_t>(address, privilege, Access::Read, execute);
}

uint32_t VirtualMemory::Read32(uint32_t address, Privilege privilege, bool execute)
{
    return AccessMemory<uint32_t>(address, privilege, Access::Read, execute);
}

uint64_t VirtualMemory::Read64(uint32_t address, Privilege privilege, bool execute)
{
    return AccessMemory<uint64_t>(address, privilege, Access::Read, execute);
}

void VirtualMemory::Write8(uint32_t address, uint8_t value, Privilege privilege)
{
    AccessMemory<uint8_t>(address, privilege, Access::Write, false, value);
}

void VirtualMemory::Write16(uint32_t address, uint16_t value, Privilege privilege)
{
    AccessMemory<uint16_t>(address, privilege, Access::Write, false, value);
}

void VirtualMemory::Write32(uint32_t address, uint32_t value, Privilege privilege)
{
    AccessMemory<uint32_t>(address, privilege, Access::Write, false, value);
}

void VirtualMemory::Write64(uint32_t address, uint64_t value, Privilege privilege)
{
    AccessMemory<uint64_t>(address, privilege, Access::Write, false, value);
}

uint32_t VirtualMemory::TranslateAndCheck(uint32_t virtualAddress, Access access, Privilege privilege, bool needToExecute) const
{
    const uint32_t virtualPageNumber = (virtualAddress & VADDR_VPN_MASK) >> VADDR_PAGENUM_SHIFT;
    const uint32_t offset = virtualAddress & VADDR_OFFSET_MASK;

    const uint32_t pteAddress = m_pageTableAddress + (virtualPageNumber * sizeof(uint32_t));

    PTE pte;
    try
    {
       pte.raw = m_physicalMemory.Read32(pteAddress);
    }
    catch (const std::exception&)
    {
       throw PageFault(virtualPageNumber, access, PageFaultReason::PhysicalAccessOutOfRange);
    }

    if (!pte.IsPresent())
    {
       throw PageFault(virtualPageNumber, access, PageFaultReason::NotPresent);
    }
    if (privilege == Privilege::User && !pte.IsUser())
    {
       throw PageFault(virtualPageNumber, access, PageFaultReason::UserAccessToSupervisor);
    }

    if (needToExecute && pte.IsNX() && privilege == Privilege::User)
    {
       throw PageFault(virtualPageNumber, access, PageFaultReason::ExecOnNX);
    }

    if (access == Access::Write && !pte.IsWritable())
    {
       throw PageFault(virtualPageNumber, access, PageFaultReason::WriteToReadOnly);
    }

    PTE newPte = pte;
    newPte.SetAccessed(true);
    if (access == Access::Write)
    {
       newPte.SetDirty(true);
    }

    m_physicalMemory.Write32(pteAddress, newPte.raw);

    const uint32_t frameNumber = pte.GetFrame();
    const uint32_t physicalAddress = (frameNumber << VADDR_PAGENUM_SHIFT) | offset;

    return physicalAddress;
}

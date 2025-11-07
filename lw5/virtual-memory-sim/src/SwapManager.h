#pragma once

#include "VMContext.h"

#include <filesystem>
#include <map>

class SwapManager
{
public:
	explicit SwapManager(VMSupervisorContext& vmContext, const std::filesystem::path& path);
	uint32_t AllocateSlot(uint32_t virtualPageNumber);
	void FreeSlot(uint32_t virtualPageNumber);
	// Считывает страницу из swap-файла
	void ReadPage(uint32_t slot, uint32_t virtualPageNumber);
	// Записывает страницу в файл
	void WritePage(uint32_t slot, uint32_t virtualPageNumber);

private:
	[[nodiscard]] std::fstream OpenSwapFile(std::ios_base::openmode mode) const;

	VMSupervisorContext& m_vmContext;
	std::filesystem::path m_path;

	uint32_t m_nextSlotId = 1;
	std::map<uint32_t, uint32_t> m_vpnToSlot;
	std::map<uint32_t, uint32_t> m_slotToVpn;
	std::vector<uint32_t> m_freeSlots;

	static constexpr uint32_t PAGE_SIZE = 4096;
};
#include "SysInfoWinProvider.h"

namespace
{
std::string BytesToMBString(ULARGE_INTEGER bytes)
{
	return std::to_string(bytes.QuadPart / (1024.f * 1024.f)) + "MB";
}

std::string BytesToMBString(uint64_t bytes)
{
	return std::to_string(bytes / (1024.f * 1024.f)) + "MB";
}
} // namespace

std::string SysInfoWinProvider::GetWindowsVersionString() const
{
	if (IsWindows10OrGreater())
	{
		return "10 or Greater";
	}
	if (IsWindows8Point1OrGreater())
	{
		return "8.1 or Greater";
	}
	if (IsWindows8OrGreater())
	{
		return "8 or Greater";
	}
	if (IsWindows7SP1OrGreater())
	{
		return "7 with Service Pack 1 or Greater";
	}
	if (IsWindows7OrGreater())
	{
		return "7 or Greater";
	}
	if (IsWindowsXPOrGreater())
	{
		return "XP or Greater";
	}

	return "Windows version less than XP";
}

std::wstring SysInfoWinProvider::GetWindowsComputerNameString() const
{
	wchar_t name[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD size = sizeof(name) / sizeof(name[0]);
	if (!GetComputerNameW(name, &size))
	{
		throw std::runtime_error("Failed to get computer name");
	}

	return std::wstring(name);
}

std::wstring SysInfoWinProvider::GetWindowsUserNameString() const
{
	wchar_t name[UNLEN + 1] = {};
	DWORD size = sizeof(name) / sizeof(name[0]);
	if (!GetUserNameW(name, &size))
	{
		throw std::runtime_error("Failed to get user name");
	}

	return std::wstring(name);
}

std::string SysInfoWinProvider::GetWindowsArchitectureString() const
{
	SYSTEM_INFO si{};
	GetNativeSystemInfo(&si);

	switch (si.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		return "x64 (AMD64)";
	case PROCESSOR_ARCHITECTURE_INTEL:
		return "x86";
	case PROCESSOR_ARCHITECTURE_ARM:
		return "ARM";
	case PROCESSOR_ARCHITECTURE_ARM64:
		return "ARM64";
	default:
		return "Unknown architecture";
	}
}

SYSTEM_INFO SysInfoWinProvider::GetWindowsNativeSystemInfo() const
{
	SYSTEM_INFO si{};
	GetNativeSystemInfo(&si);

	return si;
}

MEMORYSTATUSEX SysInfoWinProvider::GetWindowsGlobalMemoryStatusEx() const
{
	MEMORYSTATUSEX memStatus{};
	memStatus.dwLength = sizeof(memStatus);
	if (!GlobalMemoryStatusEx(&memStatus))
	{
		throw std::runtime_error("GlobalMemoryStatusEx failed");
	}

	return memStatus;
}

PERFORMANCE_INFORMATION SysInfoWinProvider::GetWindowsPerfomanceInfo() const
{
	PERFORMANCE_INFORMATION perfomanceInfo{};
	perfomanceInfo.cb = sizeof(perfomanceInfo);
	if (!GetPerformanceInfo(&perfomanceInfo, sizeof(perfomanceInfo)))
	{
		throw std::runtime_error("GetPerformanceInfo failed");
	}
	
	return perfomanceInfo;
}

std::vector<wchar_t> SysInfoWinProvider::GetWindowLogicalDriveStrings() const
{
	DWORD len = GetLogicalDriveStringsW(0, nullptr);
	if (len == 0)
	{
		throw std::runtime_error("Failed to get drive strings");
	}
	std::vector<wchar_t> buffer(len);
	if (!GetLogicalDriveStringsW(len, buffer.data()))
	{
		throw std::runtime_error("Failed to get drive strings");
	}

	return buffer;
}

UINT SysInfoWinProvider::GetWindowsLogicalDriveType(wchar_t* drive) const
{
	return GetDriveTypeW(drive);
}

std::wstring SysInfoWinProvider::GetWindowsVolumeInformation(wchar_t* drive) const
{
	wchar_t fsName[MAX_PATH] = {};
	if (!GetVolumeInformationW(drive, nullptr, 0, nullptr, nullptr, nullptr, fsName, MAX_PATH))
	{
		wcscpy_s(fsName, L"Unknown");
	}
	return std::wstring(fsName);
}


void SysInfoWinProvider::GetWindowsDiskFreeSpaceEx(
	wchar_t* drive,
	ULARGE_INTEGER& freeBytesAvailable,
	ULARGE_INTEGER& totalNumberOfBytes,
	ULARGE_INTEGER& totalNumberOfFreeBytes) const
{
	if (!GetDiskFreeSpaceExW(drive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
	{
		throw std::runtime_error("Unable to get free space on disk" + *drive);
	}
}

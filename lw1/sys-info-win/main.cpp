#include <windows.h>       
#include <psapi.h>         
#include <versionhelpers.h>
#include <Lmcons.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

std::string BytesToMBString(ULARGE_INTEGER bytes)
{
	return std::to_string(bytes.QuadPart / (1024 * 1024)) + "MB";
}

std::string BytesToMBString(uint64_t bytes)
{
	return std::to_string(bytes / (1024 * 1024)) + "MB";
}

std::string GetArchitectureString()
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

std::wstring GetComputerNameString()
{
	wchar_t name[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD size = sizeof(name) / sizeof(name[0]);
	if (!GetComputerNameW(name, &size))
	{
		throw std::runtime_error("Failed to get computer name");
	}
	return std::wstring(name);
}

std::wstring GetUserNameString()
{
	wchar_t name[UNLEN + 1] = {};
	DWORD size = sizeof(name) / sizeof(name[0]);
	if (!GetUserNameW(name, &size))
	{
		throw std::runtime_error("Failed to get user name");
	}
	return std::wstring(name);
}

std::string GetWindowsVersionString()
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

void PrintDrivesInfo()
{
	DWORD len = GetLogicalDriveStringsW(0, nullptr);
	if (len == 0)
	{
		std::cerr << "Failed to get drive strings\n";
		return;
	}
	std::vector<wchar_t> buffer(len);
	if (!GetLogicalDriveStringsW(len, buffer.data()))
	{
		std::cerr << "Failed to get drive strings\n";
		return;
	}

	std::wcout << L"Drives:\n";
	wchar_t* drive = buffer.data();
	while (*drive)
	{
		UINT type = GetDriveTypeW(drive);

		wchar_t fsName[MAX_PATH] = {};
		if (!GetVolumeInformationW(drive, nullptr, 0, nullptr, nullptr, nullptr, fsName, MAX_PATH))
		{
			wcscpy_s(fsName, L"Unknown");
		}

		ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
		if (GetDiskFreeSpaceExW(drive, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
		{
			std::wcout << L"  - " << drive << L" (" << fsName << L"): "
					   << freeBytesAvailable.QuadPart / (1024 * 1024 * 1024) << L" GB free / "
					   << totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024) << L" GB total\n";
		}
		else
		{
			std::wcout << L"  - " << drive << L" (" << fsName << L"): Unable to get free space\n";
		}

		while (*drive)
		{
			++drive;
		}
		++drive;
	}
}

void PrintSystemInfo()
{
	try
	{
		std::cout << "OS: Windows " << GetWindowsVersionString() << std::endl;
		std::wcout << L"Computer Name: " << GetComputerNameString() << std::endl;
		std::wcout << L"User: " << GetUserNameString() << std::endl;

		std::cout << "Architecture: " << GetArchitectureString() << std::endl;

		SYSTEM_INFO sysInfo{};
		GetNativeSystemInfo(&sysInfo);
		std::cout << "Processors: " << sysInfo.dwNumberOfProcessors << std::endl;

		MEMORYSTATUSEX memStatus{};
		memStatus.dwLength = sizeof(memStatus);
		if (!GlobalMemoryStatusEx(&memStatus))
		{
			throw std::runtime_error("GlobalMemoryStatusEx failed");
		}

		std::cout << "RAM: " << BytesToMBString(memStatus.ullAvailPhys)
				  << " / " << BytesToMBString(memStatus.ullTotalPhys) << std::endl;

		std::cout << "Virtual Memory: " << BytesToMBString(memStatus.ullAvailVirtual)
				  << " / " << BytesToMBString(memStatus.ullTotalVirtual) << std::endl;

		std::cout << "Memory Load: " << memStatus.dwMemoryLoad << "%" << std::endl;

		PERFORMANCE_INFORMATION perfInfo{};
		perfInfo.cb = sizeof(perfInfo);
		if (!GetPerformanceInfo(&perfInfo, sizeof(perfInfo)))
		{
			throw std::runtime_error("GetPerformanceInfo failed");
		}
		std::cout << "Pagefile: "
				  << memStatus.ullAvailPageFile / (1024 * 1024) << "MB / "
				  << memStatus.ullTotalPageFile / (1024 * 1024) << "MB"
				  << std::endl;

		PrintDrivesInfo();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

int main()
{
	PrintSystemInfo();

	return 0;
}

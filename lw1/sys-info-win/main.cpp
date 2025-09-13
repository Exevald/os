#include "SysInfoWinProvider.h"

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

void PrintDrivesInfo()
{
	SysInfoWinProvider provider;
	std::vector<wchar_t> drives = provider.GetWindowLogicalDriveStrings();

	std::wcout << L"Drives:\n";
	const wchar_t* ptr = drives.data();
	while (*ptr != L'\0')
	{
		UINT type = provider.GetWindowsLogicalDriveType(const_cast<wchar_t*>(ptr));
		std::wstring fsName = provider.GetWindowsVolumeInformation(const_cast<wchar_t*>(ptr));

		ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
		try
		{
			provider.GetWindowsDiskFreeSpaceEx(
				const_cast<wchar_t*>(ptr), 
				freeBytesAvailable, 
				totalNumberOfBytes,
				totalNumberOfFreeBytes);
			std::wcout << L"  - " << ptr << L" (" << fsName << L"): "
					   << freeBytesAvailable.QuadPart / (1024.f * 1024.f * 1024.f) << L" GB free / "
					   << totalNumberOfBytes.QuadPart / (1024.f * 1024.f * 1024.f) << L" GB total\n";
		}
		catch (const std::exception& exception)
		{
			std::wcout << L"  - " << ptr << L" (" << fsName << L"): Unable to get free space\n";
		}
		ptr += wcslen(ptr) + 1;
	}
}

void PrintSystemInfo()
{
	try
	{
		SysInfoWinProvider provider;
		std::cout << "OS: Windows " << provider.GetWindowsVersionString() << std::endl;
		std::wcout << L"Computer Name: " << provider.GetWindowsComputerNameString() << std::endl;
		std::wcout << L"User: " << provider.GetWindowsUserNameString() << std::endl;

		std::cout << "Architecture: " << provider.GetWindowsArchitectureString() << std::endl;

		SYSTEM_INFO sysInfo = provider.GetWindowsNativeSystemInfo();
		std::cout << "Processors: " << sysInfo.dwNumberOfProcessors << std::endl;

		MEMORYSTATUSEX memStatus = provider.GetWindowsGlobalMemoryStatusEx();
		std::cout << "RAM: " << BytesToMBString(memStatus.ullAvailPhys)
				  << " / " << BytesToMBString(memStatus.ullTotalPhys) << std::endl;

		std::cout << "Virtual Memory: " << BytesToMBString(memStatus.ullAvailVirtual)
				  << " / " << BytesToMBString(memStatus.ullTotalVirtual) << std::endl;

		std::cout << "Memory Load: " << memStatus.dwMemoryLoad << "%" << std::endl;

		PERFORMANCE_INFORMATION perfInfo = provider.GetWindowsPerfomanceInfo();
		std::cout << "Pagefile: "
				  << memStatus.ullAvailPageFile / (1024.f * 1024.f) << "MB / "
				  << memStatus.ullTotalPageFile / (1024.f * 1024.f) << "MB"
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

#pragma once

#include <windows.h>       
#include <psapi.h>         
#include <versionhelpers.h>
#include <Lmcons.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <string>

class SysInfoWinProvider
{
public:
	SysInfoWinProvider() = default;

	std::string GetWindowsVersionString() const;
	std::wstring GetWindowsComputerNameString() const;
	std::wstring GetWindowsUserNameString() const;
	std::string GetWindowsArchitectureString() const;

	SYSTEM_INFO GetWindowsNativeSystemInfo() const;
	MEMORYSTATUSEX GetWindowsGlobalMemoryStatusEx() const;

	PERFORMANCE_INFORMATION GetWindowsPerfomanceInfo() const;

	std::vector<wchar_t> GetWindowLogicalDriveStrings() const;
	UINT GetWindowsLogicalDriveType(wchar_t* drive) const;
	std::wstring GetWindowsVolumeInformation(wchar_t* drive) const;
	void GetWindowsDiskFreeSpaceEx(
		wchar_t* drive,
		ULARGE_INTEGER& freeBytesAvailable,
		ULARGE_INTEGER& totalNumberOfBytes,
		ULARGE_INTEGER& totalNumberOfFreeBytes) const;
};
#pragma once

#include "ProcessInfo.h"

#include <string>
#include <vector>

class ProcessScanner
{
public:
	static std::vector<pid_t> ScanProcForPids();
	static ProcessInfo CollectProcessInfo(pid_t pid);
	static std::vector<ProcessInfo> CollectAllProcesses();
	static void PrintProcessTable(const std::vector<ProcessInfo>& processes);
	static void PrintSummary(const std::vector<ProcessInfo>& processes);

private:
	static std::string GetExeBasename(pid_t pid);
	static std::string ReadProcFile(const std::string& path);
	static bool ReadUidFromStatus(const std::string& statusPath, uid_t& uid);
	static bool ReadThreadsFromStatus(const std::string& statusPath, int& threads);
	static std::string ReadComm(const std::string& commPath);
	static std::string ReadCmdline(const std::string& cmdlinePath);
	static bool TryReadSmapsRollup(const std::string& smapsPath, unsigned long long& privateMem, unsigned long long& sharedMem);
	static bool TryReadStatusFallback(const std::string& statusPath, unsigned long long& privateMem, unsigned long long& sharedMem);
	static std::string GetUserName(uid_t uid);
	static std::string FormatSize(unsigned long long sizeKib);
};
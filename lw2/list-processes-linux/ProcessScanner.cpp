#include "ProcessScanner.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <pwd.h>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

namespace
{
std::string GetUserName(uid_t uid)
{
	if (const passwd* pwd = getpwuid(uid))
	{
		return pwd->pw_name;
	}
	return "unknown";
}

std::string ReadProcFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		return "";
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

bool ReadUidFromStatus(const std::string& statusPath, uid_t& uid)
{
	std::string content = ReadProcFile(statusPath);
	if (content.empty())
	{
		return false;
	}

	std::istringstream iss(content);
	std::string line;
	while (std::getline(iss, line))
	{
		if (line.find("Uid:") == 0)
		{
			std::istringstream lss(line);
			std::string key;
			unsigned int u, u2, u3, u4;
			lss >> key >> u >> u2 >> u3 >> u4;
			uid = u;
			return true;
		}
	}
	return false;
}

bool ReadThreadsFromStatus(const std::string& statusPath, int& threads)
{
	std::string content = ReadProcFile(statusPath);
	if (content.empty())
	{
		return false;
	}

	std::istringstream iss(content);
	std::string line;
	while (std::getline(iss, line))
	{
		if (line.find("Threads:") == 0)
		{
			std::istringstream lss(line);
			std::string key;
			lss >> key >> threads;

			return true;
		}
	}
	return false;
}

std::string ReadComm(const std::string& commPath)
{
	if (std::string content = ReadProcFile(commPath); !content.empty())
	{
		if (content.back() == '\n')
		{
			content.pop_back();
		}
		return content;
	}
	return "";
}

std::string ReadCmdline(const std::string& cmdlinePath)
{
	std::ifstream file(cmdlinePath, std::ios::binary);
	if (!file.is_open())
	{
		return "";
	}

	std::string result;
	char ch;
	while (file.get(ch))
	{
		if (ch == '\0')
		{
			result += ' ';
		}
		else
		{
			result += ch;
		}
	}
	if (!result.empty() && result.back() == ' ')
	{
		result.pop_back();
	}
	return result.empty() ? "[no cmdline]" : result;
}

bool TryReadSmapsRollup(const std::string& smapsPath, unsigned long long& privateMemoryUsage, unsigned long long& sharedMemoryUsage)
{
	const std::string content = ReadProcFile(smapsPath);
	if (content.empty())
	{
		return false;
	}

	std::istringstream iss(content);
	std::string line;
	unsigned long long privateClean = 0, privateDirty = 0, sharedClean = 0, sharedDirty = 0;

	while (std::getline(iss, line))
	{
		if (line.find("Private_Clean:") == 0)
		{
			sscanf(line.c_str(), "Private_Clean: %llu kB", &privateClean);
		}
		else if (line.find("Private_Dirty:") == 0)
		{
			sscanf(line.c_str(), "Private_Dirty: %llu kB", &privateDirty);
		}
		else if (line.find("Shared_Clean:") == 0)
		{
			sscanf(line.c_str(), "Shared_Clean: %llu kB", &sharedClean);
		}
		else if (line.find("Shared_Dirty:") == 0)
		{
			sscanf(line.c_str(), "Shared_Dirty: %llu kB", &sharedDirty);
		}
	}
	privateMemoryUsage = privateClean + privateDirty;
	sharedMemoryUsage = sharedClean + sharedDirty;

	return true;
}

bool TryReadStatusFallback(const std::string& statusPath, unsigned long long& privateMem, unsigned long long& sharedMem)
{
	const std::string content = ReadProcFile(statusPath);
	if (content.empty())
	{
		return false;
	}

	std::istringstream iss(content);
	std::string line;
	unsigned long long rssAnon = 0, rssFile = 0, rssShmem = 0;

	while (std::getline(iss, line))
	{
		if (line.find("RssAnon:") == 0)
		{
			sscanf(line.c_str(), "RssAnon: %llu kB", &rssAnon);
		}
		else if (line.find("RssFile:") == 0)
		{
			sscanf(line.c_str(), "RssFile: %llu kB", &rssFile);
		}
		else if (line.find("RssShmem:") == 0)
		{
			sscanf(line.c_str(), "RssShmem: %llu kB", &rssShmem);
		}
	}

	privateMem = rssAnon;
	sharedMem = rssFile + rssShmem;
	return true;
}

std::string GetExeBasename(pid_t pid)
{
	char linkTarget[PATH_MAX];
	const std::string exePath = "/proc/" + std::to_string(pid) + "/exe";
	if (const ssize_t len = readlink(exePath.c_str(), linkTarget, sizeof(linkTarget) - 1); len != -1)
	{
		linkTarget[len] = '\0';
		std::string fullPath(linkTarget);
		if (const size_t lastSlash = fullPath.find_last_of('/'); lastSlash != std::string::npos)
		{
			return fullPath.substr(lastSlash + 1);
		}
		return fullPath;
	}
	return "";
}
} // namespace

std::vector<pid_t> ProcessScanner::GetAllProcessesIds()
{
	std::vector<pid_t> pids;
	DIR* dir = opendir("/proc");
	if (!dir)
	{
		std::cerr << "Failed to open /proc" << std::endl;
		return pids;
	}

	dirent* entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		if (entry->d_type == DT_DIR)
		{
			try
			{
				auto pid = static_cast<pid_t>(std::stoul(entry->d_name));
				pids.push_back(pid);
			}
			catch (...)
			{
			}
		}
	}
	closedir(dir);
	return pids;
}

ProcessInfo ProcessScanner::GetProcessInfo(pid_t pid)
{
	ProcessInfo info;
	info.pid = pid;

	if (const std::string exeBasename = GetExeBasename(pid); exeBasename.empty())
	{
		const std::string commPath = "/proc/" + std::to_string(pid) + "/comm";
		info.processName = ReadComm(commPath);
		if (info.processName.empty())
		{
			info.processName = "[unknown]";
		}
	}
	else
	{
		info.processName = exeBasename;
	}

	const std::string statusPath = "/proc/" + std::to_string(pid) + "/status";
	if (uid_t uid; ReadUidFromStatus(statusPath, uid))
	{
		info.user = GetUserName(uid);
	}
	else
	{
		info.user = "unknown";
	}

	if (const std::string smapsPath = "/proc/" + std::to_string(pid) + "/smaps_rollup";
		!TryReadSmapsRollup(smapsPath, info.privateMemKb, info.sharedMemKb))
	{
		if (!TryReadStatusFallback(statusPath, info.privateMemKb, info.sharedMemKb))
		{
			info.privateMemKb = 0;
			info.sharedMemKb = 0;
		}
	}

	if (!ReadThreadsFromStatus(statusPath, info.threads))
	{
		info.threads = 0;
	}

	const std::string cmdlinePath = "/proc/" + std::to_string(pid) + "/cmdline";
	info.cmdline = ReadCmdline(cmdlinePath);

	return info;
}

std::vector<ProcessInfo> ProcessScanner::CollectAllProcesses()
{
	const std::vector<pid_t> pids = GetAllProcessesIds();
	std::vector<ProcessInfo> processes;
	processes.reserve(pids.size());

	for (const pid_t pid : pids)
	{
		processes.push_back(GetProcessInfo(pid));
	}
	return processes;
}
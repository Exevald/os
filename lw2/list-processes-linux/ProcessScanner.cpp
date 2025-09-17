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

std::vector<pid_t> ProcessScanner::ScanProcForPids()
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
				pid_t pid = std::stoul(entry->d_name);
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

ProcessInfo ProcessScanner::CollectProcessInfo(pid_t pid)
{
	ProcessInfo info;
	info.pid = pid;

	if (const std::string exeBasename = GetExeBasename(pid); exeBasename.empty())
	{
		const std::string commPath = "/proc/" + std::to_string(pid) + "/comm";
		info.comm = ReadComm(commPath);
		if (info.comm.empty())
		{
			info.comm = "[unknown]";
		}
	}
	else
	{
		info.comm = exeBasename;
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

	if (const std::string smapsPath = "/proc/" + std::to_string(pid) + "/smaps_rollup"; !TryReadSmapsRollup(smapsPath, info.privateMem, info.sharedMem))
	{
		if (!TryReadStatusFallback(statusPath, info.privateMem, info.sharedMem))
		{
			info.privateMem = 0;
			info.sharedMem = 0;
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
	const std::vector<pid_t> pids = ScanProcForPids();
	std::vector<ProcessInfo> processes;
	processes.reserve(pids.size());

	for (const pid_t pid : pids)
	{
		processes.push_back(CollectProcessInfo(pid));
	}
	return processes;
}

void ProcessScanner::PrintProcessTable(const std::vector<ProcessInfo>& processes)
{
	std::cout << std::left
			  << std::setw(8) << "PID"
			  << std::setw(20) << "COMM"
			  << std::setw(15) << "USER"
			  << std::setw(12) << "PRIVATE"
			  << std::setw(12) << "SHARED"
			  << std::setw(8) << "%CPU"
			  << std::setw(8) << "#THR"
			  << "CMD" << std::endl;

	std::cout << std::string(90, '-') << std::endl;

	for (const auto& proc : processes)
	{
		std::cout << std::setw(8) << proc.pid
				  << std::setw(20) << proc.comm
				  << std::setw(15) << proc.user
				  << std::setw(12) << FormatSize(proc.privateMem)
				  << std::setw(12) << FormatSize(proc.sharedMem)
				  << std::setw(8) << std::fixed << std::setprecision(1) << proc.cpuPercent
				  << std::setw(8) << proc.threads
				  << proc.cmdline << std::endl;
	}
}

void ProcessScanner::PrintSummary(const std::vector<ProcessInfo>& processes)
{
	unsigned long long totalPrivate = 0, totalShared = 0;
	const std::size_t totalCount = processes.size();

	for (const auto& proc : processes)
	{
		totalPrivate += proc.privateMem;
		totalShared += proc.sharedMem;
	}

	std::cout << std::string(90, '=') << std::endl;
	std::cout << "Всего процессов: " << totalCount << std::endl;
	std::cout << "Суммарная PRIVATE: " << FormatSize(totalPrivate) << std::endl;
	std::cout << "Суммарная SHARED: " << FormatSize(totalShared) << std::endl;
	std::cout << "Суммарная TOTAL: " << FormatSize(totalPrivate + totalShared) << std::endl;
}

std::string ProcessScanner::FormatSize(unsigned long long sizeKib)
{
	if (sizeKib < 1024)
	{
		return std::to_string(sizeKib) + " KiB";
	}
	const double mib = static_cast<double>(sizeKib) / 1024.0;
	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << mib << " MiB";
	return ss.str();
}

std::string ProcessScanner::GetUserName(uid_t uid)
{
	if (const passwd* pwd = getpwuid(uid))
	{
		return std::string(pwd->pw_name);
	}
	return "unknown";
}

std::string ProcessScanner::ReadProcFile(const std::string& path)
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

bool ProcessScanner::ReadUidFromStatus(const std::string& statusPath, uid_t& uid)
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

bool ProcessScanner::ReadThreadsFromStatus(const std::string& statusPath, int& threads)
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

std::string ProcessScanner::ReadComm(const std::string& commPath)
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

std::string ProcessScanner::ReadCmdline(const std::string& cmdlinePath)
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

bool ProcessScanner::TryReadSmapsRollup(const std::string& smapsPath, unsigned long long& privateMem, unsigned long long& sharedMem)
{
	const std::string content = ReadProcFile(smapsPath);
	if (content.empty())
	{
		return false;
	}

	std::istringstream iss(content);
	std::string line;
	unsigned long long p_clean = 0, p_dirty = 0, s_clean = 0, s_dirty = 0;

	while (std::getline(iss, line))
	{
		if (line.find("Private_Clean:") == 0)
		{
			sscanf(line.c_str(), "Private_Clean: %llu kB", &p_clean);
		}
		else if (line.find("Private_Dirty:") == 0)
		{
			sscanf(line.c_str(), "Private_Dirty: %llu kB", &p_dirty);
		}
		else if (line.find("Shared_Clean:") == 0)
		{
			sscanf(line.c_str(), "Shared_Clean: %llu kB", &s_clean);
		}
		else if (line.find("Shared_Dirty:") == 0)
		{
			sscanf(line.c_str(), "Shared_Dirty: %llu kB", &s_dirty);
		}
	}

	privateMem = p_clean + p_dirty;
	sharedMem = s_clean + s_dirty;
	return true;
}

bool ProcessScanner::TryReadStatusFallback(const std::string& statusPath, unsigned long long& privateMem, unsigned long long& sharedMem)
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

std::string ProcessScanner::GetExeBasename(pid_t pid)
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
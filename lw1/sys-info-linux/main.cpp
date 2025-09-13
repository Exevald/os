#include "FileDesc.h"

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <mntent.h>
#include <pwd.h>
#include <sstream>
#include <string>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <vector>

void GetMemoryInfo(int& freeMemoryMB, int& totalMemoryMB);
void GetSwapInfo(int& totalSwapMB, int& freeSwapMB);
int GetVirtualMemory();

std::string ReadFileContent(const char* path)
{
	const FileDesc fd(path, O_RDONLY);
	constexpr size_t bufferSize = 4096;
	std::vector<char> buffer(bufferSize);
	std::string result;
	ssize_t n;
	while ((n = fd.Read(buffer.data(), bufferSize)) > 0)
	{
		result.append(buffer.data(), static_cast<size_t>(n));
	}
	if (n < 0)
	{
		throw std::system_error(errno, std::generic_category(), "Error reading file");
	}
	return result;
}

std::string GetKernelInfo()
{
	utsname uts{};
	if (uname(&uts) < 0)
	{
		perror("uname");
		throw std::runtime_error("uname() failed");
	}
	return std::string(uts.sysname) + " " + uts.release;
}

std::string GetArchitecture()
{
	utsname uts{};
	if (uname(&uts) < 0)
	{
		perror("uname");
		throw std::runtime_error("uname() failed");
	}
	return uts.machine;
}

std::string GetHostname()
{
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) < 0)
	{
		perror("gethostname");
		throw std::runtime_error("gethostname() failed");
	}
	return hostname;
}

std::string GetUsername()
{
	if (const char* login = getlogin())
	{
		return login;
	}
	if (const passwd* pw = getpwuid(getuid()))
	{
		return pw->pw_name;
	}
	return "unknown";
}

void GetMemoryStatus(int& freeMemoryMB, int& totalMemoryMB)
{
	try
	{
		GetMemoryInfo(freeMemoryMB, totalMemoryMB);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading memory info: " << e.what() << "\n";
		freeMemoryMB = 0;
		totalMemoryMB = 0;
	}
}

std::string GetOSRelease()
{
	const std::string content = ReadFileContent("/etc/os-release");
	std::istringstream iss(content);
	std::string line;
	while (getline(iss, line))
	{
		if (line.find("PRETTY_NAME=") == 0)
		{
			if (const auto pos = line.find('='); pos != std::string::npos)
			{
				std::string str = line.substr(pos + 1);
				return str;
			}
		}
	}
	return {};
}

void ParseMemoryInfoField(const std::string& content, const std::string& field, int& outMB)
{
	std::istringstream iss(content);
	std::string line;
	outMB = 0;
	while (getline(iss, line))
	{
		if (line.find(field) == 0)
		{
			std::istringstream stream(line);
			std::string key;
			int valueKB;
			stream >> key >> valueKB;
			outMB = valueKB / 1024;
			break;
		}
	}
}

void GetMemoryInfo(int& freeMemoryMB, int& totalMemoryMB)
{
	const std::string content = ReadFileContent("/proc/meminfo");
	ParseMemoryInfoField(content, "MemFree:", freeMemoryMB);
	ParseMemoryInfoField(content, "MemTotal:", totalMemoryMB);
}

void GetSwapInfo(int& totalSwapMB, int& freeSwapMB)
{
	const std::string content = ReadFileContent("/proc/meminfo");
	ParseMemoryInfoField(content, "TotalSwap:", totalSwapMB);
	ParseMemoryInfoField(content, "FreeSwap:", freeSwapMB);
}

int GetVirtualMemory()
{
	const std::string content = ReadFileContent("/proc/meminfo");
	int totalVirtualMemoryMB = 0;
	ParseMemoryInfoField(content, "TotalVirtualMemory:", totalVirtualMemoryMB);
	return totalVirtualMemoryMB;
}

void GetSwapStatus(int& totalSwapMB, int& freeSwapMB)
{
	try
	{
		GetSwapInfo(totalSwapMB, freeSwapMB);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading swap info: " << e.what() << "\n";
		totalSwapMB = 0;
		freeSwapMB = 0;
	}
}

int GetVirtualMem()
{
	try
	{
		return GetVirtualMemory();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading virtual memory info: " << e.what() << "\n";
		return 0;
	}
}

int GetLogicalProcessors()
{
	return get_nprocs();
}

void GetLoadAverages(double& load1, double& load5, double& load15)
{
	struct sysinfo si{};
	if (sysinfo(&si) < 0)
	{
		perror("sysinfo");
		throw std::runtime_error("sysinfo() failed");
	}

	load1 = static_cast<double>(si.loads[0]) / 65536.0;
	load5 = static_cast<double>(si.loads[1]) / 65536.0;
	load15 = static_cast<double>(si.loads[2]) / 65536.0;
}

FileDesc OpenProcMounts()
{
	return FileDesc("/proc/mounts", O_RDONLY);
}

void PrintDrivesFileDescriptor(const FileDesc& fd)
{
	constexpr size_t bufferSize = 4096;
	std::vector<char> buffer(bufferSize);
	std::string allData;
	ssize_t bytesRead;
	while ((bytesRead = fd.Read(buffer.data(), bufferSize)) > 0)
	{
		allData.append(buffer.data(), static_cast<size_t>(bytesRead));
	}
	if (bytesRead < 0)
	{
		perror("Read /proc/mounts");
		return;
	}
	FILE* mountsStream = fmemopen(allData.data(), allData.size(), "r");
	if (!mountsStream)
	{
		perror("fmemopen");
		return;
	}

	std::cout << "Drives:\n";
	mntent* mnt;
	while ((mnt = getmntent(mountsStream)) != nullptr)
	{
		struct statvfs vfs{};
		if (statvfs(mnt->mnt_dir, &vfs) == 0)
		{
			const auto totalBytes = vfs.f_blocks * vfs.f_frsize;
			const auto freeBytes = vfs.f_bfree * vfs.f_frsize;
			const auto totalGB = totalBytes / (1024ULL * 1024 * 1024);
			const auto freeGB = freeBytes / (1024ULL * 1024 * 1024);

			std::cout << "  " << mnt->mnt_dir << "\t" << mnt->mnt_type << "\t"
					  << freeGB << "GB free / " << totalGB << "GB total\n";
		}
	}
	fclose(mountsStream);
}

void PrintSystemInfo()
{
	std::string osRelease;
	try
	{
		osRelease = GetOSRelease();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading /etc/os-release: " << e.what() << "\n";
		osRelease = "unknown";
	}

	try
	{
		std::cout << "OS: " << osRelease << "\n";
		std::cout << "Kernel: " << GetKernelInfo() << "\n";
		std::cout << "Architecture: " << GetArchitecture() << "\n";
		std::cout << "Hostname: " << GetHostname() << "\n";
		std::cout << "User: " << GetUsername() << "\n";

		int freeMem, totalMem;
		GetMemoryStatus(freeMem, totalMem);
		std::cout << "RAM: " << freeMem << "MB free / " << totalMem << "MB total\n";

		int totalSwap, freeSwap;
		GetSwapStatus(totalSwap, freeSwap);
		std::cout << "Swap: " << totalSwap << "MB total / " << freeSwap << "MB free\n";

		int virtualMem = GetVirtualMem();
		std::cout << "Virtual memory: " << virtualMem << " MB\n";

		std::cout << "Processors: " << GetLogicalProcessors() << "\n";

		double load1, load5, load15;
		GetLoadAverages(load1, load5, load15);
		std::cout << "Load average: " << load1 << ", " << load5 << ", " << load15 << "\n";
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error getting system info: " << e.what() << "\n";
	}
}

int main()
{
	PrintSystemInfo();

	try
	{
		const FileDesc mountsFileDescriptor = OpenProcMounts();
		PrintDrivesFileDescriptor(mountsFileDescriptor);
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Error opening /proc/mounts: " << exception.what() << "\n";
	}

	return 0;
}

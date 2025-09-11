#include "FileDesc.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
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


std::string read_file_content(const char* path)
{
	const FileDesc fd(path, O_RDONLY);
	constexpr size_t buf_size = 4096;
	std::vector<char> buffer(buf_size);
	std::string result;
	ssize_t n;
	while ((n = fd.Read(buffer.data(), buf_size)) > 0)
	{
		result.append(buffer.data(), static_cast<size_t>(n));
	}
	if (n < 0)
	{
		throw std::system_error(errno, std::generic_category(), "Error reading file");
	}
	return result;
}

std::string get_os_release()
{
	std::string content = read_file_content("/etc/os-release");
	std::istringstream iss(content);
	std::string line;
	while (getline(iss, line))
	{
		if (line.find("PRETTY_NAME=") == 0)
		{
			auto pos = line.find('=');
			if (pos != std::string::npos)
			{
				std::string val = line.substr(pos + 1);
				if (!val.empty() && val.front() == '"')
					val.erase(0, 1);
				if (!val.empty() && val.back() == '"')
					val.pop_back();
				return val;
			}
		}
	}
	return {};
}

void parse_meminfo_field(const std::string& content, const std::string& field, int& out_mb)
{
	std::istringstream iss(content);
	std::string line;
	out_mb = 0;
	while (getline(iss, line))
	{
		if (line.find(field) == 0)
		{
			std::istringstream ls(line);
			std::string key;
			int value_kb;
			ls >> key >> value_kb;
			out_mb = value_kb / 1024;
			break;
		}
	}
}

void get_mem_info(int& mem_free_mb, int& mem_total_mb)
{
	std::string content = read_file_content("/proc/meminfo");
	parse_meminfo_field(content, "MemFree:", mem_free_mb);
	parse_meminfo_field(content, "MemTotal:", mem_total_mb);
}

void get_swap_info(int& swap_total_mb, int& swap_free_mb)
{
	std::string content = read_file_content("/proc/meminfo");
	parse_meminfo_field(content, "SwapTotal:", swap_total_mb);
	parse_meminfo_field(content, "SwapFree:", swap_free_mb);
}

int get_virtual_mem()
{
	std::string content = read_file_content("/proc/meminfo");
	int vm_total_mb = 0;
	parse_meminfo_field(content, "VmallocTotal:", vm_total_mb);
	return vm_total_mb;
}

FileDesc open_proc_mounts()
{
	return FileDesc("/proc/mounts", O_RDONLY);
}

void print_drives_fd(const FileDesc& fd)
{
	constexpr size_t buf_size = 4096;
	std::vector<char> buffer(buf_size);
	std::string all_data;
	ssize_t bytes_read;
	while ((bytes_read = fd.Read(buffer.data(), buf_size)) > 0)
	{
		all_data.append(buffer.data(), static_cast<size_t>(bytes_read));
	}
	if (bytes_read < 0)
	{
		perror("Read /proc/mounts");
		return;
	}

	FILE* mounts_stream = fmemopen(all_data.data(), all_data.size(), "r");
	if (!mounts_stream)
	{
		perror("fmemopen");
		return;
	}

	std::cout << "Drives:\n";
	mntent* mnt;
	while ((mnt = getmntent(mounts_stream)) != nullptr)
	{
		struct statvfs vfs{};
		if (statvfs(mnt->mnt_dir, &vfs) == 0)
		{
			unsigned long long total_bytes = vfs.f_blocks * vfs.f_frsize;
			unsigned long long free_bytes = vfs.f_bfree * vfs.f_frsize;
			unsigned long long total_gb = total_bytes / (1024ULL * 1024 * 1024);
			unsigned long long free_gb = free_bytes / (1024ULL * 1024 * 1024);

			std::cout << "  " << mnt->mnt_dir << "\t" << mnt->mnt_type << "\t"
				 << free_gb << "GB free / " << total_gb << "GB total\n";
		}
	}
	fclose(mounts_stream);
}

int main()
{
	utsname uts{};
	if (uname(&uts) < 0)
	{
		perror("uname");
		return 1;
	}

	std::string os_release;
	try
	{
		os_release = get_os_release();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading /etc/os-release: " << e.what() << "\n";
		os_release = "unknown";
	}

	std::string arch = uts.machine;

	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)) < 0)
	{
		perror("gethostname");
		return 1;
	}

	char* login = getlogin();
	std::string username;
	if (login)
		username = login;
	else
	{
		passwd* pw = getpwuid(getuid());
		if (pw)
			username = pw->pw_name;
		else
			username = "unknown";
	}

	int mem_free_mb = 0, mem_total_mb = 0;
	try
	{
		get_mem_info(mem_free_mb, mem_total_mb);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading memory info: " << e.what() << "\n";
	}

	int swap_total_mb = 0, swap_free_mb = 0;
	try
	{
		get_swap_info(swap_total_mb, swap_free_mb);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading swap info: " << e.what() << "\n";
	}

	int vm_total_mb = 0;
	try
	{
		vm_total_mb = get_virtual_mem();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error reading virtual memory info: " << e.what() << "\n";
	}

	int logical_processors = get_nprocs();

	struct sysinfo si{};
	if (sysinfo(&si) < 0)
	{
		perror("sysinfo");
		return 1;
	}

	const double load1 = si.loads[0] / 65536.0;
	const double load5 = si.loads[1] / 65536.0;
	const double load15 = si.loads[2] / 65536.0;

	std::cout << "OS: " << os_release << "\n";
	std::cout << "Kernel: " << uts.sysname << " " << uts.release << "\n";
	std::cout << "Architecture: " << arch << "\n";
	std::cout << "Hostname: " << hostname << "\n";
	std::cout << "User: " << username << "\n";
	std::cout << "RAM: " << mem_free_mb << "MB free / " << mem_total_mb << "MB total\n";
	std::cout << "Swap: " << swap_total_mb << "MB total / " << swap_free_mb << "MB free\n";
	std::cout << "Virtual memory: " << vm_total_mb << " MB\n";
	std::cout << "Processors: " << logical_processors << "\n";
	std::cout << "Load average: " << load1 << ", " << load5 << ", " << load15 << "\n";

	try
	{
		const FileDesc mounts_fd = open_proc_mounts();
		print_drives_fd(mounts_fd);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error opening /proc/mounts: " << e.what() << "\n";
	}

	return 0;
}

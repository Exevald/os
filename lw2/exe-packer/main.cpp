#include "FileDesc.h"
#include "Packer.h"
#include "Unpacker.h"
#include "Utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <zlib.h>

namespace
{
std::string GetSelfPath()
{
	char buffer[1024];
	const ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
	if (len == -1)
	{
		throw std::system_error(errno, std::generic_category(), "readlink /proc/self/exe failed");
	}
	buffer[len] = '\0';
	return buffer;
}

bool HasPayload(const std::string& selfPath)
{
	try
	{
		const FileDesc payloadFileDesc(selfPath.c_str(), O_RDONLY);
		struct stat payloadFileStat{};
		if (fstat(payloadFileDesc.Get(), &payloadFileStat) != 0 || static_cast<size_t>(payloadFileStat.st_size) < HeaderSize)
		{
			return false;
		}

		if (const off_t offset = payloadFileStat.st_size - static_cast<off_t>(HeaderSize);
			lseek(payloadFileDesc.Get(), offset, SEEK_SET) == -1)
		{
			return false;
		}

		SFXHeader header{};
		const ssize_t bytesRead = payloadFileDesc.Read(&header, HeaderSize);
		return (bytesRead == static_cast<ssize_t>(HeaderSize)) && (std::memcmp(header.signature, SFXSignature, 4) == 0);
	}
	catch (...)
	{
		return false;
	}
}

void RunUnpackMode(const std::string& selfPath, char* argv[])
{
	const Unpacker unpacker(selfPath);
	unpacker.Unpack(argv);
}

void RunPackMode(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage (pack mode): " << argv[0] << " <input_exe> <output_sfx>\n";
		throw std::invalid_argument("invalid arguments");
	}
	const Packer packer(argv[1], argv[2]);
	packer.Pack();
}
} // namespace

int main(int argc, char* argv[])
{
	try
	{
		if (const std::string selfPath = GetSelfPath(); HasPayload(selfPath))
		{
			RunUnpackMode(selfPath, argv);
		}
		else
		{
			RunPackMode(argc, argv);
		}
	}
	catch (const std::invalid_argument& invalidArgumentException)
	{
		std::cerr << invalidArgumentException.what() << std::endl;
		return EXIT_FAILURE;
	}
	catch (const std::exception& exception)
	{
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
#include "Packer.h"
#include "FileDesc.h"
#include "Utils.h"

#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <zlib.h>

namespace
{
std::vector<uint8_t> ReadFile(const std::string& path)
{
	const FileDesc fileDesc(path.c_str(), O_RDONLY);
	std::vector<uint8_t> fileData;

	constexpr size_t bufferSize = 64 * 1024; // 64 KiB
	std::vector<uint8_t> buffer(bufferSize);

	while (true)
	{
		const ssize_t bytesRead = fileDesc.Read(buffer.data(), bufferSize);
		if (bytesRead == 0)
		{
			break;
		}
		fileData.insert(fileData.end(), buffer.begin(), buffer.begin() + bytesRead);
	}

	return fileData;
}

void WriteFile(const std::string& path, const std::vector<uint8_t>& data)
{
	const FileDesc fileDesc(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (!data.empty())
	{
		if (const auto bytesWritten = fileDesc.Write(data.data(), data.size()); bytesWritten != data.size())
		{
			throw std::system_error(errno, std::generic_category());
		}
	}
}

std::vector<uint8_t> CompressData(const std::vector<uint8_t>& input)
{
	uLongf compressedSize = compressBound(input.size());
	std::vector<uint8_t> output(compressedSize);
	if (const int res = compress2(output.data(), &compressedSize, input.data(), input.size(), Z_BEST_COMPRESSION);
		res != Z_OK)
	{
		throw std::runtime_error("zlib compression failed");
	}
	output.resize(compressedSize);

	return output;
}
} // namespace

void Packer::Pack() const
{
	const auto original = ReadFile(m_input);
	auto compressed = CompressData(original);

	char bufferSize	[1024];
	const ssize_t len = readlink("/proc/self/exe", bufferSize, sizeof(bufferSize) - 1);
	if (len == -1)
	{
		throw std::system_error(errno, std::generic_category(), "readlink failed");
	}
	bufferSize[len] = '\0';

	const auto selfBinary = ReadFile(bufferSize);

	std::vector<uint8_t> output = selfBinary;
	output.insert(output.end(), compressed.begin(), compressed.end());

	SFXHeader header{};
	std::memcpy(header.signature, SFXSignature, 4);
	header.originalSize = original.size();
	header.compressedSize = compressed.size();

	output.insert(output.end(), reinterpret_cast<uint8_t*>(&header), reinterpret_cast<uint8_t*>(&header) + HeaderSize);

	WriteFile(m_outputSfx, output);

	if (chmod(m_outputSfx.c_str(), 0755) != 0)
	{
		throw std::system_error(errno, std::generic_category(), "chmod output failed");
	}

	std::cout << "SFX archive created: " << m_outputSfx << "\n";
}
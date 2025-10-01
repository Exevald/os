#include "Unpacker.h"
#include "FileDesc.h"

#include <cstring>
#include <sys/stat.h>
#include <sys/wait.h>
#include <zlib.h>

namespace
{
std::vector<uint8_t> DecompressData(const std::vector<uint8_t>& input, size_t origSize)
{
	std::vector<uint8_t> output(origSize);
	auto decompressedFileSize = origSize;
	if (const int res = uncompress(output.data(), &decompressedFileSize, input.data(), input.size());
		res != Z_OK || decompressedFileSize != origSize)
	{
		throw std::runtime_error("zlib decompression failed");
	}

	return output;
}

std::string CreateTempExecutable(const std::vector<uint8_t>& data)
{
	char tempPath[] = "/tmp/exe-packer-XXXXXX";
	const int fd = mkstemp(tempPath);
	if (fd == -1)
	{
		throw std::system_error(errno, std::generic_category(), "mkstemp failed");
	}

	FileDesc tempFileDesc(fd);
	if (const auto bytesWritten = tempFileDesc.Write(data.data(), data.size());
		bytesWritten != data.size())
	{
		throw std::system_error(errno, std::generic_category(), "write failed");
	}
	tempFileDesc.Close();

	if (chmod(tempPath, 0700) != 0)
	{
		unlink(tempPath);
		throw std::system_error(errno, std::generic_category(), "chmod failed");
	}

	return tempPath;
}

[[noreturn]] void ExecuteAndWait(const std::string& path, char* argv[])
{
	const pid_t pid = fork();
	if (pid == 0)
	{
		execv(path.c_str(), argv);
		perror("execv failed");
		_exit(127);
	}
	if (pid > 0)
	{
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status))
		{
			exit(WEXITSTATUS(status));
		}
		if (WIFSIGNALED(status))
		{
			raise(WTERMSIG(status));
			exit(128 + WTERMSIG(status));
		}
		exit(1);
	}

	throw std::system_error(errno, std::generic_category(), "fork failed");
}
} // namespace

void Unpacker::Unpack(char* argv[]) const
{
	const auto compressed = ReadPayload();
	const auto original = DecompressData(compressed, m_header.originalSize);
	const std::string tempPath = CreateTempExecutable(original);
	ExecuteAndWait(tempPath, argv);
}

void Unpacker::LoadHeader()
{
	const FileDesc payloadFileDesc(m_selfPath.c_str(), O_RDONLY);
	struct stat payloadFileStat{};
	if (fstat(payloadFileDesc.Get(), &payloadFileStat) != 0)
	{
		throw std::system_error(errno, std::generic_category(), "fstat failed");
	}

	if (static_cast<size_t>(payloadFileStat.st_size) < HeaderSize)
	{
		throw std::runtime_error("File too small to contain SFX header");
	}

	if (const off_t offset = payloadFileStat.st_size - static_cast<off_t>(HeaderSize);
		lseek(payloadFileDesc.Get(), offset, SEEK_SET) == -1)
	{
		throw std::system_error(errno, std::generic_category(), "lseek failed");
	}

	if (const ssize_t bytesRead = payloadFileDesc.Read(&m_header, HeaderSize);
		bytesRead != static_cast<ssize_t>(HeaderSize))
	{
		throw std::runtime_error("Failed to read SFX header");
	}
	if (std::memcmp(m_header.signature, SFXSignature, 4) != 0)
	{
		throw std::runtime_error("Invalid SFX signature");
	}
}

std::vector<uint8_t> Unpacker::ReadPayload() const
{
	const FileDesc payloadFileDesc(m_selfPath.c_str(), O_RDONLY);
	struct stat payloadFileStat{};
	fstat(payloadFileDesc.Get(), &payloadFileStat);
	const auto fileSize = static_cast<size_t>(payloadFileStat.st_size);

	if (const size_t payloadStart = fileSize - HeaderSize - m_header.compressedSize;
		lseek(payloadFileDesc.Get(), static_cast<off_t>(payloadStart), SEEK_SET) == -1)
	{
		throw std::system_error(errno, std::generic_category(), "lseek to payload failed");
	}

	std::vector<uint8_t> data(m_header.compressedSize);
	ssize_t compressedBytesRead = 0;
	while (compressedBytesRead < static_cast<ssize_t>(m_header.compressedSize))
	{
		const ssize_t bytesRead = payloadFileDesc.Read(data.data() + compressedBytesRead, m_header.compressedSize - compressedBytesRead);
		if (bytesRead <= 0)
		{
			throw std::runtime_error("Failed to read payload");
		}
		compressedBytesRead += bytesRead;
	}

	return data;
}
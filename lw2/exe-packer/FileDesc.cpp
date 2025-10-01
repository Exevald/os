#include "FileDesc.h"

#include <fcntl.h>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <utility>

FileDesc::FileDesc(int desc)
	: m_desc(desc == InvalidDesc || desc >= 0
			  ? desc
			  : throw std::invalid_argument("invalid file descriptor"))
{
}

FileDesc::FileDesc(const char* pathname, int flags, mode_t mode)
	: m_desc(-1)
{
	if (mode == 0)
	{
		m_desc = open(pathname, flags);
	}
	else
	{
		m_desc = open(pathname, flags, mode);
	}
	if (m_desc == -1)
	{
		throw std::system_error(errno, std::generic_category(), "Failed to open file");
	}
}

FileDesc::~FileDesc() noexcept
{
	try
	{
		Close();
	}
	catch (...)
	{
	}
}

FileDesc::FileDesc(FileDesc&& other) noexcept
	: m_desc(std::exchange(other.m_desc, InvalidDesc))
{
}

FileDesc& FileDesc::operator=(FileDesc&& other) noexcept
{
	if (this != &other)
	{
		Close();
		m_desc = std::exchange(other.m_desc, InvalidDesc);
	}
	return *this;
}

bool FileDesc::IsOpen() const
{
	return m_desc != InvalidDesc;
}

void FileDesc::Open(const char* pathname, int flags)
{
	Close();
	if ((m_desc = open(pathname, flags)) == -1)
	{
		throw std::system_error(errno, std::generic_category());
	}
}

void FileDesc::Close()
{
	if (m_desc != InvalidDesc)
	{
		if (close(m_desc) != 0)
		{
			throw std::system_error(errno, std::generic_category());
		}
		m_desc = InvalidDesc;
	}
}
int FileDesc::Get() const
{
	return m_desc;
}

void FileDesc::Swap(FileDesc& other) noexcept
{
	std::swap(m_desc, other.m_desc);
}

ssize_t FileDesc::Read(void* buffer, size_t length) const
{
	EnsureOpen();
	if (const auto bytesRead = read(m_desc, buffer, length); bytesRead != -1)
	{
		return bytesRead;
	}
	throw std::system_error(errno, std::generic_category());
}

ssize_t FileDesc::Write(const void* buffer, size_t length) const
{
	EnsureOpen();
	if (const auto bytesWritten = write(m_desc, buffer, length); bytesWritten != -1)
	{
		return bytesWritten;
	}
	throw std::system_error(errno, std::generic_category());
}

void FileDesc::EnsureOpen() const
{
	if (!IsOpen())
	{
		throw std::logic_error("file descriptor is not open");
	}
}
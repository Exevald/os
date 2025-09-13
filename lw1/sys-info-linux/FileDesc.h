#pragma once

#include <fcntl.h>
#include <sys/types.h>

class FileDesc
{
	constexpr static int InvalidDesc = -1;

public:
	FileDesc() = default;
	explicit FileDesc(int desc);
	explicit FileDesc(const char* pathname, int flags, mode_t mode = 0);
	~FileDesc() noexcept;

	FileDesc(const FileDesc&) = delete;
	FileDesc& operator=(const FileDesc&) = delete;

	FileDesc(FileDesc&& other) noexcept;
	FileDesc& operator=(FileDesc&& other) noexcept;

	[[nodiscard]] bool IsOpen() const;
	void Open(const char* pathname, int flags);
	void Close();

	void Swap(FileDesc& other) noexcept;

	ssize_t Read(void* buffer, size_t length) const;
	ssize_t Write(const void* buffer, size_t length) const;

private:
	void EnsureOpen() const;

	int m_desc = InvalidDesc;
};

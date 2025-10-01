#pragma once

#include "Utils.h"
#include <string>
#include <vector>

class Unpacker
{
public:
	explicit Unpacker(std::string selfPath)
		: m_selfPath(std::move(selfPath))
	{
		LoadHeader();
	}

	void Unpack(char* argv[]) const;

private:
	void LoadHeader();
	[[nodiscard]] std::vector<uint8_t> ReadPayload() const;

	std::string m_selfPath;
	SFXHeader m_header{};
};
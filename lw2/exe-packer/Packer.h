#pragma once

#include <iostream>
#include <string>

class Packer
{
public:
	Packer(std::string inputExe, std::string outputSfx)
		: m_input(std::move(inputExe))
		, m_outputSfx(std::move(outputSfx))
	{
	}

	void Pack() const;

private:
	std::string m_input;
	std::string m_outputSfx;
};
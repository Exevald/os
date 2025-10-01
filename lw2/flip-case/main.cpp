#include "FileDesc.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <vector>

struct Args
{
	std::vector<std::string> inputFiles;
};

std::optional<Args> ParseArgs(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " input1.txt input2.txt ..." << std::endl
				  << "For each input file, creates <file>.out with ASCII letters case flipped." << std::endl
				  << "If output file exists, it will be overwritten." << std::endl;
		return std::nullopt;
	}

	Args args;
	args.inputFiles.reserve(argc - 1);
	for (int i = 1; i < argc; ++i)
	{
		args.inputFiles.emplace_back(argv[i]);
	}

	return args;
}

void FlipASCIICase(char* buffer, ssize_t length)
{
	for (ssize_t i = 0; i < length; ++i)
	{
		if (const auto c = static_cast<unsigned char>(buffer[i]); c >= 'a' && c <= 'z')
		{
			buffer[i] = static_cast<char>(c - 'a' + 'A');
		}
		else if (c >= 'A' && c <= 'Z')
		{
			buffer[i] = static_cast<char>(c - 'A' + 'a');
		}
	}
}

void CopyAndFlipCase(const FileDesc& input, const FileDesc& output)
{
	constexpr size_t bufferSize = 64 * 1024; // 64 KiB
	char buffer[bufferSize];

	while (true)
	{
		const ssize_t bytesRead = input.Read(buffer, sizeof(buffer));
		if (bytesRead == 0)
		{
			break;
		}
		if (bytesRead == -1)
		{
			throw std::system_error(errno, std::generic_category(), "read failed");
		}

		FlipASCIICase(buffer, bytesRead);

		ssize_t bytesWritten = 0;
		while (bytesWritten < bytesRead)
		{
			const ssize_t chunk = output.Write(buffer + bytesWritten, static_cast<size_t>(bytesRead - bytesWritten));
			if (chunk <= 0)
			{
				if (errno == EINTR)
				{
					continue;
				}
				throw std::system_error(errno, std::generic_category(), "write failed");
			}
			bytesWritten += chunk;
		}
	}
}

[[noreturn]] void CreateAndRunChildProcess(const char* inputPath)
{
	const pid_t pid = getpid();
	std::cout << "Process " << pid << " is processing " << inputPath << '\n';

	try
	{
		const FileDesc inputFileDesc(inputPath, O_RDONLY);
		const std::string outFilePath = std::string(inputPath) + ".out";
		const FileDesc outFileDesc(outFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

		CopyAndFlipCase(inputFileDesc, outFileDesc);

		std::cout << "Process " << pid << " has finished writing to " << outFilePath << '\n';
		_exit(0);
	}
	catch (const std::system_error& error)
	{
		std::cerr << "Error processing file '" << inputPath << "': " << error.what() << '\n';
		_exit(1);
	}
	catch (...)
	{
		std::cerr << "Unknown error processing file '" << inputPath << "'\n";
		_exit(1);
	}
}

bool LaunchChildProcesses(const std::vector<std::string>& inputFiles, std::vector<pid_t>& childPids)
{
	bool hadError = false;
	for (const auto& inputFile : inputFiles)
	{
		if (!std::flush(std::cout) || !std::flush(std::cerr))
		{
			return true;
		}
		if (pid_t pid = fork(); pid == -1)
		{
			std::cerr << "Error creating process for file '" << inputFile << "': " << strerror(errno) << '\n';
			hadError = true;
		}
		else if (pid == 0)
		{
			CreateAndRunChildProcess(inputFile.c_str());
		}
		else
		{
			childPids.push_back(pid);
		}
	}

	return hadError;
}

bool WaitForAllChildren(const std::vector<pid_t>& childPids)
{
	size_t collected = 0;
	bool hadError = false;

	while (collected < childPids.size())
	{
		int status;
		const pid_t pid = waitpid(-1, &status, 0);
		if (pid == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			if (errno == ECHILD)
			{
				break;
			}
			std::cerr << "waiting process error: " << strerror(errno) << '\n';
			break;
		}

		std::cerr << "Child process " << pid << " is over.\n";
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
			hadError = true;
		}
		++collected;
	}

	return hadError;
}

int main(int argc, char* argv[])
{
	const auto args = ParseArgs(argc, argv);
	if (!args)
	{
		return 2;
	}

	std::vector<pid_t> childPids;
	const bool launchError = LaunchChildProcesses(args->inputFiles, childPids);
	const bool waitError = WaitForAllChildren(childPids);

	return (launchError || waitError) ? 1 : 0;
}
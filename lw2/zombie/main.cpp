#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
	const pid_t childPID = fork();
	if (childPID == -1)
	{
		std::cerr << "Error: fork() failed: " << std::strerror(errno) << std::endl;
		return EXIT_FAILURE;
	}
	if (childPID == 0)
	{
		_exit(EXIT_SUCCESS);
	}

	std::cout << "Child process has exited and is now a zombie." << std::endl
			  << "Enter the zombie PID to reap it: " << std::endl;

	pid_t userPID;
	int status;

	while (true)
	{
		std::cin >> userPID;
		if (const pid_t result = waitpid(userPID, &status, WNOHANG); result == -1)
		{
			if (errno == ECHILD)
			{
				std::cout << "Not a child of this process or already reaped. Try again: ";
			}
			else
			{
				std::cout << "waitpid error: " << std::strerror(errno) << ". Try again: ";
			}
		}
		else if (result == 0)
		{
			std::cout << "Process is not a zombie (or not our child). Try again: ";
		}
		else
		{
			std::cout << "Successfully reaped zombie with PID " << result << std::endl;
			break;
		}
	}

	return EXIT_SUCCESS;
}
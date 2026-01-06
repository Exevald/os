#pragma once

#include "Connection.h"

#include <cstdint>
#include <iostream>
#include <string>

struct ClientMode
{
	std::string address;
	uint16_t port;
};

inline void Run(const ClientMode& mode)
{
	try
	{
		boost::asio::io_context io_context;
		Connection connection(io_context, mode.address, mode.port);

		std::cout << "Connected to server at " << mode.address << ":" << mode.port << std::endl;

		std::string command;
		while (std::getline(std::cin, command))
		{
			connection.Send(command);
			const auto response = connection.Receive();
			std::cout << "Server Response: " << response << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
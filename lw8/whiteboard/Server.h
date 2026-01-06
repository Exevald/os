#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

struct ServerMode
{
	uint16_t port;
};

inline int Calculate(const char sign, std::vector<int> const& numbers)
{
	int result;

	if (sign == '+')
	{
		result = std::reduce(numbers.begin(), numbers.end(), 0, std::plus());
	}
	else if (sign == '-')
	{
		result = std::reduce(numbers.begin() + 1, numbers.end(), numbers[0], std::minus());
	}
	else
	{
		throw std::runtime_error("Unknown sign");
	}

	return result;
}

inline int CalculateCommand(const std::string& command)
{
	auto stream = std::istringstream(command);
	char sign;
	stream >> sign;
	std::vector<int> numbers;
	int number;
	while (stream >> number)
	{
		numbers.push_back(number);
	}

	if (numbers.empty())
	{
		throw std::runtime_error("no numbers in command");
	}

	if (!stream.eof())
	{
		throw std::runtime_error("invalid command");
	}

	return Calculate(sign, numbers);
}

inline void HandleRequest(tcp::socket& socket, const std::string& command)
{
	int result;
	try
	{
		result = CalculateCommand(command);
	}
	catch (const std::exception& e)
	{
		const std::string errorMessage = &e.what()['\n'];
		boost::asio::write(socket, boost::asio::buffer(errorMessage));
		return;
	}
	const auto resultStr = std::to_string(result) + '\n';
	boost::asio::write(socket, boost::asio::buffer(resultStr));
}

inline void HandleClient(tcp::socket socket)
{
	boost::asio::streambuf buffer;
	boost::system::error_code error;
	while (true)
	{
		if (error == boost::asio::error::eof)
			break;
		if (error)
		{
			std::cerr << "Error reading from socket: " << error.message() << std::endl;
			break;
		}

		std::istream is(&buffer);
		std::string command;
		std::getline(is, command);
		HandleRequest(socket, command);
	}
}

inline void Run(const ServerMode& mode)
{
	boost::asio::io_context io_context;
	tcp::endpoint endpoint(tcp::v4(), mode.port);
	tcp::acceptor acceptor(io_context, endpoint);

	std::cout << "Listening to the port " << mode.port << std::endl;

	while (true)
	{
		tcp::socket clientSocket(io_context);
		acceptor.accept(clientSocket);
		std::cout << "Accepted a new connection." << std::endl;

		std::jthread{ HandleClient, std::move(clientSocket) }.detach();
	}
}

#pragma once

#include <boost/asio.hpp>
#include <stdexcept>
#include <string>

class Connection
{
public:
	Connection(boost::asio::io_context& io_context, const std::string& serverAddrStr, uint16_t port)
		: m_socket(io_context)
	{
		boost::asio::ip::tcp::resolver resolver(io_context);
		auto endpoints = resolver.resolve(serverAddrStr, std::to_string(port));
		boost::asio::connect(m_socket, endpoints);
	}

	void Send(const std::string& message)
	{
		boost::system::error_code ec;
		boost::asio::write(m_socket, boost::asio::buffer(message), ec);
		if (ec)
		{
			throw std::runtime_error("Failed to send message: " + ec.message());
		}
	}

	std::string Receive()
	{
		boost::asio::streambuf buffer;
		boost::system::error_code ec;
		boost::asio::read_until(m_socket, buffer, "\n", ec);
		if (ec && ec != boost::asio::error::eof)
		{
			throw std::runtime_error("Failed to receive message: " + ec.message());
		}

		std::istream is(&buffer);
		std::string message;
		std::getline(is, message);
		return message;
	}

private:
	boost::asio::ip::tcp::socket m_socket;
};
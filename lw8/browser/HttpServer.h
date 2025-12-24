#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using RequestHandler = std::function<void(
	const http::request<http::string_body>&,
	http::response<http::string_body>&)>;

class HttpServer
{
public:
	HttpServer(
		net::io_context& ioc,
		const tcp::endpoint& endpoint,
		RequestHandler handler,
		std::atomic<bool>& stopping);

	void run();

private:
	void doAccept();

	struct Session : public std::enable_shared_from_this<Session>
	{
		Session(tcp::socket&& socket, RequestHandler handler, std::atomic<bool>& stopping);
		void run();

	private:
		void doRead();
		void process();

		tcp::socket m_socket;
		beast::flat_buffer m_buffer;
		http::request<http::string_body> m_request;
		RequestHandler m_handler;
		std::atomic<bool>& m_stopping;
	};

	net::io_context& m_ioc;
	tcp::acceptor m_acceptor;
	RequestHandler m_handler;
	std::atomic<bool>& m_stopping;
};
#include "HttpServer.h"

HttpServer::HttpServer(
	net::io_context& ioc,
	const tcp::endpoint& endpoint,
	RequestHandler handler,
	std::atomic<bool>& stopping)
	: m_ioc(ioc)
	, m_acceptor(ioc)
	, m_handler(std::move(handler))
	, m_stopping(stopping)
{
	beast::error_code ec;
	m_acceptor.open(endpoint.protocol(), ec);
	if (ec) throw std::runtime_error("open: " + ec.message());
	m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
	if (ec) throw std::runtime_error("set_option: " + ec.message());
	m_acceptor.bind(endpoint, ec);
	if (ec) throw std::runtime_error("bind: " + ec.message());
	m_acceptor.listen(net::socket_base::max_listen_connections, ec);
	if (ec) throw std::runtime_error("listen: " + ec.message());
}

void HttpServer::run()
{
	doAccept();
}

void HttpServer::doAccept()
{
	if (m_stopping.load()) return;

	m_acceptor.async_accept(m_ioc,
		[this](beast::error_code ec, tcp::socket socket)
		{
			if (!ec)
			{
				std::make_shared<Session>(std::move(socket), m_handler, m_stopping)->run();
			}
			if (!m_stopping.load())
			{
				doAccept();
			}
		});
}

// === Session ===

HttpServer::Session::Session(tcp::socket&& socket, RequestHandler handler, std::atomic<bool>& stopping)
	: m_socket(std::move(socket))
	, m_handler(std::move(handler))
	, m_stopping(stopping)
{
}

void HttpServer::Session::run()
{
	doRead();
}

void HttpServer::Session::doRead()
{
	auto self = shared_from_this();
	http::async_read(m_socket, m_buffer, m_request,
		[self](beast::error_code ec, std::size_t)
		{
			if (!ec)
				self->process();
			// else — клиент закрыл соединение
		});
}

void HttpServer::Session::process()
{
	if (m_stopping.load())
	{
		http::response<http::string_body> res{http::status::service_unavailable, 11};
		res.set(http::field::server, "SearchEngine/1.0");
		res.body() = "Server shutting down";
		res.prepare_payload();
		http::async_write(m_socket, res, [self = shared_from_this()](auto, auto){});
		return;
	}

	http::response<http::string_body> res{http::status::ok, 11};
	res.set(http::field::server, "SearchEngine/1.0");
	try
	{
		m_handler(m_request, res);
	}
	catch (...)
	{
		res.result(http::status::internal_server_error);
		res.body() = "Internal error";
		res.prepare_payload();
	}

	auto self = shared_from_this();
	http::async_write(m_socket, res,
		[self](beast::error_code, std::size_t)
		{
			// Соединение закрывается после ответа (HTTP/1.1 без keep-alive)
		});
}
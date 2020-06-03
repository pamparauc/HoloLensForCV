#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

//------------------------------------------------------------------------------

// Echoes back all received WebSocket messages
void do_session(tcp::socket& socket)
{
	try
	{
		// Construct the stream by moving in the socket
		websocket::stream<tcp::socket> ws{ std::move(socket) };

		// Accept the websocket handshake
		ws.accept();

		for (;;)
		{
			// This buffer will hold the incoming message
			boost::beast::multi_buffer buffer;

			// Read a message
			ws.read(buffer);

			// Echo the message back
			ws.text(ws.got_text());
			ws.write(buffer.data());
		}
	}
	catch (boost::system::system_error const& se)
	{
		// This indicates that the session was closed
		if (se.code() != websocket::error::closed)
			;
	}
	catch (std::exception const& e)
	{

	}
}
	void start()
	{
		auto const address = boost::asio::ip::make_address("127.0.0.1");
		auto const port = static_cast<unsigned short>(8080);

		// The io_context is required for all I/O
		boost::asio::io_context ioc{ 1 };

		// The acceptor receives incoming connections
		tcp::acceptor acceptor{ ioc, {address, port} };
		for (;;)
		{
			// This will receive the new connection
			tcp::socket socket{ ioc };

			// Block until we get a connection
			acceptor.accept(socket);

			// Launch the session, transferring ownership of the socket
			std::thread{ std::bind(
				&do_session,
				std::move(socket)) }.detach();
		}
	}



#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <cstdint>
#include <algorithm>

std::vector<char> vBuffer(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket& socket)
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), [&](std::error_code error, std::size_t length)
		{
			std::cout << "starting read some" << std::endl;
			if (!error)
			{
				std::cout << "\n\nRead " << length << " bytes\n\n";
				for (int i = 0; i < length; i++)
					std::cout << vBuffer[i];

				GrabSomeData(socket);
			}
			std::cout << "ending read some" << std::endl;
		});
}


int main()
{
	asio::error_code error_code;
	asio::io_context context;
	asio::io_context::work idleWork(context);
	std::thread threadContext = std::thread([&]() {context.run(); });
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("165.227.134.195"), 80); //"51.38.81.49"), 80) ;
	asio::ip::tcp::socket socket(context);
	socket.connect(endpoint, error_code);

	if (!error_code)
	{
		std::cout << "connected! " << std::endl;
	}
	else
	{
		std::cout << "failed to connect to address \n" << error_code.message() << std::endl;
	}

	if (socket.is_open())
	{
		GrabSomeData(socket);
		std::cout << "starting request" << std::endl;
		std::string sRequest =
			"GET /index.html HHTP/1.1\r\n"
			"Host: david-barr.co.uk\r\n"
			"Connection: close\r\n\r\n";

		std::cout << "write some" << std::endl;
		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), error_code);

		std::cout << "leaving is open" << std::endl;
	}
	else
	{
		std::cout << "something \n" << error_code.message() << std::endl;
	}
	return 0;
}

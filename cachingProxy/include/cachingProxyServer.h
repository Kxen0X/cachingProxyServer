#pragma once

#include <iostream>
#include <unordered_map>
#include <list>
#include <string_view>

#include <net_common.h>
#include <TSqueue.h>
#include <session.h>


class ProxyServer{

public:

	ProxyServer(uint16_t port, std::string_view url) :ConnPort{port}, originURL{ url }, acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), ConnPort)) {

	}

	~ProxyServer() {
		Stop();
	}

	bool Start() {
		try {
			Start_acception();
			this->contextThread = std::thread([this]() {context.run(); });
		}
		catch (std::exception& ec) {
			std::cerr << ec.what() << std::endl;
			return 0;
		}
		std::cout << "STARTED" << std::endl;
		return 1;
	}
	void Stop() {
		context.stop();

		if (this->contextThread.joinable()) this->contextThread.join();

	}

private:
	void Start_acception() {
		this->acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket connSock) {

			if (!ec) {
				std::cout << connSock.remote_endpoint() << std::endl;

				std::make_shared<Session>(context, std::move(connSock), originURL, cache)->Start();
				 
			}
			else {
				std::cerr << ec.message() << std::endl;
			}




			Start_acception();
		});
	}

	

private:

	std::unordered_map<std::string, std::string> cache;
	
	std::string originURL;
	int ConnPort;


	asio::io_context context;
	std::thread contextThread;

	asio::ip::tcp::acceptor acceptor;

	std::deque<asio::ip::tcp::socket> connections;

	
};


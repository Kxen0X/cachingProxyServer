#pragma once

#include <iostream>
#include <unordered_map>
#include <list>
#include <string_view>

#include <net_common.h>
#include <session.h>


class ProxyServer{

public:

	ProxyServer(uint16_t port, std::string_view url) :ConnPort{port}, originURL{ url }, acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), ConnPort)) {
		asio::ip::tcp::resolver resolver(context);
		host = getHostFromOrigin();
		service = getProtocol();

		std::error_code ec;
		endp = resolver.resolve(host, service, ec);

		if (ec) {
			std::cerr << "Resolve Error: " << ec.message() << std::endl;
		}
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
	std::string getHostFromOrigin() {
		std::string host = originURL;
		size_t pos = host.find("://");
		if (pos != std::string::npos) {
			host = host.substr(pos + 3);
		}
		pos = host.find('/');
		if (pos != std::string::npos) {
			host = host.substr(0, pos);
		}
		return host;
	}

	std::string getProtocol() {
		size_t pos = originURL.find("://");
		std::string res;
		if (pos == std::string::npos) {
			std::cerr << "wtf" << std::endl;
			return res;
		}
		return originURL.substr(0, pos);

	}

	void Start_acception() {
		this->acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket connSock) {

			if (!ec) {
				std::cout << connSock.remote_endpoint() << std::endl;

				std::make_shared<Session>(context, std::move(connSock), cache, host, service, endp)->Start();
				 
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

	std::string host;
	std::string service;

	asio::io_context context;
	std::thread contextThread;

	asio::ip::tcp::acceptor acceptor;



	asio::ip::tcp::resolver::results_type endp;

};


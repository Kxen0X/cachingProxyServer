#pragma once

#include <net_common.h>
#include <session.h>
#include <LRUCache.h>



#ifdef _WIN32
#include <windows.h>

BOOL WINAPI consoleHandler(DWORD ctrlType) {
	if (ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_LOGOFF_EVENT || ctrlType == CTRL_SHUTDOWN_EVENT) {
		auto path = std::filesystem::temp_directory_path() / "caching-serverPort.port";
		if (std::filesystem::exists(path)) {
			std::filesystem::remove(path);
		}
		return TRUE;
	}
	return FALSE;
}
#endif

class ProxyServer{

public:

	ProxyServer(uint16_t port, std::string_view url) : cache(), ConnPort{ port }, originURL{ url }, acceptor(context), signals{ context, SIGINT, SIGTERM, }{
		#if defined(SIGHUP)
			signals.add(SIGHUP);
		#endif
		#ifdef _WIN32
					SetConsoleCtrlHandler(consoleHandler, TRUE);
		#endif
		asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), ConnPort);

		acceptor.open(endpoint.protocol());

		acceptor.set_option(asio::socket_base::reuse_address(true));

		acceptor.bind(endpoint);
		acceptor.listen();
		
		

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

		asio::co_spawn(this->context.get_executor(), [this]() -> asio::awaitable<void> {
	
			auto [ec, signal_number] = co_await signals.async_wait(asio::as_tuple(asio::use_awaitable));

			if (!ec) {
				std::cout << "\nStopping server gracefully..." << std::endl;
				removePortFile();
				std::error_code close_ec;
				acceptor.close(close_ec);
				context.stop();
			}
		}, asio::detached);

		
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
		if (isStopped.exchange(true)) return;

		removePortFile();
		std::error_code ec;
		signals.cancel(ec);
		if (acceptor.is_open()) {
			acceptor.close(ec);
		}
		context.stop();

		if (this->contextThread.joinable()) {
			this->contextThread.join();
			
		}

	}

	void Wait() {
		if (this->contextThread.joinable()) {
			this->contextThread.join();
		}
	}

private:
	void removePortFile() {
		auto path = std::filesystem::temp_directory_path() / "caching-serverPort.port";
		if (std::filesystem::exists(path)) {
			std::filesystem::remove(path);
		}
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
		res = originURL.substr(0, pos);
		std::ranges::transform(res, res.begin(), [](char c) {return std::tolower(c); });
		return res;

	}

	void Start_acception() {

		asio::co_spawn(context.get_executor(), [this]() -> asio::awaitable<void> {
			for (;;) {
				try {
					auto socket = co_await acceptor.async_accept(asio::use_awaitable);
					std::make_shared<Session>(context, std::move(socket), cache, host, service, endp)->Start();
				}
				catch (const std::system_error& e) {
					if (e.code() == asio::error::operation_aborted) {
						break;
					}
					std::cerr << "Accept error: " << e.what() << std::endl;
				}
			}
		}, asio::detached);

	}

	

private:

	LRUCache cache;
	
	std::string originURL;
	int ConnPort;

	std::string host;
	std::string service;

	asio::io_context context;
	std::thread contextThread;

	asio::ip::tcp::acceptor acceptor;

	asio::ip::tcp::resolver::results_type endp;


	asio::signal_set signals;

	std::atomic<bool> isStopped{ false };
};


#pragma once

#include <net_common.h>
#include <session.h>
#include <LRUCache.h>



#ifdef _WIN32
#include <windows.h>

inline BOOL WINAPI  consoleHandler(DWORD ctrlType) {
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

	ProxyServer(uint16_t, std::string_view);

	~ProxyServer();

	bool Start();

	void Stop();

	void Wait();

private:
	void removePortFile();
	

private:
	std::string getHostFromOrigin();

	std::string getProtocol();

	void Start_acception();
	

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


#pragma once

#include <net_common.h>
#include <LRUCache.h>



class Session: public std::enable_shared_from_this<Session> {
public:

	Session(asio::io_context&,
		asio::ip::tcp::socket,
		LRUCache&,
		const std::string&,
		const std::string&,
		const asio::ip::tcp::resolver::results_type&);


	void Start();

	asio::awaitable<void> ReadHeader();

	asio::awaitable<void> ReadBody(size_t);

	asio::awaitable<void> WriteDataToOrigin(std::string);

	asio::awaitable<void> ReadResponseFromOrigin();

	asio::awaitable<void> WriteResponceToClient(bool);

	void Close();
private:

	bool isClearingRequest(std::string_view);
	

	std::string getRoute();

	int getContentLength(std::string_view);

	asio::awaitable<void> handleRequest();
	std::string modifyHostHeader(const std::string&);

private:

	std::string host;
	std::string service;
	

	asio::ip::tcp::socket sock;
	asio::io_context &context;

	std::string RequestHeaders;
	std::string bodyData;
	std::string HTTPRequest;

	asio::ip::tcp::socket originSock;

	std::string requestToOrigin;
	std::string responseToClient;

	asio::ip::tcp::resolver::results_type endpoints;

	LRUCache &cache;
	bool incache;

	asio::ssl::context sslContext;
	asio::ssl::stream<asio::ip::tcp::socket> sslsock;


};
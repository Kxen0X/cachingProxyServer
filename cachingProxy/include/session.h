#pragma once

#include <net_common.h>




class Session: public std::enable_shared_from_this<Session> {
public:

	Session(asio::io_context& cnxt, asio::ip::tcp::socket s, std::string& URL, std::unordered_map<std::string, std::string>& c) : context(cnxt), sock(std::move(s)), originURL{ URL }, originSock(cnxt), cache{ c } {}


	void Start() {
		ReadHeader();
	}

	void ReadHeader() {

		auto self = shared_from_this();

		asio::async_read_until(sock, asio::dynamic_buffer(RequestHeaders, 8192), "\r\n\r\n", [self, this](std::error_code ec, size_t length) {
			if (!ec) {

				std::string headers = RequestHeaders.substr(0, length);

				size_t ContentLength = getContentLength(headers);
				if (ContentLength == 0) {
					this->HTTPRequest = this->RequestHeaders;
					this->handleRequest();
				}
				else {
					size_t remainDataSize = RequestHeaders.size() - length;
					ReadBody(ContentLength - remainDataSize);
				}
			}
			else {
				std::cerr << ec.message() << std::endl;
			}


			});
	}

	void ReadBody(size_t contentLength) {
		auto self = shared_from_this();

		asio::async_read(sock, asio::dynamic_buffer(bodyData), asio::transfer_exactly(contentLength), [self, this](std::error_code ec, size_t length) {
			if (!ec) {
				this->HTTPRequest = this->RequestHeaders + this->bodyData;
				this->handleRequest();
			}
			else {
				std::cerr << ec.message() << std::endl;
				
			}
		});
	}

	void WriteDataToOrigin(std::string request) {
		this->requestToOrigin = std::move(request);
		auto self = shared_from_this();

		auto resolver = std::make_shared<asio::ip::tcp::resolver>(context);

		std::string host = getHostFromOrigin();
		std::string service = getProtocol();

		resolver->async_resolve(host, service,
			[self, this, resolver](std::error_code ec, asio::ip::tcp::resolver::results_type endpoints) {
				if (!ec) {
					asio::async_connect(originSock, endpoints,
						[self, this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
							if (!ec) {
								asio::async_write(originSock, asio::buffer(requestToOrigin),
									[self, this](std::error_code ec, size_t length) {
										if (!ec) {
											ReadResponseFromOrigin();
										}
										else {
											std::cerr << "Write to origin Error: " << ec.message() << std::endl;
										}
									});
							}
							else {
								std::cerr << "Connect Error: " << ec.message() << std::endl;
							}
						});
				}
				else {
					std::cerr << "Resolve Error: " << ec.message() << std::endl;
				}
			});
	}

	void ReadResponseFromOrigin() {
		auto self = shared_from_this();

		asio::async_read(originSock, asio::dynamic_buffer(responseFromOrigin), [self, this](std::error_code ec, size_t length) {
			if (!ec || ec == asio::error::eof) {
				WriteResponceToClient();
			}
			else {
				std::cerr << ec.message() << std::endl;

			}
		});
	}

	void WriteResponceToClient() {
		auto self = shared_from_this();

		if (incache) {
			responseFromOrigin = cache.at(getRoute());
			size_t firstNewLine = responseFromOrigin.find("\r\n");
			if (firstNewLine != std::string::npos) {
				responseFromOrigin.insert(firstNewLine + 2, "X-Cache: HIT\r\n");
			}
		}
		else {
			cache.insert({ getRoute(), responseFromOrigin });
			size_t firstNewLine = responseFromOrigin.find("\r\n");
			if (firstNewLine != std::string::npos) {
				responseFromOrigin.insert(firstNewLine + 2, "X-Cache: MISS\r\n");
			}
			
		}

		asio::async_write(sock, asio::buffer(responseFromOrigin), [self, this](std::error_code ec, size_t length) {
			if (!ec) {
				Close();
			}
			else {
				std::cerr << ec.message() << std::endl;
			}
			});
	}
	void Close() {
		std::error_code ec;

		sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		sock.close(ec);

		originSock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		originSock.close(ec);
	}
private:

	std::string getProtocol() {
		size_t pos = originURL.find("://");
		std::string res;
		if (pos == std::string::npos) {
			std::cerr << "wtf" << std::endl;
			return res;
		}
		return originURL.substr(0, pos);
		

	}

	std::string getRoute() {
		size_t firstSpacePos = HTTPRequest.find(' ');
		size_t secondSpacePos = HTTPRequest.find(' ', firstSpacePos + 1);
		return HTTPRequest.substr(firstSpacePos + 1, secondSpacePos - firstSpacePos - 1);
	}

	int getContentLength(std::string headers) {
		std::transform(headers.begin(), headers.end(), headers.begin(), [](char c) { return std::tolower(c); });

		std::string target = "content-length:";

		size_t ct = headers.find(target);
		if (ct == std::string::npos) {
			return 0;
		}

		auto end_pos = headers.find_first_of("\r\n", ct);
		if (end_pos == std::string::npos) {
			end_pos = headers.size();
		}

		std::size_t val_start = ct + target.length();
		std::string value_str = headers.substr(val_start, end_pos - val_start);

		try {
			return std::stoul(value_str);
		}
		catch (...) {
			return 0;
		}
	}

	void handleRequest() {
		std::string route = getRoute();

		if (!cache.contains(route)) {
			incache = 0;
			std::string request = modifyHostHeader(this->HTTPRequest);

			WriteDataToOrigin(request);
		}
		else {
			incache = 1;
			WriteResponceToClient();
		}

		
	}

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

	std::string modifyHostHeader(const std::string& originalRequest) {
		std::string request = originalRequest;
		std::string hostTarget = "Host:";
		auto it = std::search(
			request.begin(), request.end(),
			hostTarget.begin(), hostTarget.end(),
			[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
		);

		std::string targetHost = getHostFromOrigin();

		if (it != request.end()) {
			size_t hostPos = std::distance(request.begin(), it);
			size_t endPos = request.find("\r\n", hostPos);
			if (endPos != std::string::npos) {
				request.replace(hostPos, endPos - hostPos, "Host: " + targetHost);
			}
		}
		else {
			size_t firstLineEnd = request.find("\r\n");
			if (firstLineEnd != std::string::npos) {
				request.insert(firstLineEnd + 2, "Host: " + targetHost + "\r\n");
			}
		}

		size_t connPos = request.find("Connection:");
		if (connPos != std::string::npos) {
			size_t endPos = request.find("\r\n", connPos);
			request.replace(connPos, endPos - connPos, "Connection: close");
		}
		else {
			size_t firstLineEnd = request.find("\r\n");
			if (firstLineEnd != std::string::npos) {
				request.insert(firstLineEnd + 2, "Connection: close\r\n");
			}
		}
		return request;
	}

private:


	asio::ip::tcp::socket sock;
	asio::io_context &context;

	std::string RequestHeaders;
	std::string bodyData;
	std::string HTTPRequest;


	std::string originURL;
	asio::io_context originContext;
	asio::ip::tcp::socket originSock;

	std::string requestToOrigin;
	std::string responseFromOrigin;



	std::unordered_map<std::string, std::string> &cache;
	bool incache;
};
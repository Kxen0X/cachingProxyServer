#include "session.h"

Session::Session(asio::io_context& cnxt,
	asio::ip::tcp::socket s,
	LRUCache& c,
	const std::string& hhost,
	const std::string& sservice,
	const asio::ip::tcp::resolver::results_type& eendp) : context(cnxt), sock(std::move(s)), originSock(cnxt), cache(c), host{ hhost }, service{ sservice }, endpoints{ eendp }, sslContext(asio::ssl::context::tls_client), sslsock(context, sslContext) {

	#ifdef _WIN32
		load_windows_system_certs(sslContext);
	#else
		sslContext.set_default_verify_paths(ec);
	#endif	

}

void Session::Start() {
	auto self = shared_from_this();

	asio::co_spawn(this->context.get_executor(), [this, self]() -> asio::awaitable<void> {
		try {
			co_await ReadHeader();
		}
		catch (std::system_error& ec) {
			std::cout << ec.what() << std::endl;
			Close();
		}
		}, asio::detached);

}

asio::awaitable<void> Session::ReadHeader() {
	std::error_code ec;
	size_t length = co_await asio::async_read_until(sock, asio::dynamic_buffer(RequestHeaders, 8192), "\r\n\r\n", asio::redirect_error(asio::use_awaitable, ec));
	if (ec == asio::error::eof || !ec) {

		std::string headers = RequestHeaders.substr(0, length);
		if (isClearingRequest(headers)) {
			cache.clear();
			std::cout << "cache cleared" << std::endl;

			co_await WriteResponceToClient(1);

		}
		else {

			size_t ContentLength = getContentLength(headers);
			if (ContentLength == 0) {
				this->HTTPRequest = this->RequestHeaders;


				co_await this->handleRequest();
			}
			else {
				size_t remainDataSize = RequestHeaders.size() - length;
				if (remainDataSize >= ContentLength) {
					this->HTTPRequest = RequestHeaders.substr(0, length + ContentLength);


					co_await this->handleRequest();
				}
				else {

					co_await ReadBody(ContentLength - remainDataSize);
				}
			}
		}
	}
}

asio::awaitable<void> Session::ReadBody(size_t contentLength) {
	std::error_code ec;

	size_t length = co_await asio::async_read(sock, asio::dynamic_buffer(bodyData), asio::transfer_exactly(contentLength), asio::redirect_error(asio::use_awaitable, ec));
	if (ec == asio::error::eof || !ec) {

		this->HTTPRequest = this->RequestHeaders + this->bodyData;

		co_await this->handleRequest();
	}

}

asio::awaitable<void> Session::WriteDataToOrigin(std::string request) {
	this->requestToOrigin = std::move(request);
	if (service != "https") {

		auto endpoint = co_await asio::async_connect(originSock, endpoints, asio::use_awaitable);

		size_t length = co_await asio::async_write(originSock, asio::buffer(requestToOrigin), asio::use_awaitable);
		co_await ReadResponseFromOrigin();



	}
	else {
		auto endpoint = co_await asio::async_connect(sslsock.lowest_layer(), endpoints, asio::use_awaitable);

		sslsock.lowest_layer().set_option(asio::ip::tcp::no_delay(true));
		sslsock.set_verify_mode(asio::ssl::verify_peer);
		sslsock.set_verify_callback(asio::ssl::host_name_verification(host));

		SSL_set_tlsext_host_name(sslsock.native_handle(), host.c_str());

		co_await sslsock.async_handshake(asio::ssl::stream<asio::ip::tcp::socket>::client, asio::use_awaitable);

		size_t length = co_await asio::async_write(sslsock, asio::buffer(requestToOrigin), asio::use_awaitable);

		co_await ReadResponseFromOrigin();

	}

}

asio::awaitable<void> Session::ReadResponseFromOrigin() {
	if (service != "https") {
		std::error_code ec;

		size_t length = co_await asio::async_read(originSock, asio::dynamic_buffer(responseToClient), asio::redirect_error(asio::use_awaitable, ec));
		if (ec == asio::error::eof || !ec) {

			co_await WriteResponceToClient(0);
		}

	}
	else {

		std::error_code ec;

		size_t length = co_await asio::async_read(sslsock, asio::dynamic_buffer(responseToClient), asio::redirect_error(asio::use_awaitable, ec));
		if (ec == asio::error::eof || !ec) {

			co_await WriteResponceToClient(0);
		}

	}
}

asio::awaitable<void> Session::WriteResponceToClient(bool isClearing) {
	if (isClearing) {
		this->responseToClient = "HTTP/1.1 200 OK\r\nX_Clear-cache-header: 1\r\n\r\n";

		size_t length = co_await asio::async_write(sock, asio::buffer(this->responseToClient), asio::use_awaitable);

		Close();

	}
	else {
		if (incache) {
			this->responseToClient = cache.get(getRoute()).value();

			size_t length = co_await asio::async_write(sock, asio::buffer(this->responseToClient), asio::use_awaitable);

			Close();
		}
		else {
			std::string temp = responseToClient;
			size_t firstNewLine = temp.find("\r\n");
			if (firstNewLine != std::string::npos) {
				temp.insert(firstNewLine + 2, "X-Cache: HIT\r\n");
			}
			cache.add(getRoute(), temp);

			firstNewLine = responseToClient.find("\r\n");
			if (firstNewLine != std::string::npos) {
				responseToClient.insert(firstNewLine + 2, "X-Cache: MISS\r\n");
			}

			size_t length = co_await asio::async_write(sock, asio::buffer(responseToClient), asio::use_awaitable);
			Close();

		}
	}
}


void Session::Close() {
	std::error_code ec;
	if (sock.is_open()) {
		sock.cancel(ec);
		sock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		sock.close(ec);
	}
	if (originSock.is_open()) {
		originSock.cancel(ec);
		originSock.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		originSock.close(ec);
	}
	if (sslsock.lowest_layer().is_open()) {
		sslsock.lowest_layer().cancel(ec);
		sslsock.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		sslsock.lowest_layer().close();
	}
}

bool Session::isClearingRequest(std::string_view headers) {
	std::string target = "X_Clear-cache-header: 1";
	auto pos = std::search(headers.begin(), headers.end(), target.begin(), target.end(), [](char c1, char c2) { return std::tolower(c1) == std::tolower(c2); });
	return pos != headers.end();
}

std::string Session::getRoute() {
	size_t firstSpacePos = HTTPRequest.find(' ');
	if (firstSpacePos == std::string::npos) {
		return "";
	}

	size_t secondSpacePos = HTTPRequest.find(' ', firstSpacePos + 1);
	if (secondSpacePos == std::string::npos) {
		return "";
	}
	return HTTPRequest.substr(firstSpacePos + 1, secondSpacePos - firstSpacePos - 1);
}

int Session::getContentLength(std::string_view headers) {
	std::string target = "Content-Length:";

	auto it = std::search(
		headers.begin(), headers.end(),
		target.begin(), target.end(),
		[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
	);

	if (it != headers.end()) {
		size_t ct = std::distance(headers.begin(), it);

		auto end_pos = headers.find_first_of("\r\n", ct);
		if (end_pos == std::string::npos) {
			end_pos = headers.size();
		}
		std::size_t val_start = ct + target.length();
		std::string value_str(headers.substr(val_start, end_pos - val_start));

		try {
			return std::stoul(value_str);
		}
		catch (...) {
			return 0;
		}
	}

	return 0;

}

asio::awaitable<void> Session::handleRequest() {
	std::string route = getRoute();
	if (route.empty()) {
		std::cerr << "INVALID HTTP REQUEST" << std::endl;
		Close();
		co_return;
	}

	if (!cache.get(route).has_value()) {
		incache = 0;
		std::string request = modifyHostHeader(this->HTTPRequest);

		co_await WriteDataToOrigin(request);
	}
	else {
		incache = 1;
		co_await WriteResponceToClient(0);
	}


}

std::string Session::modifyHostHeader(const std::string& originalRequest) {
	std::string request = originalRequest;
	std::string hostTarget = "Host:";
	auto it = std::search(
		request.begin(), request.end(),
		hostTarget.begin(), hostTarget.end(),
		[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
	);

	std::string targetHost = host;

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

	std::string connTarget = "Connection:";
	it = std::search(
		request.begin(), request.end(),
		connTarget.begin(), connTarget.end(),
		[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
	);
	if (it != request.end()) {
		size_t connPos = std::distance(request.begin(), it);
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
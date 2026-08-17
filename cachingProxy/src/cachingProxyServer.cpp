#include "../include/cachingProxyServer.h"

std::filesystem::path getPortPath() {
    return std::filesystem::temp_directory_path() / "caching-serverPort.port";
}

bool savePort(uint16_t port) {
    std::ofstream out(getPortPath());

    if (!out.is_open()) {
        return false;
    }

    out << port;
    out.flush(); 

    if (out.fail()) {
        return false;
    }

    return true; 
}

std::optional<std::string> getPort() {
    std::filesystem::path path = getPortPath();

    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    std::ifstream in(path);
    std::string port;
    if (in >> port) {
        return port;
    }
    return std::nullopt;
}

void removePortFile() {
    std::filesystem::path path = getPortPath();
    if (std::filesystem::exists(path)) {
        std::cout << 1 << std::endl;
        std::filesystem::remove(path);
    }
}


int main(int argc, char* argv[])
{
	std::string_view port;
	std::string_view origin;
    bool clearCache = 0;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        
        if (arg == "--port" || arg == "--origin" || arg == "--clear-cache") {
            
            if (arg != "--clear-cache" && (i + 1 >= argc || std::string_view(argv[i + 1]).starts_with("--"))) {
                std::cerr << "Error: Flag " << arg << " requires a valid value!" << std::endl;
                std::cout << "Usage: caching-proxy --port <portNum> --origin <URL>" << std::endl;
                return 1;
            }

            if (arg == "--port") {
                port = argv[++i];
            }
            else if (arg == "--origin") {
                origin = argv[++i];
            }
            else {
                clearCache = 1;
            }
        }
        else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            return 1;
        }
    }
    
    if (!clearCache) {
        
        

        if (port.empty() && origin.empty()) {
            std::cerr << "Lack of arguments" << std::endl;
            return 1;
        }
        if (port.empty()) {
            std::cerr << "The port number must be entered" << std::endl;
            return 1;
        }
        if (origin.empty()) {
            std::cerr << "URL must be entered" << std::endl;
            return 1;
        }

        if (std::filesystem::exists(getPortPath())) {
            std::cerr << "This port is already in use by another server" << std::endl;
            return 1;
        }
        uint16_t portNum;

        auto [ptr, ec] = std::from_chars(port.data(), port.data() + port.size(), portNum);
        if (ec != std::errc{} || ptr != port.data() + port.size()) {
            std::cerr << "Invalid port format or size" << std::endl;
            return 1;
        }

        ProxyServer server(portNum, origin);

        if (!server.Start()) {
            removePortFile();
            std::cerr << "Failed to start server" << std::endl;
            return 1;
        }

        if(!savePort(portNum)){
            std::cerr << "WARNING: CANNOT CREATE FILE WITH PORT NUMBER, --clear-cache WILL NOT BE USABLE" << std::endl;

        }

        server.Wait();
    }
    else {
        if (!std::filesystem::exists(getPortPath())) {
            std::cerr << "The server is not running" << std::endl;
            return 1;
        }
        auto serversPort = getPort();
        if (serversPort.has_value()) {

            std::string request = "GET / HTTP/1.1\r\nHost: 127.0.0.1:" + serversPort.value() + "\r\nX_Clear-cache-header: 1\r\n\r\n";

            asio::io_context context;
            asio::ip::tcp::resolver resolver(context);
            

            std::error_code ec;
            auto endp = resolver.resolve("127.0.0.1", serversPort.value(), ec);

            if (ec) {
                std::cerr << "Resolve Error: " << ec.message() << std::endl;
                return 1;
            }

            asio::ip::tcp::socket socket(context);

            auto succEndp = asio::connect(socket, endp, ec);
            if (ec) {
                std::cerr << "Connection failed" << std::endl;
                return 1;
            }

            size_t cnt = asio::write(socket, asio::buffer(request), ec);

            if (ec) {
                std::cout << "Failed to send request" << std::endl;
                return 1;
            }


            if (cnt == request.size()) {
                std::string response;

                asio::read_until(socket, asio::dynamic_buffer(response), "\r\n\r\n", ec);

                if (ec && ec != asio::error::eof) {
                    std::cerr << "Failed to read response: " << ec.message() << std::endl;
                    return 1;
                }

                if (response == "HTTP/1.1 200 OK\r\nX_Clear-cache-header: 1\r\n\r\n") {
                    std::cout << "cache cleared" << std::endl;
                    socket.close();
                }
            }
            else {
                std::cerr << "Failed to send the request; try again" << std::endl;
                return 1;
            }

        }


    }
   

	return 0;
}

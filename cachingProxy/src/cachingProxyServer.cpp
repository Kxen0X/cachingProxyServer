#include "../include/cachingProxyServer.h"

int main(int argc, char* argv[])
{
	std::string_view port;
	std::string_view origin;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        
        if (arg == "--port" || arg == "--origin") {
            
            if (i + 1 >= argc || std::string_view(argv[i + 1]).starts_with("--")) {
                std::cerr << "Error: Flag " << arg << " requires a valid value!" << std::endl;
                return 1;
            }

            if (arg == "--port") {
                port = argv[++i];
            }
            else {
                origin = argv[++i];
            }
        }
        else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            return 1;
        }
    }

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
	
    uint16_t portNum;
    
    auto [ptr, ec] = std::from_chars(port.data(), port.data() + port.size(), portNum);
    if (ec != std::errc{} || ptr != port.data() + port.size()) {
        std::cerr << "Invalid port format or size" << std::endl;
        return 1;
    }

    ProxyServer server(portNum, origin);
    
    if (!server.Start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    server.Wait();

	return 0;
}

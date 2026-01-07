#include "server/GameServer.hpp"
#include <iostream>
#include <csignal>
#include <cstring>

mmorpg::GameServer* g_server = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    if (g_server) {
        g_server->shutdown();
    }
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "Options:\n"
              << "  -c, --config <file>  Load configuration from JSON file\n"
              << "  -p, --port <port>    Override server port\n"
              << "  -h, --help           Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=== MMORPG Game Server ===" << std::endl;

    // Default config
    mmorpg::GameServer::Config config;
    std::string configFile;
    uint16_t portOverride = 0;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                configFile = argv[++i];
            }
        } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                portOverride = static_cast<uint16_t>(std::atoi(argv[++i]));
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            // Legacy: first argument is port number
            portOverride = static_cast<uint16_t>(std::atoi(argv[i]));
        }
    }

    // Load config from file if specified
    if (!configFile.empty()) {
        config = mmorpg::GameServer::Config::loadFromFile(configFile);
    }

    // Override port if specified
    if (portOverride > 0) {
        config.port = portOverride;
    }

    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create and run server
    mmorpg::GameServer server(config);
    g_server = &server;

    if (!server.initialize()) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }

    server.run();

    std::cout << "Server exited cleanly" << std::endl;
    return 0;
}

#include <crow.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include "system_info.h"

using json = nlohmann::json;

int main()
{
    crow::SimpleApp app;

    // ---------------------------------------
    // Health endpoint
    // ---------------------------------------

    CROW_ROUTE(app, "/health")
    ([] {
        json response = {
            {"status", "UP"},
            {"service", "network-monitor"},
            {"version", "1.0.0"}
        };

        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");

        return res;
    });

    // ---------------------------------------
    // System information
    // ---------------------------------------

    CROW_ROUTE(app, "/system")
    ([] {
        json response = {
            {"hostname", get_hostname()},
            {"cpu_load", get_cpu_load()},
            {"memory_usage_percent", get_memory_usage()},
            {"uptime_seconds", get_uptime()}
        };

        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");

        return res;
    });

    // ---------------------------------------
    // Application statistics
    // ---------------------------------------

    CROW_ROUTE(app, "/stats")
    ([] {
        json response = {
            {"requests", "API statistics coming soon"},
            {"threads",
             std::thread::hardware_concurrency()}
        };

        crow::response res(response.dump());
        res.set_header("Content-Type", "application/json");

        return res;
    });

    // ---------------------------------------
    // Root endpoint
    // ---------------------------------------

    CROW_ROUTE(app, "/")
    ([] {
        return "Network Monitor API is running";
    });

    std::cout << "Network Monitor starting..." << std::endl;
    std::cout << "Listening on port 9000" << std::endl;

    app.port(9000)
       .multithreaded()
       .run();
}

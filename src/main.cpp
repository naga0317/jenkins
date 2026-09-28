#include <crow.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>

using json = nlohmann::json;

// Read a value from /proc/loadavg
double get_cpu_load()
{
    std::ifstream file("/proc/loadavg");

    if (!file)
        return -1.0;

    double load1;
    file >> load1;

    return load1;
}

// Read memory information from /proc/meminfo
double get_memory_usage()
{
    std::ifstream file("/proc/meminfo");

    if (!file)
        return -1.0;

    long mem_total = 0;
    long mem_available = 0;

    std::string key;
    long value;
    std::string unit;

    while (file >> key >> value >> unit)
    {
        if (key == "MemTotal:")
            mem_total = value;

        if (key == "MemAvailable:")
            mem_available = value;
    }

    if (mem_total == 0)
        return -1.0;

    return 100.0 *
           (static_cast<double>(mem_total - mem_available) /
            mem_total);
}

// Read system uptime
long get_uptime()
{
    std::ifstream file("/proc/uptime");

    if (!file)
        return -1;

    double uptime;

    file >> uptime;

    return static_cast<long>(uptime);
}

// Get hostname
std::string get_hostname()
{
    char hostname[256];

    if (gethostname(hostname, sizeof(hostname)) != 0)
        return "unknown";

    return hostname;
}

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

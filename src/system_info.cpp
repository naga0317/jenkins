#include "system_info.h"

#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

double get_cpu_load()
{
    std::ifstream file("/proc/loadavg");

    double load = 0.0;

    if (file)
    {
        file >> load;
    }

    return load;
}

double get_memory_usage()
{
    std::ifstream file("/proc/meminfo");

    std::string key;
    long long value;
    std::string unit;

    long long mem_total = 0;
    long long mem_available = 0;

    while (file >> key >> value >> unit)
    {
        if (key == "MemTotal:")
        {
            mem_total = value;
        }
        else if (key == "MemAvailable:")
        {
            mem_available = value;
        }
    }

    if (mem_total == 0)
    {
        return 0.0;
    }

    return 100.0 *
           static_cast<double>(mem_total - mem_available) /
           static_cast<double>(mem_total);
}

double get_uptime()
{
    std::ifstream file("/proc/uptime");

    double uptime = 0.0;

    if (file)
    {
        file >> uptime;
    }

    return uptime;
}

std::string get_hostname()
{
    char hostname[256] = {};

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        return std::string(hostname);
    }

    return "unknown";
}

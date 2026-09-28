#include <gtest/gtest.h>

#include "system_info.h"

TEST(SystemInfoTest, CpuLoadIsNonNegative)
{
    double load = get_cpu_load();

    EXPECT_GE(load, 0.0);
}

TEST(SystemInfoTest, MemoryUsageIsWithinValidRange)
{
    double memory = get_memory_usage();

    EXPECT_GE(memory, 0.0);
    EXPECT_LE(memory, 100.0);
}

TEST(SystemInfoTest, UptimeIsNonNegative)
{
    double uptime = get_uptime();

    EXPECT_GE(uptime, 0.0);
}

TEST(SystemInfoTest, HostnameIsNotEmpty)
{
    std::string hostname = get_hostname();

    EXPECT_FALSE(hostname.empty());
}

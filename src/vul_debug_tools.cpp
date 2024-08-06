#include "vul_device.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <chrono>
#include <iostream>
#include <vul_debug_tools.hpp>
#include <vulkan/vulkan_core.h>

struct Measurement {
    const char *name;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
};
std::vector<Measurement> measurements;

PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;
VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo) 
{
    return pfn_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

namespace vul {

namespace ProfAnalyzer {

void resetMeasurements()
{
    measurements.clear();
}

void dumpMeasurementTree(const std::string &fileName)
{
    std::vector<Measurement> sortedMeasurements = measurements;
    std::sort(sortedMeasurements.begin(), sortedMeasurements.end(), [](Measurement a, Measurement b){return a.start.time_since_epoch() < b.start.time_since_epoch();});

    std::vector<const Measurement *> previouses;
    int indent = 0;
    std::ofstream output(fileName);
    output << std::fixed << std::setprecision(3);
    for (const Measurement &meas : sortedMeasurements) {
        for (int64_t i = static_cast<int64_t>(previouses.size()) - 1; i >= 0; i--){
            if (previouses[i]->end.time_since_epoch() < meas.start.time_since_epoch()) {
                previouses.pop_back();
                indent--;
            }
        }

        int64_t timeInt = std::chrono::duration_cast<std::chrono::nanoseconds>(meas.end - meas.start).count();
        double time = static_cast<double>(timeInt) / 1000.0;
        output << std::string(indent * 4, ' ') << time << "us " << meas.name << "\n";

        previouses.push_back(&meas);
        indent++;
    }
}

void dumpMeasurementSummary(const std::string &fileName)
{
    struct Data {
        std::string name;
        int64_t time = 0;
        int64_t minTime = std::numeric_limits<int64_t>::max();
        int64_t maxTime = std::numeric_limits<int64_t>::min();
        std::vector<int64_t> times;
        void add(int64_t addTime)
        {
            time += addTime;
            minTime = std::min(minTime, addTime);
            maxTime = std::max(maxTime, addTime);
            times.push_back(addTime);
        }
    };
    std::unordered_map<const char *, Data> measurementMap;
    for (const Measurement &meas : measurements) {
        int64_t time = std::chrono::duration_cast<std::chrono::nanoseconds>(meas.end - meas.start).count();
        measurementMap[meas.name].add(time);
    }
    std::vector<Data> sortedData;
    size_t longestTime = 0;
    size_t longestCount = 0;
    size_t longestAvg = 0;
    size_t longestMedian = 0;
    size_t longestMax = 0;
    size_t longestMin = 0;
    for (std::pair<const char*, Data> kv : measurementMap) {
        longestTime = std::max(longestTime, std::to_string(kv.second.time).length());
        longestCount = std::max(longestCount, std::to_string(kv.second.times.size()).length());
        longestAvg = std::max(longestAvg, std::to_string(kv.second.time / kv.second.times.size()).length());
        longestMedian = std::max(longestMedian, std::to_string(kv.second.times[kv.second.times.size() / 2]).length());
        longestMax = std::max(longestMax, std::to_string(kv.second.maxTime).length());
        longestMin = std::max(longestMin, std::to_string(kv.second.minTime).length());
        sortedData.push_back(kv.second);
        sortedData[sortedData.size() - 1].name = kv.first;
    }
    std::sort(sortedData.begin(), sortedData.end(), [](Data a, Data b){return a.time > b.time;});

    std::ofstream output(fileName);
    output << std::fixed << std::setprecision(3);
    for (const Data &data : sortedData) {
        double timeInUs = static_cast<double>(data.time) / 1000.0;
        double avgTime = timeInUs / static_cast<double>(data.times.size());
        double medianTimeInUs = static_cast<double>(data.times[data.times.size() / 2]) / 1000.0;
        double maxTimeInUs = static_cast<double>(data.maxTime) / 1000.0;
        double minTimeInUs = static_cast<double>(data.minTime) / 1000.0;
        size_t timeLen = std::max(std::to_string(data.time).length(), 4ul);
        size_t countLen = std::to_string(data.times.size()).length();
        size_t avgLen = std::max(std::to_string(data.time / data.times.size()).length(), 4ul);
        size_t medianLen = std::max(std::to_string(data.times[data.times.size() / 2]).length(), 4ul);
        size_t maxLen = std::max(std::to_string(data.maxTime).length(), 4ul);
        size_t minLen = std::max(std::to_string(data.minTime).length(), 4ul);
        output << "Sum: " << timeInUs << "us " << std::string(longestTime - timeLen, ' ') << "Cnt: " << data.times.size() << std::string(longestCount - countLen + 1, ' ') << "Avg: " << avgTime << "us " << std::string(longestAvg - avgLen, ' ') << "Med: " << medianTimeInUs << "us " << std::string(longestMedian - medianLen, ' ') << "Max: " << maxTimeInUs << "us " << std::string(longestMax - maxLen, ' ') << "Min: " << minTimeInUs << "us " << std::string(longestMin - minLen, ' ') << data.name << "\n";
    }
}

}

ScopedTimer::ScopedTimer(const char *name)
{
    m_name = name;
    m_startTime = std::chrono::steady_clock::now();
}

ScopedTimer::~ScopedTimer()
{
    measurements.emplace_back(Measurement{m_name, m_startTime, std::chrono::steady_clock::now()});
}

}

namespace vul {

namespace DebugNamer {

VulDevice *vulDevice;
void initialize(VulDevice &device)
{
    vulDevice = &device;
    pfn_vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(device.getInstace(), "vkSetDebugUtilsObjectNameEXT"));
}

void setObjectName(const uint64_t object, const std::string &name, VkObjectType type)
{
    VkDebugUtilsObjectNameInfoEXT nameInfo{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, type, object, name.c_str()};
    vkSetDebugUtilsObjectNameEXT(vulDevice->device(), &nameInfo);
}

}

}

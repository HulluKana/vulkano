#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <vul_profiler.hpp>

struct Measurement {
    const char *name;
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point end;
};
std::vector<Measurement> measurements;

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

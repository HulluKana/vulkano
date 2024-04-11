#pragma once

#include <chrono>
#include <string>

#define COMBINE_THINGS_ASSISTANT(X,Y) X##Y
#define COMBINE_THINGS(X,Y) COMBINE_THINGS_ASSISTANT(X,Y)

#define VUL_PROFILE_SCOPE(name) vul::ScopedTimer COMBINE_THINGS(scopedTimer_, __LINE__)(name);
#define VUL_PROFILE_FUNC() VUL_PROFILE_SCOPE(__PRETTY_FUNCTION__)

namespace vul {

namespace ProfAnalyzer {

void resetMeasurements();
void dumpMeasurementTree(const std::string &fileName);

}
    
class ScopedTimer {
    public:
        ScopedTimer(const char *name);
        ~ScopedTimer();
    private:
        const char *m_name; 
        std::chrono::steady_clock::time_point m_startTime;
};

}

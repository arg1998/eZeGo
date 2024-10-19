#include "definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/assertion/assertions.hpp"

int main() {
    
    EZ_LOG_FATAL("fatal log %d %s", 123, " </fatal>");
    EZ_LOG_ERROR("error log %d %s", 456, "</error>");
    EZ_LOG_WARN("warn log %d %d %s", 789, false, "</warn>");
    EZ_LOG_INFO("info log %d %s", 123456, "</info>");
    EZ_LOG_DEBUG("debug log %d %s", 456789, "</debug>");
    EZ_LOG_TRACE("trace log %d %s", 789123, "</trace>");

    EZ_ASSERT(1 == 1);
    EZ_ASSERT_MSG(1 == 0, "why 1 is not equal to 0?");

    return 0;
}
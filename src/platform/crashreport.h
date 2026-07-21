#ifndef FOUR_WINDS_CRASHREPORT_H
#define FOUR_WINDS_CRASHREPORT_H

#include <cstdint>
#include <string>
#include <vector>

namespace CrashReport
{
    void install(const std::string & application);
    void shutdown(void);
    void breadcrumb(const std::string & message);
    std::uint64_t breadcrumbSequence(void);
    std::vector<std::string> recentBreadcrumbs(void);
    void reportException(const std::string & message);
    const std::string & filePath(void);
}

#endif

#ifndef FOUR_WINDS_CRASHREPORT_H
#define FOUR_WINDS_CRASHREPORT_H

#include <string>

namespace CrashReport
{
    void install(const std::string & application);
    void shutdown(void);
    void breadcrumb(const std::string & message);
    void reportException(const std::string & message);
    const std::string & filePath(void);
}

#endif

#include "crashreport.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <string>

#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "swe/swe_systems.h"

namespace
{
constexpr std::streamoff MaxReportSize = 4 * 1024 * 1024;

std::string reportPath;
std::ofstream reportStream;
bool failureReported = false;

#if defined(__APPLE__) || defined(__linux__)
int reportDescriptor = -1;
volatile std::sig_atomic_t handlingSignal = 0;
#endif

std::string timestamp(void)
{
    const std::time_t now = std::time(nullptr);
    std::tm local = {};

#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
    return buffer;
}

void appendLine(const std::string & message)
{
    if(reportStream.is_open())
        reportStream << timestamp() << " " << message << '\n';
}

#if defined(__APPLE__) || defined(__linux__)
void writeAll(const char* data, std::size_t size)
{
    while(reportDescriptor >= 0 && size)
    {
        const ssize_t written = ::write(reportDescriptor, data, size);
        if(written <= 0) return;
        data += written;
        size -= static_cast<std::size_t>(written);
    }
}

void writeSignalNumber(int value)
{
    char digits[16] = {};
    std::size_t count = 0;
    unsigned int number = value < 0 ? static_cast<unsigned int>(-value) :
                                      static_cast<unsigned int>(value);

    do
    {
        digits[count++] = static_cast<char>('0' + number % 10);
        number /= 10;
    }
    while(number && count < sizeof(digits));

    if(value < 0) digits[count++] = '-';
    while(count) writeAll(&digits[--count], 1);
}

void writeStackTrace(void)
{
    if(reportDescriptor < 0) return;

    void* frames[64] = {};
    const int count = ::backtrace(frames, 64);
    if(0 < count) ::backtrace_symbols_fd(frames, count, reportDescriptor);
}

void fatalSignalHandler(int signal)
{
    if(handlingSignal)
        _exit(128 + signal);

    handlingSignal = 1;
    static const char header[] = "\n[FATAL SIGNAL] signal=";
    static const char stack[] = "\nStack trace:\n";
    writeAll(header, sizeof(header) - 1);
    writeSignalNumber(signal);
    writeAll(stack, sizeof(stack) - 1);
    writeStackTrace();
    if(0 <= reportDescriptor) ::fsync(reportDescriptor);

    ::kill(::getpid(), signal);
    _exit(128 + signal);
}

void installSignalHandler(int signal)
{
    struct sigaction action = {};
    action.sa_handler = fatalSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESETHAND;
    sigaction(signal, &action, nullptr);
}
#endif
}

void CrashReport::install(const std::string & application)
{
    std::string directory;
    if(const char* overrideDirectory = SWE::Systems::environment("FOUR_WINDS_DIAGNOSTICS_DIR"))
        directory = overrideDirectory;
    else
        directory = SWE::Systems::homeDirectory(application);

    if(!directory.empty() && !SWE::Systems::isDirectory(directory))
        SWE::Systems::makeDirectory(directory);

    reportPath = directory.empty() ? std::string("crash-report.log") :
        SWE::Systems::concatePath(directory, "crash-report.log");

    std::ifstream previous(reportPath, std::ios::binary | std::ios::ate);
    if(previous.is_open() && MaxReportSize < previous.tellg())
    {
        previous.close();
        const std::string rotated = reportPath + ".previous";
        std::remove(rotated.c_str());
        std::rename(reportPath.c_str(), rotated.c_str());
    }

    reportStream.open(reportPath, std::ios::out | std::ios::app);
    if(reportStream.is_open()) reportStream.setf(std::ios::unitbuf);
    appendLine("[SESSION START]");

#if defined(__APPLE__) || defined(__linux__)
    reportDescriptor = ::open(reportPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    installSignalHandler(SIGABRT);
    installSignalHandler(SIGBUS);
    installSignalHandler(SIGFPE);
    installSignalHandler(SIGILL);
    installSignalHandler(SIGSEGV);
#endif

    std::set_terminate([]
    {
        CrashReport::reportException("std::terminate");
        std::abort();
    });
}

void CrashReport::shutdown(void)
{
    appendLine(failureReported ? "[SESSION END] failure" : "[SESSION END] clean shutdown");
    if(reportStream.is_open()) reportStream.close();

#if defined(__APPLE__) || defined(__linux__)
    if(0 <= reportDescriptor)
    {
        ::close(reportDescriptor);
        reportDescriptor = -1;
    }
#endif
}

void CrashReport::breadcrumb(const std::string & message)
{
    appendLine("[ACTION] " + message);
}

void CrashReport::reportException(const std::string & message)
{
    failureReported = true;
    appendLine("[FATAL EXCEPTION] " + message);

#if defined(__APPLE__) || defined(__linux__)
    if(0 <= reportDescriptor)
    {
        static const char stack[] = "Stack trace:\n";
        writeAll(stack, sizeof(stack) - 1);
        writeStackTrace();
        ::fsync(reportDescriptor);
    }
#endif
}

const std::string & CrashReport::filePath(void)
{
    return reportPath;
}

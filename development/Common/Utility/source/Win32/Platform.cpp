#include "YagetVersion.h"
#include "Platform/Support.h"
#include "Logger/YLog.h"
#include "StringHelpers.h"
#include "Debugging/Assert.h"
#include "Debugging/DevConfiguration.h"
#include "App/AppUtilities.h"
#include "Exception/Exception.h"
#include "Metrics/Concurrency.h"
#include "App/FileUtilities.h"
#include "Debugging/DevConfigurationParsers.h"

#include "Platform/WindowsLean.h"
#include "CommandLineArgs.inl"

#include <thread>
#include <random>
#include <locale>
#include <codecvt>
#include <list>
#include <comdef.h>
#include <filesystem>

namespace fs = std::filesystem;

using namespace yaget;

namespace
{
    // Support for GetSystemTime...AsFileTime() function on windows depends on version, so we rsolve at run time which function
    // is available and assign that value to GetPreciseSystemTime
    typedef void (WINAPI *FuncT) (LPFILETIME lpSystemTimeAsFileTime);
    FuncT GetPreciseSystemTime = nullptr;

    bool ResolveTimerFunction()
    {
        if (HINSTANCE hDLL = LoadLibrary("Kernel32.dll"))
        {
            if (GetPreciseSystemTime = (FuncT)::GetProcAddress((HMODULE)hDLL, "GetSystemTimePreciseAsFileTime"); !GetPreciseSystemTime)
            {
                // Function is exposed by the DLL, now it can be called
                GetPreciseSystemTime = (FuncT)::GetProcAddress((HMODULE)hDLL, "GetSystemTimeAsFileTime");
            }
        }

        YAGET_ASSERT(GetPreciseSystemTime, "GetPreciseSystemTime was not resolved, kernall32.dll does not support GetSystemTimePreciseAsFileTime or GetSystemTimeAsFileTime.");

        return GetPreciseSystemTime != nullptr;
    }

    bool ParseOptions(args::Options &options, int argCount, char** argValues, std::string* errorMessage)
    {
        try
        {
            if (argCount)
            {
                options.parse(argCount, argValues);
                if (options.find<bool>("help", false))
                {
                    yaget::platform::DebuggerOutput(options.help().c_str());
                    return false;
                }
            }

            return true;
        }
        catch (const args::OptionException& e)
        {
            yaget::platform::DebuggerOutput(options.help().c_str());
            yaget::platform::DebuggerOutput(e.what());
            if (errorMessage)
            {
                *errorMessage = e.what();
            }
            return false;
        }
    }

    struct Releaser
    {
        ~Releaser()
        {
            if (mArg) { ::LocalFree(mArg); }
        }

        char** mArg = nullptr;
    };

    // This attaches us to a console so we can pipe output to it, specialy from std::cout
    // but only if executable was started from console
    class ConsoleRedirector
    {
    public:
        ConsoleRedirector()
        {
            if ((mConsoleAttached = ::AttachConsole(ATTACH_PARENT_PROCESS)) == true)
            {
                // Redirect STDIN if the console has an input handle
                if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
                {
                    if (freopen_s(&mInHandle, "CONIN$", "r", stdin) == 0)
                    {
                        setvbuf(stdin, NULL, _IONBF, 0);
                    }
                }

                // Redirect STDOUT if the console has an output handle
                if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
                {
                    if (freopen_s(&mOutHandle, "CONOUT$", "w", stdout) == 0)
                    {
                        setvbuf(stdout, NULL, _IONBF, 0);
                    }
                }

                // Redirect STDERR if the console has an error handle
                if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
                {
                    if (freopen_s(&mErrHandle, "CONOUT$", "w", stderr) == 0)
                    {
                        setvbuf(stderr, NULL, _IONBF, 0);
                    }
                }

                // Make C++ standard streams point to console as well.
                std::ios::sync_with_stdio(true);

                // Clear the error state for each of the C++ standard streams.
                std::wcout.clear();
                std::cout.clear();
                std::wcerr.clear();
                std::cerr.clear();
                std::wcin.clear();
                std::cin.clear();
            }
        }

        ~ConsoleRedirector()
        {
            if (mConsoleAttached)
            {
                fclose(mInHandle);
                fclose(mOutHandle);
                fclose(mErrHandle);

                FILE* fp = nullptr;
                // Just to be safe, redirect standard IO to NUL before releasing.

                // Redirect STDIN to NUL
                if (freopen_s(&fp, "NUL:", "r", stdin) == 0)
                {
                    setvbuf(stdin, NULL, _IONBF, 0);
                }

                // Redirect STDOUT to NUL
                fp = nullptr;
                if (freopen_s(&fp, "NUL:", "w", stdout) == 0)
                {
                    setvbuf(stdout, NULL, _IONBF, 0);
                }

                // Redirect STDERR to NUL
                fp = nullptr;
                if (freopen_s(&fp, "NUL:", "w", stderr) == 0)
                {
                    setvbuf(stderr, NULL, _IONBF, 0);
                }

                // Detach from console
                FreeConsole();
            }
        }

    private:
        bool mConsoleAttached = false;
        FILE* mInHandle = nullptr;
        FILE* mOutHandle = nullptr;
        FILE* mErrHandle = nullptr;
    } consoleRedirector;


    system::InitializationResult InitializeEngine(const char* commandLine, int argc, char* argv[], args::Options& options, const char* configData, size_t configSize)
    {
        util::DefaultOptions(options);

        int argCount = argc;
        char** argValues = argv;

        Releaser releaser;

        if (commandLine)
        {
            std::string appName = util::ExpendEnv("$(ExecutableName)", nullptr);
            if (conv::ToLower(conv::safe(commandLine)).find(conv::ToLower(appName)) != std::string::npos)
            {
                // looks like commandLine passed to us already has app name, so we don't prefix here
                appName = "";
            }
            else
            {
                appName += " ";
            }

            std::string commands = appName + conv::safe(commandLine);
            argValues = CommandLineToArgvA(commands.c_str(), &argCount);

            //std::string appName = util::ExpendEnv("$(ExecutableName)", nullptr);
            //std::string commands = appName + " " + commandLine;
            //argValues = CommandLineToArgvA(commands.c_str(), &argCount);
            releaser.mArg = argValues;
        }

        std::string errorMessage;
        if (!ParseOptions(options, argCount, argValues, &errorMessage))
        {
            std::string commands;
            for (int i = 1; i < argCount; ++i)
            {
                commands += argValues[i];
                commands += " ";
            }

            std::string titleString = errorMessage.empty() ? "Help" : "Error";
            std::string message = fmt::format("{}\nOriginal command: '{}'\n\n{}", errorMessage, commands, options.help());
            std::string errorTitle = fmt::format("{} Command Line Options {}", util::ExpendEnv("$(AppName)", nullptr), titleString);

            util::DisplayDialog(errorTitle.c_str(), message.c_str());

            return system::InitializationResult::ParseError;
        }

        try
        {
            system::Initialize(options, configData, configSize);

            // let's check for config_view option and dump some configuration values
            if (options.find<bool>("options_view", false))
            {
                platform::DebuggerOutput(util::DisplayCurrentConfiguration(&options));
                return system::InitializationResult::Helped;
            }
            if (options.find<bool>("generate_config", false))
            {
                // create default configuration structure, add few samples,
                // so generated json file will have it
                dev::Configuration configuration;

                configuration.mDebug.mLogging.Filters = { "MAIN", "TEST" };
                configuration.mDebug.mLogging.Outputs = {
                    { "Console", { { "Key1", "Value1" } } }
                };

                configuration.mInit.mEnvironmentList = {
                    { "$(AliasOne)", { "Folder1", true} }
                };

                configuration.mInit.mVTSConfig =
                {
                    {
                        "BinTester3",
                        { "$(Temp)/section-11" },
                        { "*.bin" },
                        "BINNER",
                        false,
                        true
                    },
                    {
                        "BinTester4",
                        { "$(Temp)/section-417" },
                        { "*.foo" },
                        "FOOER",
                        true,
                        false
                    }
                };

                nlohmann::json jsonBlock;
                to_json(jsonBlock, configuration);
                platform::DebuggerOutput(json::PrettyPrint(jsonBlock));

                return system::InitializationResult::Helped;
            }

            return system::InitializationResult::OK;
        }
        catch (const ex::bad_init& e)
        {
            std::string message = fmt::format("Yaget Engine failed to initialize\nExamine log at: '{}'\n{}", util::ExpendEnv("$(LogFolder)", nullptr), e.what());
            std::string errorTitle = fmt::format("{} Startup Error", util::ExpendEnv("$(AppName)", nullptr));

            platform::DebuggerOutput(message);
            YLOG_ERROR("MAIN", "Terminating application. %s", e.what());

            if (platform::IsDebuggerAttached())
            {
                platform::DebuggerBreak();
            }

            util::DisplayDialog(errorTitle.c_str(), message.c_str());

            return system::InitializationResult::InitError;
        }
    }

    const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
        DWORD dwType;       // Must be 0x1000.
        LPCSTR szName;      // Pointer to name (in user addr space).
        DWORD dwThreadID;   // Thread ID (-1=caller thread).
        DWORD dwFlags;      // Reserved for future use, must be zero.
    } THREADNAME_INFO;
#pragma pack(pop)

    std::mt19937 GetRng()
    {
        std::mt19937 rng;
        rng.seed(std::random_device()());

        return rng;
    }

    template<typename T>
    class TimeSourceHRWin32
    {
    public:
        TimeSourceHRWin32()
            : mResolved(ResolveTimerFunction())
            , mInitialTime(GetPresizeTime<T>())
        {
        }

        T GetTime(time::TimeUnits_t timeUnit) const
        {
            T val = GetPresizeTime<T>();
            return (val - mInitialTime) / timeUnit;
        }

        void AdjustDrift(time::TimeUnits_t amount, time::TimeUnits_t timeUnit)
        {
            mInitialTime += amount * timeUnit;
        }

    private:
        template<typename T>
        static T GetPresizeTime()
        {
            FILETIME platformTimestamp;
            GetPreciseSystemTime(&platformTimestamp);
            T val = (static_cast<T>(platformTimestamp.dwHighDateTime) << 32) + platformTimestamp.dwLowDateTime;

            return val;
        }

        const bool mResolved = false;
        T mInitialTime = 0;
    };
    TimeSourceHRWin32<yaget::time::TimeUnits_t> HighResTimeSource;

    class ThreadNames
    {
    public:
        void SetName(uint32_t threadId, const char* name) noexcept
        {
            std::unique_lock<std::mutex> locker(mMutex);
            mNames.insert(std::make_pair(threadId, name));
        }

        std::string GetName(uint32_t threadId) const
        {
            std::unique_lock<std::mutex> locker(mMutex);
            auto it = mNames.find(threadId);
            if (it != mNames.end())
            {
                return it->second;
            }

            return "";
        }

    private:
        mutable std::mutex mMutex;
        std::map<uint32_t, std::string> mNames;
    };
    ThreadNames threadNames;

} // namespace

/*
*Get the list of all files in given directory and its sub directories.
*
* Arguments
* 	dirPath : Path of directory to be traversed
* 	dirSkipList : List of folder names to be skipped
*
* Returns :
*vector containing paths of all the files in given directory and its sub directories
*
*/
std::vector<std::string> getAllFilesInDir(const std::string &dirPath, const std::vector<std::string> dirSkipList = { })
{

    // Create a vector of string
    std::vector<std::string> listOfFiles;
    try 
    {
        // Check if given path exists and points to a directory
        if (fs::exists(dirPath) && fs::is_directory(dirPath))
        {
            // Create a Recursive Directory Iterator object and points to the starting of directory
            fs::recursive_directory_iterator iter(dirPath);

            // Create a Recursive Directory Iterator object pointing to end.
            fs::recursive_directory_iterator end;

            // Iterate till end
            while (iter != end)
            {
                // Check if current entry is a directory and if exists in skip list
                if (fs::is_directory(iter->path()) &&
                    (std::find(dirSkipList.begin(), dirSkipList.end(), iter->path().filename()) != dirSkipList.end()))
                {
                    // Skip the iteration of current directory pointed by iterator
                    // c++17 Filesystem API to skip current directory iteration
                    iter.disable_recursion_pending();
                }
                else
                {
                    // Add the name in vector
                    listOfFiles.push_back(iter->path().string());
                }

                std::error_code ec;
                // Increment the iterator to point to next entry in recursive iteration
                iter.increment(ec);
                if (ec) 
                {
                    std::cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() << '\n';
                }
            }
        }
    }
    catch (std::system_error & e)
    {
        std::cerr << "Exception :: " << e.what();
    }
    return listOfFiles;
}


void platform::SetThreadName(const char* threadName, uint32_t t)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = t;
    info.dwFlags = 0;
    threadNames.SetName(t, threadName);

    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {}

}

void platform::SetThreadName(const char* threadName, std::thread& t)
{
    platform::SetThreadName(threadName, GetThreadId(t));
}

std::string platform::GetThreadName(uint32_t t)
{
    return threadNames.GetName(t);
}

std::string platform::GetCurrentThreadName()
{
    return GetThreadName(CurrentThreadId());
}

uint32_t platform::GetThreadId(std::thread& t)
{
    uint32_t threadId = ::GetThreadId(static_cast<HANDLE>(t.native_handle()));
    return threadId;
}

uint32_t platform::CurrentThreadId()
{
    return ::GetCurrentThreadId();
}

void platform::Sleep(SleepPredicate sleepPredicate)
{
    while (sleepPredicate())
    {
        std::this_thread::yield();
    }
}

void platform::Sleep(time::TimeUnits_t numSleep, time::TimeUnits_t unitType)
{
    time::Microsecond_t sleepDuration = time::FromTo<time::Microsecond_t>(numSleep, unitType, time::kMicrosecondUnit);
    std::this_thread::sleep_for(std::chrono::microseconds(sleepDuration));
}

void platform::BusySleep(time::TimeUnits_t numSleep, time::TimeUnits_t unitType)
{
    time::TimeUnits_t endTime = platform::GetRealTime(unitType) + numSleep;
    while (platform::GetRealTime(unitType) < endTime)
    {
        std::this_thread::yield();
    }
}

int platform::GetRandom(int from, int to)
{
    static std::mt19937 rng = GetRng();

    std::uniform_int_distribution<std::mt19937::result_type> dist(from, to);
    return dist(rng);
}

yaget::time::TimeUnits_t platform::GetRealTime(time::TimeUnits_t timeUnit)
{
    return HighResTimeSource.GetTime(timeUnit);
}

void platform::AdjustDrift(time::TimeUnits_t amount, time::TimeUnits_t timeUnit)
{
    HighResTimeSource.AdjustDrift(amount, timeUnit);
}

void platform::LogLastError(const std::string& userMessage)
{
    LPVOID lpMsgBuf = nullptr;
    DWORD dw = GetLastError();

    ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    dw,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&lpMsgBuf,
                    0, nullptr);

    YLOG_ERROR("PLAT", "GetLastError was: '%s'. User message: '%s'.", (const char *)lpMsgBuf, userMessage.c_str());
    ::LocalFree(lpMsgBuf);
}

std::string yaget::platform::LastErrorMessage()
{
    uint64_t hr = ::GetLastError();
    _com_error cr(HRESULT_FROM_WIN32(static_cast<unsigned long>(hr)));
    const char* errorMessage = cr.ErrorMessage();
    std::string textError = fmt::format("Error code: '{}', Error message: {}", hr, errorMessage);

    return textError;
}

bool platform::ParseArgs(const char* commandLine, args::Options& options, std::string* errorMessage)
{
    int argCount;
    std::string appName = util::ExpendEnv("$(ExecutableName)", nullptr);
    if (conv::ToLower(conv::safe(commandLine)).find(conv::ToLower(appName)) != std::string::npos)
    {
        // looks like commandLine passed to us already has app name, so we don't prefix here
        appName = "";
    }
    else
    {
        appName += " ";
    }

    std::string commands = appName + conv::safe(commandLine);
    char** argValues = CommandLineToArgvA(commands.c_str(), &argCount);
    Releaser releaser;
    releaser.mArg = argValues;

    return ParseOptions(options, argCount, argValues, errorMessage);
}

bool platform::ParseArgs(args::Options& options, std::string* errorMessage)
{
    const char* cl = ::GetCommandLineA();
    return ParseArgs(cl, options, errorMessage);
}

bool platform::IsDebuggerAttached()
{
    return ::IsDebuggerPresent();
}

void platform::DebuggerBreak()
{
    ::DebugBreak();
}

void platform::DebuggerOutput(const std::string& message)
{
    std::cout << message << std::endl;

    if (platform::IsDebuggerAttached())
    {
        ::OutputDebugStringA(message.c_str());
        ::OutputDebugStringA("\n");
    }
}

void system::Initialize(const args::Options& options, const char* configData, size_t configSize)
{
    dev::Initialize(options, configData, configSize);
    ylog::Initialize(options);
    metrics::Initialize(options);

    double appTime = time::FromTo<double>(platform::GetRealTime(time::kMicrosecondUnit), time::kMicrosecondUnit, time::kSecondUnit);
    std::string buildNumber = dev::CurrentConfiguration().mDebug.mFlags.BuildId == -1 ? "" : fmt::format(", Build: '{}'", dev::CurrentConfiguration().mDebug.mFlags.BuildId);
    YLOG_NOTICE("INIT", "YAGET Engine initialized. Application: '%s', Version: '%s'%s at time: %f sec.",
        util::ExpendEnv("$(AppName)", nullptr).c_str(),
        yaget::ToString(yaget::YagetVersion).c_str(),
        buildNumber.c_str(), appTime);
}

system::InitializationResult system::InitializeSetup(int argc, char* argv[], args::Options& options, const char* configData, size_t configSize)
{
    return InitializeEngine(nullptr, argc, argv, options, configData, configSize);
}

system::InitializationResult system::InitializeSetup(const char* commandLine, args::Options& options, const char* configData, size_t configSize)
{
    return InitializeEngine(commandLine, 0, nullptr, options, configData, configSize);
}

system::InitializationResult system::InitializeSetup(args::Options& options, const char* configData, size_t configSize)
{
    const char* commandLine = ::GetCommandLineA();
    return InitializeSetup(commandLine, options, configData, configSize);
}

system::InitializationResult system::InitializeSetup()
{
    std::string appName = util::ExpendEnv("$(AppName)", nullptr);
    args::Options options(appName);
    
    return InitializeSetup(options, nullptr, 0);
}


// Convert wide Unicode String to UTF8 string
std::string conv::wide_to_utf8(const wchar_t* wstr)
{
    if (!wstr)
    {
        return std::string();
    }

    int slen = static_cast<int>(wcslen(wstr));

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, slen, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, slen, &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring conv::utf8_to_wide(const std::string &str)
{
    if (str.empty())
    {
        return std::wstring();
    }

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
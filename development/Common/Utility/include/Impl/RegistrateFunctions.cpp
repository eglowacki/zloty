//#include "Registrate.h"
//#include <boost/filesystem.hpp>
//#include <boost/algorithm/string.hpp>


//using namespace eg;
//namespace bfs = boost::filesystem;

/*
__declspec(dllexport) boost::any registrate::GetObject(char const *)
{
    return boost::any();
}


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef GetObject

//extern "C" __declspec(dllexport) const char *yagetGetExecutablePath();
//extern "C" __declspec(dllexport) const char *yagetGetAppName();
//extern "C"__declspec(dllexport) void yagetsleep(uint32_t miliseconds);

// just to satisfy linker for the time being
__declspec(dllexport) const char *yagetGetExecutablePath()
{
    static char buf[512] = {'\0'};
    GetModuleFileName(0, buf, 511);
    bfs::path test_path = buf;
    std::string appPath = test_path.string();
    boost::ierase_last(appPath, test_path.filename());
    if (!appPath.empty() && (*appPath.rbegin() == '/' || *appPath.rbegin() == '\\'))
    {
        appPath.erase(appPath.size()-1);
    }
    std::copy(appPath.begin(), appPath.end(), buf);
    return buf;
}

__declspec(dllexport) const char *yagetGetAppName()
{
    static char buf[512] = {'\0'};
    GetModuleFileName(0, buf, 511);
    bfs::path app_path = buf;
    std::string appName = app_path.stem();
    std::copy(appName.begin(), appName.end(), buf);
    return buf;
}

__declspec(dllexport) void yagetsleep(uint32_t miliseconds)
{
    ::Sleep(miliseconds);
}
*/
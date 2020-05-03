//////////////////////////////////////////////////////////////////////
// YagetVersion.h
//
//  Copyright 9/1/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Version number for Yaget engine and all plugins used by it
//      This uses P4 $Change attribute to extract build number. 
//      To update the build number, simply check this file out
//      and check in to trigger expansion of $Change
//
//  #include "YagetVersion.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include <ostream>


namespace yaget::internal { uint32_t ParseBuildChange(const char* changeText); }

#define YAGET_MAJOR 0
#define YAGET_MINOR 2
#define YAGET_BUILD 0
#define YAGET_CHANGE yaget::internal::ParseBuildChange("$Change: 717 $")

namespace yaget
{
    //! Version number for current implementation of Yaget engine.
    struct Version
    {
        inline bool operator==(const Version& rh) const
        {
            return Major == rh.Major
                && Minor == rh.Minor
                && Build == rh.Build
#ifdef  YAGET_SHIPPING
                && Change == rh.Change
#endif //  YAGET_SHIPPING

                ;
        }

        //! Major version of engine. In most cases no other plugins will work anymore
        uint32_t Major;
        //! Small incremental change in Yaget engine.
        uint32_t Minor;
        //! Current public build of Yaget engine
        uint32_t Build;
        //! Current public build of Yaget engine
        uint32_t Change;
    };

    const static Version YagetVersion{ YAGET_MAJOR, YAGET_MINOR, YAGET_BUILD, YAGET_CHANGE };

    std::string ToString(const Version& version);

    inline std::ostream& operator<<(std::ostream& out, const Version& version)
    {
        return out << version.Major << "." << version.Minor << "." << version.Build << "." << version.Change;
    }

    bool CheckVersion(const Version& version, const char* compilerVersion);

} // namespace yaget


// _MSVC_LANG Defined as an integer literal that specifies the C++ language standard targeted by the compiler.
// It's set only in code compiled as C++. The macro is the integer literal value 201402L by default, 
// or when the /std:c++14 compiler option is specified. The macro is set to 201703L if the /std:c++17 compiler option 
// is specified. It's set to a higher, unspecified value when the / std:c++latest option is specified.Otherwise, 
// the macro is undefined.The _MSVC_LANG macro and / std(Specify Language Standard Version) compiler options 
// are available beginning in Visual Studio 2015 Update 3.
#define YAGET_COMPILER_INFO "Yaget Engine: MSVC Version: " YAGET_STRINGIZE(_MSC_VER) ", C++ Supported Standard: " YAGET_STRINGIZE(_MSVC_LANG)

#define YAGET_CHECKVERSION yaget::CheckVersion(yaget::YagetVersion, YAGET_COMPILER_INFO)

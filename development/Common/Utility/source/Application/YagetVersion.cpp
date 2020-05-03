#include "YagetVersion.h"
#include "StringHelpers.h"


uint32_t yaget::internal::ParseBuildChange(const char* changeText)
{
    std::string text(changeText ? changeText : "");

    yaget::conv::ReplaceAll(text, "$Change:", "");
    yaget::conv::ReplaceAll(text, "$", "");
    return yaget::conv::Convertor<uint32_t>::FromString(text.c_str());
}

bool yaget::CheckVersion(const Version& version, const char* compilerVersion)
{
    YAGET_ASSERT(version == yaget::YagetVersion, "Mismatch of Yaget Engine Version. Engine Version: '%s'. Requested Version: '%s'.",
        yaget::ToString(yaget::YagetVersion).c_str(), yaget::ToString(version).c_str());
    YAGET_ASSERT(std::string(compilerVersion) == YAGET_COMPILER_INFO, "Mismatch of Yaget Engine compiler. Engine Compileed with: '%s'. User Version: '%s'.", YAGET_COMPILER_INFO, compilerVersion);

    return version == yaget::YagetVersion;
}

std::string yaget::ToString(const Version& version)
{
    return fmt::format("{}.{}.{}.{}", version.Major, version.Minor, version.Build, version.Change);
}

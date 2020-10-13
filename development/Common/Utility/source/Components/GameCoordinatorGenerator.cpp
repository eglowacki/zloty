#include "Components/GameCoordinatorGenerator.h"


yaget::Strings yaget::comp::db::internal::ResolveUserStripKeywords(const Strings& defaultSet)
{
    //std::string result = "Yaget";
    if (const auto f = yaget::util::ResolveFunction<YagetFuncUserStripKeywords>(YAGET_USER_STRIP_KEYWORDS_FUNCTION_STRING))
    {
        const auto& defaultValues = conv::Combine(defaultSet, ",");
        auto result = f(defaultValues.c_str());
        return conv::Split(result, ",");
    }

    return defaultSet;
}

#include "Json/JsonHelpers.h"
#include "StringHelpers.h"

nlohmann::json yaget::json::ParseConfig(const std::string& text)
{
    using namespace yaget;

    nlohmann::json jsonBlock;
    if (const auto keyValue = conv::Split(text, "=", true); keyValue.size() == 2)
    {
        nlohmann::json* currentJasonBlock = &jsonBlock;

        if (const auto keys = conv::Split(keyValue[0], ".", true); !keys.empty())
        {
            for (const auto key : keys)
            {
                nlohmann::json& jb = *currentJasonBlock;
                currentJasonBlock = &(jb[key]);
            }

            if (const auto& value = keyValue[1]; !value.empty())
            {
                if (value.size() > 1 && *value.begin() == '\'' && value.back() == '\'')
                {
                    *currentJasonBlock = std::string(value.begin() + 1, value.end() - 1);
                }
                else if (CompareI(value, "true") || CompareI(value, "false"))
                {
                    *currentJasonBlock = conv::Convertor<bool>::FromString(value.c_str());
                }
                else if (value.ends_with("f") || value.ends_with("F"))
                {
                    *currentJasonBlock = conv::Convertor<float>::FromString(value.c_str());
                }
            }
        }
    }

    return jsonBlock;
}

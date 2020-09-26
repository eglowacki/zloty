#include "VTS/RenderResolvedAssets.h"


namespace
{
    //------------------------------------------------------------------------------------------------------------------------------
    using D3D11AddressModeMap = std::unordered_map<std::string, D3D11_TEXTURE_ADDRESS_MODE>;

    D3D11AddressModeMap D3D11_AddressMode = {
        { "WRAP",           D3D11_TEXTURE_ADDRESS_WRAP },
        { "MIRROR",         D3D11_TEXTURE_ADDRESS_MIRROR },
        { "CLAMP",          D3D11_TEXTURE_ADDRESS_CLAMP },
        { "BORDER",         D3D11_TEXTURE_ADDRESS_BORDER },
        { "MIRROR_ONCE",    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE }
    };


    //------------------------------------------------------------------------------------------------------------------------------
    using D3D11ComparisonMap = std::unordered_map<std::string, D3D11_COMPARISON_FUNC>;

    D3D11ComparisonMap D3D11_Comparison = {
        { "NEVER",           D3D11_COMPARISON_NEVER },
        { "LESS",            D3D11_COMPARISON_LESS },
        { "EQUAL",           D3D11_COMPARISON_EQUAL },
        { "LESS_EQUAL",      D3D11_COMPARISON_LESS_EQUAL },
        { "GREATER",         D3D11_COMPARISON_GREATER },
        { "NOT_EQUAL",       D3D11_COMPARISON_NOT_EQUAL },
        { "GREATER_EQUAL",   D3D11_COMPARISON_GREATER_EQUAL },
        { "ALWAYS",          D3D11_COMPARISON_ALWAYS }
    };


    //------------------------------------------------------------------------------------------------------------------------------
    using D3D11FilterMap = std::unordered_map<std::string, D3D11_FILTER>;

    D3D11FilterMap D3D11_Filters = {
        { "MIN_MAG_MIP_POINT",                          D3D11_FILTER_MIN_MAG_MIP_POINT },
        { "MIN_MAG_POINT_MIP_LINEAR",                   D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR },
        { "MIN_POINT_MAG_LINEAR_MIP_POINT",             D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT },
        { "MIN_POINT_MAG_MIP_LINEAR",                   D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR },
        { "MIN_LINEAR_MAG_MIP_POINT",                   D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT },
        { "MIN_LINEAR_MAG_POINT_MIP_LINEAR",            D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR },
        { "MIN_MAG_LINEAR_MIP_POINT",                   D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT },
        { "MIN_MAG_MIP_LINEAR",                         D3D11_FILTER_MIN_MAG_MIP_LINEAR },
        { "ANISOTROPIC",                                D3D11_FILTER_ANISOTROPIC },
        { "COMPARISON_MIN_MAG_MIP_POINT",               D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT },
        { "COMPARISON_MIN_MAG_POINT_MIP_LINEAR",        D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR },
        { "COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT",  D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  },
        { "COMPARISON_MIN_POINT_MAG_MIP_LINEAR",        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR },
        { "COMPARISON_MIN_LINEAR_MAG_MIP_POINT",        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT },
        { "COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR", D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR },
        { "COMPARISON_MIN_MAG_LINEAR_MIP_POINT",        D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT },
        { "COMPARISON_MIN_MAG_MIP_LINEAR",              D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR },
        { "COMPARISON_ANISOTROPIC",                     D3D11_FILTER_COMPARISON_ANISOTROPIC },
        { "MINIMUM_MIN_MAG_MIP_POINT",                  D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT },
        { "MINIMUM_MIN_MAG_POINT_MIP_LINEAR",           D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR },
        { "MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT",     D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT },
        { "MINIMUM_MIN_POINT_MAG_MIP_LINEAR",           D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR },
        { "MINIMUM_MIN_LINEAR_MAG_MIP_POINT",           D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT },
        { "MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR",    D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR },
        { "MINIMUM_MIN_MAG_LINEAR_MIP_POINT",           D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT },
        { "MINIMUM_MIN_MAG_MIP_LINEAR",                 D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR },
        { "MINIMUM_ANISOTROPIC",                        D3D11_FILTER_MINIMUM_ANISOTROPIC },
        { "MAXIMUM_MIN_MAG_MIP_POINT",                  D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT },
        { "MAXIMUM_MIN_MAG_POINT_MIP_LINEAR",           D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR },
        { "MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT",     D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT },
        { "MAXIMUM_MIN_POINT_MAG_MIP_LINEAR",           D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR },
        { "MAXIMUM_MIN_LINEAR_MAG_MIP_POINT",           D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT },
        { "MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR",    D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR },
        { "MAXIMUM_MIN_MAG_LINEAR_MIP_POINT",           D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT },
        { "MAXIMUM_MIN_MAG_MIP_LINEAR",                 D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR },
        { "MAXIMUM_ANISOTROPIC",                        D3D11_FILTER_MAXIMUM_ANISOTROPIC }
    };

    template <typename T>
    uint32_t GetTextureMetaValue(const std::string& name, const T container)
    {
        auto it = container.find(name);
        return it != container.end() ? it->second : 0;
    }

} // namespace


uint32_t yaget::io::render::meta::GetWrapMode(const std::string& name)
{
    return GetTextureMetaValue(name, D3D11_AddressMode);
}

uint32_t yaget::io::render::meta::GetComparisonFunc(const std::string& name)
{
    return GetTextureMetaValue(name, D3D11_Comparison);
}

uint32_t yaget::io::render::meta::GetFilter(const std::string& name)
{
    return GetTextureMetaValue(name, D3D11_Filters);
}

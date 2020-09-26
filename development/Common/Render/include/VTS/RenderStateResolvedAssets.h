//////////////////////////////////////////////////////////////////////
// RenderStateResolvedAssets.h
//
//  Copyright 8/11/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "VTS/RenderStateResolvedAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "VTS/RenderResolvedAssets.h"


namespace yaget
{
    namespace io::render
    {
        //-------------------------------------------------------------------------------------------------------------------------------
        // DepthEnable, 
        // D3D11_DEPTH_WRITE_MASK, 
        // D3D11_COMPARISON_FUNC, 
        // StencilEnable, 
        // StencilReadMask
        // StencilWriteMask
        // NOT Supporting D3D11_DEPTH_STENCILOP_DESC flag
        class DepthStencilStateAsset : public JasonMetaDataAsset<bool, D3D11_DEPTH_WRITE_MASK, D3D11_COMPARISON_FUNC, bool, uint8_t, uint8_t>
        {
        public:
            DepthStencilStateAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
                : JasonMetaDataAsset(tag, buffer, vts, "DepthStencilState",
                    []() { return true; },
                    [this]() { return json::GetValue<bool>(root, "DepthEnable", true); },
                    [this]() { return json::GetValue<D3D11_DEPTH_WRITE_MASK>(root, "DepthWriteMask", D3D11_DEPTH_WRITE_MASK_ALL); },
                    [this]() { return json::GetValue<D3D11_COMPARISON_FUNC>(root, "DepthFunc", D3D11_COMPARISON_LESS); },
                    [this]() { return json::GetValue<bool>(root, "StencilEnable", false); },
                    [this]() { return json::GetValue<uint8_t>(root, "StencilReadMask", 0); },
                    [this]() { return json::GetValue<uint8_t>(root, "StencilWriteMask", 0); })
                , mDepthEnable(std::get<0>(mFields))
                , mDepthWriteMask(std::get<1>(mFields))
                , mDepthComparisonFunc(std::get<2>(mFields))
                , mStencilEnable(std::get<3>(mFields))
                , mStencilReadMask(std::get<4>(mFields))
                , mStencilWriteMask(std::get<5>(mFields))
            {}

            const bool& mDepthEnable;
            const D3D11_DEPTH_WRITE_MASK& mDepthWriteMask;
            const D3D11_COMPARISON_FUNC& mDepthComparisonFunc;
            const bool& mStencilEnable;
            const uint8_t& mStencilReadMask;
            const uint8_t& mStencilWriteMask;
        };

        // D3D11_FILL_MODE FillMode;
        // D3D11_CULL_MODE CullMode;
        // BOOL            FrontCounterClockwise;
        // INT             DepthBias;
        // FLOAT           DepthBiasClamp;
        // FLOAT           SlopeScaledDepthBias;
        // BOOL            DepthClipEnable;
        // BOOL            ScissorEnable;
        // BOOL            MultisampleEnable;
        // BOOL            AntialiasedLineEnable;
        class RasterizerStateAsset : public JasonMetaDataAsset<D3D11_FILL_MODE, D3D11_CULL_MODE, bool, int, float, float, bool, bool, bool, bool>
        {
        public:
            RasterizerStateAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
                : JasonMetaDataAsset(tag, buffer, vts, "RasterizerState",
                    []() { return true; },
                    [this]() { return json::GetValue<D3D11_FILL_MODE>(root, "FillMode", D3D11_FILL_SOLID); },
                    [this]() { return json::GetValue<D3D11_CULL_MODE>(root, "CullMode", D3D11_CULL_BACK); },
                    [this]() { return json::GetValue<bool>(root, "FrontCounterClockwise", false); },
                    [this]() { return json::GetValue<int>(root, "DepthBias", 0); },
                    [this]() { return json::GetValue<float>(root, "DepthBiasClamp", 0.0f); },
                    [this]() { return json::GetValue<float>(root, "SlopeScaledDepthBias", 0.0f); },
                    [this]() { return json::GetValue<bool>(root, "DepthClipEnable", true); },
                    [this]() { return json::GetValue<bool>(root, "ScissorEnable", false); },
                    [this]() { return json::GetValue<bool>(root, "MultisampleEnable", false); },
                    [this]() { return json::GetValue<bool>(root, "AntialiasedLineEnable", true); })
                , mFillMode(std::get<0>(mFields))
                , mCullMode(std::get<1>(mFields))
                , mFrontCounterClockwise(std::get<2>(mFields))
                , mDepthBias(std::get<3>(mFields))
                , mDepthBiasClamp(std::get<4>(mFields))
                , mSlopeScaledDepthBias(std::get<5>(mFields))
                , mDepthClipEnable(std::get<6>(mFields))
                , mScissorEnable(std::get<7>(mFields))
                , mMultisampleEnable(std::get<8>(mFields))
                , mAntialiasedLineEnable(std::get<9>(mFields))
            {}

            const D3D11_FILL_MODE& mFillMode;
            const D3D11_CULL_MODE& mCullMode;
            const bool& mFrontCounterClockwise;
            const int&  mDepthBias;
            const float& mDepthBiasClamp;
            const float& mSlopeScaledDepthBias;
            const bool& mDepthClipEnable;
            const bool& mScissorEnable;
            const bool& mMultisampleEnable;
            const bool& mAntialiasedLineEnable;
        };


        //BOOL                           AlphaToCoverageEnable;
        //BOOL                           IndependentBlendEnable;
        //math3d::Color                  BlendFactor 
        //D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
        //                                  BOOL           BlendEnable;
        //                                  D3D11_BLEND    SrcBlend;
        //                                  D3D11_BLEND    DestBlend;
        //                                  D3D11_BLEND_OP BlendOp;
        //                                  D3D11_BLEND    SrcBlendAlpha;
        //                                  D3D11_BLEND    DestBlendAlpha;
        //                                  D3D11_BLEND_OP BlendOpAlpha;
        //                                  UINT8          RenderTargetWriteMask;
        struct TargetBlend
        {
            bool           mBlendEnable;
            D3D11_BLEND    mSrcBlend;
            D3D11_BLEND    mDestBlend;
            D3D11_BLEND_OP mBlendOp;
            D3D11_BLEND    mSrcBlendAlpha;
            D3D11_BLEND    mDestBlendAlpha;
            D3D11_BLEND_OP mBlendOpAlpha;
            uint8_t        mRenderTargetWriteMask;
        };
        using TargetBlends = std::vector<TargetBlend>;

        class BlendStateAsset : public JasonMetaDataAsset<bool, bool, math3d::Color, TargetBlends>
        {
        public:
            BlendStateAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
                : JasonMetaDataAsset(tag, buffer, vts, "BlendState",
                    [this]() { return !Get<3>().empty(); },
                    [this]() { return json::GetValue<bool>(root, "AlphaToCoverageEnable", false); },
                    [this]() { return json::GetValue<bool>(root, "IndependentBlendEnable", false); },
                    [this]() { return json::GetValue<math3d::Color>(root, "BlendFactor", math3d::Color(1, 1, 1, 1)); },
                    [this]()
                    { 
                        TargetBlends targetBlends;
                        if (auto it = root.find("Blends"); it != root.end() && it->is_array())
                        {
                            const nlohmann::json& blends = *it;
                            for (const auto& blend : blends)
                            {
                                TargetBlend targetBlend;

                                targetBlend.mBlendEnable = json::GetValue<bool>(blend, "BlendEnable", false);
                                targetBlend.mSrcBlend = json::GetValue<D3D11_BLEND>(blend, "SrcBlend", D3D11_BLEND_ONE);
                                targetBlend.mDestBlend = json::GetValue<D3D11_BLEND>(blend, "DestBlend", D3D11_BLEND_ZERO);
                                targetBlend.mBlendOp = json::GetValue<D3D11_BLEND_OP>(blend, "BlendOp", D3D11_BLEND_OP_ADD);
                                targetBlend.mSrcBlendAlpha = json::GetValue<D3D11_BLEND>(blend, "SrcBlendAlpha", D3D11_BLEND_ONE);
                                targetBlend.mDestBlendAlpha = json::GetValue<D3D11_BLEND>(blend, "DestBlendAlpha", D3D11_BLEND_ZERO);
                                targetBlend.mBlendOpAlpha = json::GetValue<D3D11_BLEND_OP>(blend, "BlendOpAlpha", D3D11_BLEND_OP_ADD);
                                targetBlend.mRenderTargetWriteMask = json::GetValue<uint8_t>(blend, "RenderTargetWriteMask", static_cast<uint8_t>(D3D11_COLOR_WRITE_ENABLE_ALL));

                                targetBlends.emplace_back(std::move(targetBlend));
                            }
                        }
                        else
                        {
                            targetBlends.emplace_back(TargetBlend{ false, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, 0 });
                        }

                        return targetBlends;
                    })
                , mAlphaToCoverageEnable(std::get<0>(mFields))
                , mIndependentBlendEnable(std::get<1>(mFields))
                , mBlendFactor(std::get<2>(mFields))
                , mTargetBlends(std::get<3>(mFields))
            {}

            const bool& mAlphaToCoverageEnable;
            const bool& mIndependentBlendEnable;
            const math3d::Color& mBlendFactor;
            const TargetBlends& mTargetBlends;
        };

    } // namespace io::render
} // namespace yaget


namespace yaget::conv
{
    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_DEPTH_WRITE_MASK>
    {
        static std::string ToString(const D3D11_DEPTH_WRITE_MASK& value)
        {
            return fmt::format("D3D11_DEPTH_WRITE_MASK: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_COMPARISON_FUNC>
    {
        static std::string ToString(const D3D11_COMPARISON_FUNC& value)
        {
            return fmt::format("D3D11_COMPARISON_FUNC: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_FILL_MODE>
    {
        static std::string ToString(const D3D11_FILL_MODE& value)
        {
            return fmt::format("D3D11_FILL_MODE: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_CULL_MODE>
    {
        static std::string ToString(const D3D11_CULL_MODE& value)
        {
            return fmt::format("D3D11_CULL_MODE: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_BLEND>
    {
        static std::string ToString(const D3D11_BLEND& value)
        {
            return fmt::format("D3D11_BLEND: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<D3D11_BLEND_OP>
    {
        static std::string ToString(const D3D11_BLEND_OP& value)
        {
            return fmt::format("D3D11_BLEND_OP: '{}'", value);
        }
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template <>
    struct Convertor<io::render::TargetBlends>
    {
        static std::string ToString(const io::render::TargetBlends& value)
        {
            std::string results;
            for (const auto& it : value)
            {
                std::string blendvalues = fmt::format("{ BlendEnable: '{}', SrcBlend: '{}', DestBlend: '{}', BlendOp: '{}', SrcBlendAlpha: '{}', DestBlendAlpha: '{}', BlendOpAlpha: '{}', mRenderTargetWriteMask: '{}' }",
                    it.mBlendEnable, it.mSrcBlend, it.mDestBlend, it.mBlendOp, it.mSrcBlendAlpha, it.mDestBlendAlpha, it.mBlendOpAlpha, it.mRenderTargetWriteMask);

                results += blendvalues;
            }

            return results.empty() ? "NULL" : results;
        }
    };

} // namespace yaget::conv


namespace DirectX::SimpleMath
{

    inline void to_json(nlohmann::json& j, const math3d::Color& color)
    {
        j = fmt::format("{}, {}, {}, {}", color.R(), color.G(), color.B(), color.A());
    }

    inline void from_json(const nlohmann::json& j, math3d::Color& color)
    {
        std::string source;
        j.get_to(source);

        auto values = yaget::conv::Split(source, ",");
        if (values.size() == 4)
        {
            float r = yaget::conv::AtoN<float>(values[0].c_str());
            float g = yaget::conv::AtoN<float>(values[1].c_str());
            float b = yaget::conv::AtoN<float>(values[2].c_str());
            float a = yaget::conv::AtoN<float>(values[3].c_str());
            color = math3d::Color(r, g, b, a);
        }
    }

} // namespace DirectX::SimpleMath



NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_DEPTH_WRITE_MASK, {
    { D3D11_DEPTH_WRITE_MASK_ALL, nullptr },
    { D3D11_DEPTH_WRITE_MASK_ZERO, "Zero" },
    { D3D11_DEPTH_WRITE_MASK_ALL, "All" }
    });

NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_COMPARISON_FUNC, {
    { D3D11_COMPARISON_LESS, nullptr },
    { D3D11_COMPARISON_NEVER, "Never" },
    { D3D11_COMPARISON_LESS, "Less" },
    { D3D11_COMPARISON_EQUAL, "Equal" },
    { D3D11_COMPARISON_LESS_EQUAL, "LessEqual" },
    { D3D11_COMPARISON_GREATER, "Greater" },
    { D3D11_COMPARISON_NOT_EQUAL, "NotEqual" },
    { D3D11_COMPARISON_GREATER_EQUAL, "GreaterEqual" },
    { D3D11_COMPARISON_ALWAYS, "Always" }
    });

NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_FILL_MODE, {
    { D3D11_FILL_SOLID, nullptr },
    { D3D11_FILL_WIREFRAME, "Wire" },
    { D3D11_FILL_SOLID, "Solid" }
    });

NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_CULL_MODE, {
    { D3D11_CULL_NONE, nullptr },
    { D3D11_CULL_NONE, "None" },
    { D3D11_CULL_FRONT, "Front" },
    { D3D11_CULL_BACK, "Back" }
    });

NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_BLEND, {
    { D3D11_BLEND_ZERO, nullptr },
    { D3D11_BLEND_ZERO, "Zero" },
    { D3D11_BLEND_ONE, "One" },
    { D3D11_BLEND_SRC_COLOR, "SrcColor" },
    { D3D11_BLEND_INV_SRC_COLOR, "InvSrcColor" },
    { D3D11_BLEND_SRC_ALPHA, "SrcAlpha" },
    { D3D11_BLEND_INV_SRC_ALPHA, "InvSrcAlpha" },
    { D3D11_BLEND_DEST_ALPHA, "DestAlpha" },
    { D3D11_BLEND_INV_DEST_ALPHA, "InvDestAlpha" },
    { D3D11_BLEND_DEST_COLOR, "DestColor" },
    { D3D11_BLEND_INV_DEST_COLOR, "InvDestColor" },
    { D3D11_BLEND_SRC_ALPHA_SAT, "SrcAlphaSat" },
    { D3D11_BLEND_BLEND_FACTOR, "BlendFactor" },
    { D3D11_BLEND_INV_BLEND_FACTOR, "InvBlendFactor" },
    { D3D11_BLEND_SRC1_COLOR, "Src1Color" },
    { D3D11_BLEND_INV_SRC1_COLOR, "InvSrc1Color" },
    { D3D11_BLEND_SRC1_ALPHA, "Src1Alpha" },
    { D3D11_BLEND_INV_SRC1_ALPHA, "InvSrc1Alpha" }
    });

NLOHMANN_JSON_SERIALIZE_ENUM(D3D11_BLEND_OP, {
    { D3D11_BLEND_OP_ADD, nullptr },
    { D3D11_BLEND_OP_ADD, "Add" },
    { D3D11_BLEND_OP_SUBTRACT, "Subtract" },
    { D3D11_BLEND_OP_REV_SUBTRACT, "RevSubtract" },
    { D3D11_BLEND_OP_MIN, "Min" },
    { D3D11_BLEND_OP_MAX, "Max" }
    });

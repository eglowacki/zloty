//////////////////////////////////////////////////////////////////////
// RenderResolvedAssets.h
//
//  Copyright 6/24/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Contains derived classes from io::Asset for renderer resolver support
//
//
//  #include "VTS/RenderResolvedAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/ResolvedAssets.h"
#include "VTS/VirtualTransportSystem.h"
#include "Streams/GeometryStream.h"
#include "Loaders/GeometryConvertor.h"
#include "Meta/CompilerAlgo.h"
#include "Json/JsonHelpers.h"

inline auto format_as(DXGI_FORMAT f) { return fmt::underlying(f); }
inline auto format_as(D3D11_USAGE f) { return fmt::underlying(f); }

namespace yaget
{
    namespace io
    {
        namespace render
        {
            //-------------------------------------------------------------------------------------------------------------------------------
            class PakAsset : public Asset
            {
            public:
                PakAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
                {
                    Mesh = io::DeserializeBuffer(buffer);
                }

                std::vector<io::GeometryData> Mesh;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class GeomAsset : public Asset
            {
            public:
                GeomAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
                {
                    Mesh = std::make_shared<yaget::render::GeometryConvertor>(buffer.first.get(), buffer.second, nullptr);
                }

                yaget::render::GeometryConvertor::Ptr Mesh;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class ShaderAsset : public Asset
            {
            public:
                ShaderAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts) {}
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class FontAsset : public Asset
            {
            public:
                FontAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts) {}
            };


            namespace internal
            {
                template<class TupType, size_t... I>
                void yaget_print(const TupType& _tup, std::index_sequence<I...>, std::string& message)
                {
                    (..., (message += (I == 0 ? "" : ", ") + conv::Convertor<std::tuple_element_t<I, TupType>>::ToString(std::get<I>(_tup))));
                }

                template<class... T>
                void yaget_print(const std::tuple<T...>& _tup, std::string& message)
                {
                    yaget_print(_tup, std::make_index_sequence<sizeof...(T)>(), message);
                }
            } // namespace internal

            //-------------------------------------------------------------------------------------------------------------------------------
            template <typename... T>
            class JasonMetaDataAsset : public io::JsonAsset
            {
            public:
                std::string ToString() const
                {
                    std::string message;
                    io::render::internal::yaget_print(mFields, message);

                    return message;
                }

            protected:
                using Section = yaget::io::VirtualTransportSystem::Section;
                using Fields = std::tuple<T...>;
                static constexpr size_t NumFields = std::tuple_size_v<std::remove_reference_t<Fields>>;

                template<std::size_t I>
                using Type = std::tuple_element_t<I, Fields>;
                using Validate = std::function<bool()>;

                template <typename... F>
                JasonMetaDataAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts, const char* rootName, Validate validate, F&&... args)
                    : io::JsonAsset(tag, buffer, vts)
                {
                    if (mValid)
                    {
                        if (mBuffer.second)
                        {
                            for (const auto&[key, node] : root.items())
                            {
                                if (key == rootName && node.is_object())
                                {
                                    mValidSection = true;
                                    root = node;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            mValidSection = true;
                        }
                    }

                    std::string catchMessage;
                    if (mValidSection)
                    {
                        using Resolvers = std::tuple<F...>;
                        Resolvers resolvers = std::tuple<F...>(args...);
                        static_assert(NumFields == std::tuple_size_v<std::remove_reference_t<Resolvers>>);

                        try
                        {
                            yaget::meta::tuple_clone<0>(mFields, resolvers);
                            mValidSection = validate();
                        }
                        catch (const nlohmann::detail::exception& ex)
                        {
                            std::string message;
                            io::render::internal::yaget_print(mFields, message);

                            catchMessage = fmt::format("Unable to assign to mFields from json meta stream : '{}'.Fields : [{}].Error : {}.", tag.ResolveVTS().c_str(), message.c_str(), ex.what());
                            //YLOG_ERROR("ASET", "Unable to asign to mFields from json meta stream: '%s'. Fields: [%s]. Error: %s.", tag.ResolveVTS().c_str(), message.c_str(), ex.what());
                            mValidSection = false;
                        }
                    }

                    if (!mValidSection)
                    {
                        if (!catchMessage.empty())
                        {
                            YLOG_ERROR("ASET", "'%s'", catchMessage.c_str());
                        }
                        else
                        {
                            std::string message;
                            io::render::internal::yaget_print(mFields, message);
                            YLOG_ERROR("ASET", "MetaData Asset: '%s' with Root Name: '%s' failed data validation with values: '%s'.", tag.ResolveVTS().c_str(), rootName ? rootName : "", message.c_str());
                        }
                    }

                    mValid = mValidSection;
                }

                template <std::size_t I>
                const Type<I>& Get() const
                {
                    return std::get<I>(mFields);
                }

                Fields mFields;

            private:
                bool mValidSection = false;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            struct Layer
            {
                // How this layer is interpret for rendering and blending with previous
                // Replace - replace all inputs with this one
                // Flatten - merge this inputs with previous layer and render as one pass
                // Overlay - render as separate pass on top of the previous one
                enum class Operation { Invalid, Replace, Flatten, Overlay };

                yaget::io::Tag mGeometryTag;
                yaget::io::Tag mMaterialTag;
                yaget::io::Tag mTextureTag;
                yaget::io::Tag mDepthStateTag;
                yaget::io::Tag mRasterizerStateTag;
                yaget::io::Tag mBlendStateTag;
                Operation mOperation;
            };

            using Layers = std::unordered_map<std::string, Layer>;

            NLOHMANN_JSON_SERIALIZE_ENUM(Layer::Operation, {
                { Layer::Operation::Invalid, nullptr },
                { Layer::Operation::Replace, "Replace" },
                { Layer::Operation::Flatten, "Flatten" },
                { Layer::Operation::Overlay, "Overlay" }
            })

            //-------------------------------------------------------------------------------------------------------------------------------
            class DescriptionAsset : public JasonMetaDataAsset<Layers>
            {
            public:
                DescriptionAsset(const yaget::io::Tag& tag, yaget::io::Buffer buffer, const yaget::io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "Description",
                        [this]() { return Get<0>().find("Default") != Get<0>().end(); },
                        [this]()
                        {
                            Layers renderLayers;
                            auto layerBlock = root.find("Layers");
                            if (layerBlock != root.end() && layerBlock->is_array())
                            {
                                nlohmann::json layers = *layerBlock;
                                for (const auto& layer : layers)
                                {
                                    auto layerName = json::GetValue<std::string>(layer, "Name", {});
                                    auto geometryTag = mVTS.GetTag(json::GetValue<Section>(layer, "Geometry", {}));
                                    auto materialTag = mVTS.GetTag(json::GetValue<Section>(layer, "Material", {}));
                                    auto textureTag = mVTS.GetTag(json::GetValue<Section>(layer, "Texture", {}));
                                    auto depthStateTag = mVTS.GetTag(json::GetValue<Section>(layer, "DepthState", {}));
                                    auto rasterizerTag = mVTS.GetTag(json::GetValue<Section>(layer, "RasterizerState", {}));
                                    auto blendTag = mVTS.GetTag(json::GetValue<Section>(layer, "BlendState", {}));
                                    auto operation = json::GetValue<Layer::Operation>(layer, "Operation", Layer::Operation::Replace);

                                    if (!layerName.empty() && (geometryTag.IsValid() || materialTag.IsValid() || textureTag.IsValid()) && operation != Layer::Operation::Invalid)
                                    {
                                        if (layerName == "Default" && (!geometryTag.IsValid() || !materialTag.IsValid() || !textureTag.IsValid()))
                                        {
                                            continue;
                                        }

                                        renderLayers.insert({ layerName, Layer{ geometryTag, materialTag, textureTag, depthStateTag, rasterizerTag, blendTag, operation } });
                                    }
                                }
                            }

                            return renderLayers;
                        })
                    , mLayers(std::get<0>(mFields))
                {}

                const Layers& mLayers;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class MaterialAsset : public JasonMetaDataAsset<io::Tag, io::Tag>
            {
            public:
                MaterialAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "Material",
                        [this]() { return Get<0>().IsValid() && Get<1>().IsValid(); },
                        [this]() { return mVTS.GetTag(json::GetValue<Section>(root, "Vertex", {})); },
                        [this]() { return mVTS.GetTag(json::GetValue<Section>(root, "Pixel", {})); })
                    , mVertexTag(std::get<0>(mFields))
                    , mPixelTag(std::get<1>(mFields))
                {}

                const io::Tag& mVertexTag;
                const io::Tag& mPixelTag;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class FontFaceAsset : public JasonMetaDataAsset<io::VirtualTransportSystem::Section, int>
            {
            public:
                using Section = yaget::io::VirtualTransportSystem::Section;

                FontFaceAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "FontFace",
                        [this]() { return Get<1>() > 0; },
                        [this]() { return json::GetValue<Section>(root, "Font", Section()); },
                        [this]() { return json::GetValue<int>(root, "Size", 0); })
                    , mFont(std::get<0>(mFields))
                    , mSize(std::get<1>(mFields))
                {}

                const io::VirtualTransportSystem::Section& mFont;
                const int& mSize = 0;
            };

            //-------------------------------------------------------------------------------------------------------------------------------
            class TextureAsset : public JasonMetaDataAsset<io::Tag, io::Tag>
            {
            public:
                TextureAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "Texture",
                        [this]() { return Get<0>().IsValid() && Get<1>().IsValid(); },
                        [this]() { return mVTS.GetTag(json::GetValue<Section>(root, "Image", {})); },
                        [this]() { return mVTS.GetTag(json::GetValue<Section>(root, "Meta", {})); })
                    , mImageTag(std::get<0>(mFields))
                    , mMetaTag(std::get<1>(mFields))
                {}

                const io::Tag& mImageTag;
                const io::Tag& mMetaTag;
            };


            namespace meta
            {
                uint32_t GetWrapMode(const std::string& name);
                uint32_t GetComparisonFunc(const std::string& name);
                uint32_t GetFilter(const std::string& name);

            } // namespace meta

            //-------------------------------------------------------------------------------------------------------------------------------
            class ImageMetaAsset : public JasonMetaDataAsset<uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, float, float, float>
            {
            public:
                ImageMetaAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "TextureMeta",
                        [this]() { return true; },
                        [this]() { return meta::GetWrapMode(json::GetValue<std::string>(root, "AddressU", {})); },
                        [this]() { return meta::GetWrapMode(json::GetValue<std::string>(root, "AddressV", {})); },
                        [this]() { return meta::GetWrapMode(json::GetValue<std::string>(root, "AddressW", {})); },
                        [this]() { return meta::GetComparisonFunc(json::GetValue<std::string>(root, "ComparisonFunc", {})); },
                        [this]() { return meta::GetFilter(json::GetValue<std::string>(root, "Filter", {})); },
                        [this]() { return json::GetValue<float>(root, "MipLODBias", {});},
                        [this]() { return json::GetValue<float>(root, "MinLOD", {}); },
                        [this]() 
                        { 
                            float maxLoad = json::GetValue<float>(root, "MaxLOD", {}); 
                            maxLoad = maxLoad == -1.0f ? D3D11_FLOAT32_MAX : maxLoad;
                            return std::clamp(maxLoad, std::get<6>(mFields), D3D11_FLOAT32_MAX);
                        })
                    , mAddressU(std::get<0>(mFields))
                    , mAddressV(std::get<1>(mFields))
                    , mAddressW(std::get<2>(mFields))
                    , mComparisonFunc(std::get<3>(mFields))
                    , mFilter(std::get<4>(mFields))
                    , mMipLODBias(std::get<5>(mFields))
                    , mMinLOD(std::get<6>(mFields))
                    , mMaxLOD(std::get<7>(mFields))
                {}

                const uint32_t& mAddressU;
                const uint32_t& mAddressV;
                const uint32_t& mAddressW;
                const uint32_t& mComparisonFunc;
                const uint32_t& mFilter;
                const float& mMipLODBias;
                const float& mMinLOD;
                const float& mMaxLOD;
            };


            //-------------------------------------------------------------------------------------------------------------------------------
            //UINT             Width;
            //UINT             Height;
            //UINT             MipLevels;
            //UINT             ArraySize;
            //DXGI_FORMAT      Format;
            //--DXGI_SAMPLE_DESC SampleDesc;
            //D3D11_USAGE      Usage;
            //UINT             BindFlags;
            //UINT             CPUAccessFlags;
            //UINT             MiscFlags;
            class RenderTargetAsset : public JasonMetaDataAsset<uint32_t, uint32_t, uint32_t, uint32_t, DXGI_FORMAT/*, DXGI_SAMPLE_DESC*/, D3D11_USAGE, uint32_t, uint32_t, uint32_t>
            {
            public:
                RenderTargetAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
                    : JasonMetaDataAsset(tag, buffer, vts, "RenderTarget",
                        [this]() { return true; },
                        [this]() { return json::GetValue<uint32_t>(root, "Width", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "Height", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "MipLevels", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "ArraySize", {}); },
                        [this]() { return json::GetValue<DXGI_FORMAT>(root, "Format", {}); },
                        [this]() { return json::GetValue<D3D11_USAGE>(root, "Usage", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "BindFlags", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "CPUAccessFlags", {}); },
                        [this]() { return json::GetValue<uint32_t>(root, "MiscFlags", {}); })
                    , mWidth(std::get<0>(mFields))
                    , mHeight(std::get<1>(mFields))
                    , mMipLevels(std::get<2>(mFields))
                    , mArraySize(std::get<3>(mFields))
                    , mFormat(std::get<4>(mFields))
                    , mUsage(std::get<5>(mFields))
                    , mBindFlags(std::get<6>(mFields))
                    , mCPUAccessFlags(std::get<7>(mFields))
                    , mMiscFlags(std::get<8>(mFields))
                {}

                const uint32_t& mWidth;
                const uint32_t& mHeight;
                const uint32_t& mMipLevels;
                const uint32_t& mArraySize;
                const DXGI_FORMAT& mFormat;
                const D3D11_USAGE& mUsage;
                const uint32_t& mBindFlags;
                const uint32_t& mCPUAccessFlags;
                const uint32_t& mMiscFlags;
            };

        } // namespace render
    } // namespace io

    namespace conv
    {
        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<io::render::Layers>
        {
            static std::string ToString(const io::render::Layers& value)
            {
                std::string results;
                for (const auto& it : value)
                {
                    results += "{ Name: '" + it.first + "': Geometry: '" + it.second.mGeometryTag.mName + "', Material: '" + it.second.mMaterialTag.mName + "', Texture: '" + it.second.mTextureTag.mName + "' }";
                }

                return results.empty() ? "NULL" : results;
            }
        };

        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<DXGI_FORMAT>
        {
            static std::string ToString(const DXGI_FORMAT& value)
            {
                return fmt::format("DXGI_FORMAT: '{}'", value);
            }
        };

        //-------------------------------------------------------------------------------------------------------------------------------
        template <>
        struct Convertor<D3D11_USAGE>
        {
            static std::string ToString(const D3D11_USAGE& value)
            {
                return fmt::format("D3D11_USAGE: '{}'", value);
            }
        };

    } // namespace conv
} // namespace yaget

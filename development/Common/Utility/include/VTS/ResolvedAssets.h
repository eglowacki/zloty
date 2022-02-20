//////////////////////////////////////////////////////////////////////
// ResolvedAssets.h
//
//  Copyright 6/24/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Contains derived classes from io::Asset for resolver support
//
//
//  #include "VTS/ResolvedAssets.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"
#include "App/FileUtilities.h"
#include "ImageLoaders/ImageProcessor.h"
#include "Json/JsonHelpers.h"
#include "Logger/YLog.h"


namespace yaget::io
{
    //-------------------------------------------------------------------------------------------------------------------------------
    class JsonAsset : public Asset
    {
    public:
        JsonAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
        {
            // We want ability to create instance of json asset type without actual data buffer. This is in essence a default ctor.
            if (mBuffer.second)
            {
                file::imemstream memStream(reinterpret_cast<const char*>(mBuffer.first.get()), mBuffer.second);

                try
                {
                    memStream >> root;
                }
                catch (const nlohmann::detail::exception& ex)
                {
                    YLOG_ERROR("ASET", "Unable to process json stream: '%s' with size: '%d'. Error: %s.", tag.ResolveVTS().c_str(), mBuffer.second, ex.what());
                    mValid = false;
                }
            }
        }

        nlohmann::json root{};
    };

    template <typename... T>
    class StructDataAsset : public JsonAsset
    {
    public:
        std::string ToString() const
        {
            std::string message;
            print_tuple(mFields, message);

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
        StructDataAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts, const char* rootName, Validate validate, F&&... args)
            : JsonAsset(tag, buffer, vts)
        {
            if (mValid)
            {
                if (mBuffer.second)
                {
                    for (const auto& [key, node] : root.items())
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
                    //io::render::internal::yaget_print(mFields, message);

                    catchMessage = fmt::format("Unable to assign to mFields from json meta stream : '{}'.Fields : [{}].Error : {}.", tag.ResolveVTS().c_str(), message.c_str(), ex.what());
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
                    const auto& message = ToString();
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


    //PixelType mColorType = PixelType::None;
    //DataType mDataType = DataType::None;

    //-------------------------------------------------------------------------------------------------------------------------------
    class ImageAsset : public Asset
    {
    public:
        ImageAsset(const io::Tag& tag, io::Buffer buffer, const io::VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts) 
            , mHeader{ image::Header::PixelType::None, image::Header::DataType::RT }
            , mPixels(buffer.second ? image::Process(mBuffer, &mHeader) : io::CreateBuffer(0))
        {
        }

        image::Header mHeader;
        io::Buffer mPixels;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template<typename T>
    inline std::shared_ptr<yaget::io::Asset> ResolveAsset(const io::Buffer& dataBuffer, const io::Tag& requestedTag, const io::VirtualTransportSystem& vts)
    {
        auto asset = std::make_shared<T>(requestedTag, dataBuffer, vts);
        return asset->IsValid() ? asset : nullptr;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // This will load asset (blocking call) and will convert json data to T.
    // It checks for null asset, but it silently ignores.
    template <typename T>
    T LoadBlob(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& section)
    {
        io::SingleBLobLoader<io::JsonAsset> loader(vts, section);
        
        T result = loader.GetAsset<T>([](auto asset)
        {
            T result{};
            if (asset)
            {
                const auto& jsonBlock = asset->root;
                from_json(jsonBlock, result);
            }

            return result;
        });

        return result;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // Helper function to just return json object
    inline std::shared_ptr<io::JsonAsset> LoadJson(io::VirtualTransportSystem& vts, const io::VirtualTransportSystem::Section& section)
    {
        io::SingleBLobLoader<io::JsonAsset> loader(vts, section);
        return loader.GetAsset();
    }

} // namespace yaget::io

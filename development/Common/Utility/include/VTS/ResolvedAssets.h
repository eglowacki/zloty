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
        JsonAsset(const io::Tag& tag, const io::Buffer& buffer, const io::VirtualTransportSystem& vts) : Asset(tag, buffer, vts)
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

    //-------------------------------------------------------------------------------------------------------------------------------
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
        StructDataAsset(const io::Tag& tag, const io::Buffer& buffer, const io::VirtualTransportSystem& vts, const char* rootName, Validate validate, F&&... args)
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

                    catchMessage = std::format("Unable to assign to mFields from json meta stream : '{}'.Fields : [{}].Error : {}.", tag.ResolveVTS().c_str(), message.c_str(), ex.what());
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

    //-------------------------------------------------------------------------------------------------------------------------------
    class TextureAsset : public Asset
    {
    public:
        TextureAsset(const Tag& tag, const Buffer& buffer, const VirtualTransportSystem& vts)
            : Asset(tag, image::GetImage(buffer), vts) 
            , mHeader(*cast_data<image::Header>(mBuffer))
            , mPixels(mHeader.GetImageSize() ? cast_to_view(mBuffer, sizeof(mHeader)) : BufferView{})
        {
            mValid = mHeader.GetImageSize() > 0;
        }

        image::Header mHeader;
        BufferView mPixels;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    // buffer all the data treated as a binary blob. It's up to user to make sense of this data
    class BinAsset : public Asset
    {
    public:
        BinAsset(const Tag& tag, const Buffer& buffer, const VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts)
        {}
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    class StringsAsset : public Asset
    {
    public:
        StringsAsset(const Tag& tag, const Buffer& buffer, const VirtualTransportSystem& vts)
            : Asset(tag, buffer, vts)
        {
            file::imemstream memStream(cast_data<const char>(mBuffer), size_data(mBuffer));

            std::string textLine;
            while (std::getline(memStream, textLine))
            {
                conv::Trim(textLine);
                if (!textLine.empty())
                {
                    mStrings.push_back(textLine);
                    textLine = "";
                }
            }
        }

        Strings mStrings;
    };

    //-------------------------------------------------------------------------------------------------------------------------------
    template<typename T>  //Asset
    requires std::is_base_of_v<Asset, T> && (!std::same_as<Asset, T>)
    inline std::shared_ptr<Asset> ResolveAsset(const Buffer& dataBuffer, const Tag& requestedTag, const VirtualTransportSystem& vts)
    {
        auto asset = std::make_shared<T>(requestedTag, dataBuffer, vts);
        return asset->IsValid() ? asset : nullptr;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // This will load asset (blocking call) and will convert json data to T.
    // It checks for null asset, but it silently ignores.
    template <typename T>
    T LoadBlob(VirtualTransportSystem& vts, const VirtualTransportSystem::Section& section)
    {
        SingleBLobLoader<JsonAsset> loader(vts, section);
        
        T result = loader.GetAsset<T>([&section](auto asset)
        {
            T result{};
            if (asset)
            {
                try
                {
                    const auto& jsonBlock = asset->root;
                    from_json(jsonBlock, result);
                }
                catch (nlohmann::json::exception& ex)
                {
                    YLOG_ERROR("ASET", "Could not convert json: '%s' to: '%s'. %s", section.ToString().c_str(), meta::type_name_v<T>().c_str(), ex.what());
                }
            }

            return result;
        });

        return result;
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // Helper function to just return json object
    inline std::shared_ptr<JsonAsset> LoadJson(VirtualTransportSystem& vts, const VirtualTransportSystem::Section& section)
    {
        SingleBLobLoader<JsonAsset> loader(vts, section);
        return loader.GetAsset();
    }

    //-------------------------------------------------------------------------------------------------------------------------------
    // Helper function to just return json object
    inline std::shared_ptr<JsonAsset> LoadJson(VirtualTransportSystem& vts, const io::Tag& tag)
    {
        SingleBLobLoader<JsonAsset> loader(vts, tag);
        return loader.GetAsset();
    }

    inline void AttachTransientAsset(const yaget::io::Tag& tag, yaget::io::VirtualTransportSystem& vts)
    {
        using namespace yaget;

        if (!vts.FindTag(tag.mGuid).IsValid())
        {
            std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::BinAsset>({}, tag, vts);
            vts.AttachTransientBlob(newAsset);
        }
    }

} // namespace yaget::io

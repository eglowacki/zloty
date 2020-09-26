//////////////////////////////////////////////////////////////////////
// DefaultResolvers.h
//
//  Copyright 9/11/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      
//
//
//  #include "VTS/DefaultResolvers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "VTS/VirtualTransportSystem.h"
#include "Device.h"


namespace yaget
{
    namespace io::tool
    {
        using AssetResolvers = io::VirtualTransportSystem::AssetResolvers;
        using TagResourceResolvers = yaget::render::Device::TagResourceResolvers;

        const AssetResolvers& GetResolvers();
        const TagResourceResolvers& GetTagResolvers();

        void AddResolvers(const AssetResolvers& assetResolvers);
        void AddTagResolvers(const TagResourceResolvers& tagResolvers);

        //-------------------------------------------------------------------------------------------------------------------------------
        template <typename R, typename T>
        std::shared_ptr<yaget::render::ResourceView> ConvertFromTag(const std::shared_ptr<io::Asset>& asset, yaget::render::Device& device)
        {
            auto userAsset = yaget::io::asset_cast<T>(asset);
            return std::make_shared<R>(device, userAsset);
        }

    } // namespace io::tool
} // namespace yaget

//////////////////////////////////////////////////////////////////////
// Populators.h
//
//  Copyright 10/18/2026 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper function to populate and save data from/to file
//
//
//  #include "Streams/Populators.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#pragma once

#include "VTS/ResolvedAssets.h"


namespace yaget::io
{
    //-------------------------------------------------------------------------------------------------
    template<typename T>
    void PopulateMap(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts, T& currentMap)
    {
        T newMap = yaget::io::LoadBlob<T>(vts, fileName);
        if (!newMap.empty() && newMap != currentMap)
        {
            currentMap = newMap;
        }
    }


    //-------------------------------------------------------------------------------------------------
    template<typename T>
    void SaveMap(io::VirtualTransportSystem::Section fileName, io::VirtualTransportSystem& vts, T& currentMap)
    {
        using namespace yaget;

        nlohmann::json jsonBlock = currentMap;
        auto textBlock = json::PrettyPrint(jsonBlock);
        io::Buffer buffer = io::CreateBuffer(textBlock);
        if (auto saveFileTag = vts.GetTag(fileName); saveFileTag.IsValid())
        {
            auto oldMap = io::LoadBlob<T>(vts, saveFileTag);
            if (oldMap.empty() || oldMap != currentMap)
            {
                io::SingleBLobLoader<io::JsonAsset> cacheLoader(vts, saveFileTag);
                auto asset = cacheLoader.GetAsset();
                asset->mBuffer = buffer;
                vts.UpdateAssetData(asset, io::VirtualTransportSystem::Request::UpdateOnly);
            }
        }
        else
        {
            auto newTag = vts.GenerateTag(fileName);
            std::shared_ptr<io::Asset> newAsset = io::ResolveAsset<io::JsonAsset>(buffer, newTag, vts);
            vts.UpdateAssetData(newAsset, io::VirtualTransportSystem::Request::Add);
        }
    }

}

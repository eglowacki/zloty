//////////////////////////////////////////////////////////////////////
// ToolVirtualTransportSystem.h
//
//  Copyright 4/28/2018 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides virtual access to assets for tools with needs for write, read, add and delete
//
//
//  #include "VTS/ToolVirtualTransportSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "VTS/VirtualTransportSystem.h"

namespace yaget::io::tool
{
    //--------------------------------------------------------------------------------------------------
    class VirtualTransportSystem : public io::VirtualTransportSystem
    {
    public:
        VirtualTransportSystem(dev::Configuration::Init::VTSConfigList configList, const AssetResolvers& assetResolvers, const std::string& fileName, RuntimeMode reset);

        enum class Options { Hierarchy, Flat, Recover };
        io::Tag CopyTag(const io::Tag& sourceTag, const Section& targetSection, Options flat) const;


        bool AttachBlob(std::shared_ptr<io::Asset> asset) { return AttachBlob(std::vector<std::shared_ptr<io::Asset>>{ asset }); }
        bool AttachBlob(const std::vector<std::shared_ptr<io::Asset>>& assets);

        bool DeleteBlob(const Section& section) { return DeleteBlob(Sections{ section }); }
        bool DeleteBlob(const Sections& sections);

    private:
        struct Locker : public DatabaseLocker
        {
            Locker(std::mutex& mutex, io::VirtualTransportSystem& vts) : DatabaseLocker(vts), mMutex(mutex) { mMutex.lock(); }
            ~Locker() { mMutex.unlock(); }

        private:
            std::mutex& mMutex;
        };

        DatabaseHandle LockDatabaseAccess() override { return std::make_unique<Locker>(mDatabaseMutex, *this); }
        DatabaseHandle LockDatabaseAccess() const override { return std::make_unique<Locker>(mDatabaseMutex, const_cast<VirtualTransportSystem&>(*this)); }

        bool mKeepWaiting = true;
        mutable std::mutex mDatabaseMutex;
    };
    
    //--------------------------------------------------------------------------------------------------
    // used to start one db with reset data, and second one to re-use existing data (helper classes only)
    class VirtualTransportSystemDefault : public io::tool::VirtualTransportSystem
    {
    public:
        VirtualTransportSystemDefault(dev::Configuration::Init::VTSConfigList configList, const AssetResolvers& assetResolvers, const std::string& fileName = "$(DatabaseFolder)/vts.sqlite")
            : VirtualTransportSystem(std::move(configList), assetResolvers, fileName, RuntimeMode::Default)
        {}
    };
    
    //--------------------------------------------------------------------------------------------------
    class VirtualTransportSystemReset : public io::tool::VirtualTransportSystem
    {
    public:
        VirtualTransportSystemReset(dev::Configuration::Init::VTSConfigList configList, const AssetResolvers& assetResolvers, const std::string& fileName = "$(DatabaseFolder)/vts.sqlite")
            : VirtualTransportSystem(configList, assetResolvers, fileName, RuntimeMode::Reset)
        {}
    };

}


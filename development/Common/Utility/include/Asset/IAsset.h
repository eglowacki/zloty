///////////////////////////////////////////////////////////////////////
// IAssetResource.h
//
//  Copyright 11/6/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to abstract asset class
//
//
//  #include "Asset/IAsset.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

/*!
\mainpage Muck Technology Manual

\section muck_Introduction Introduction
This represents years of work.
*/


#ifndef ASSET_I_ASSET_H
#define ASSET_I_ASSET_H
#pragma once

#include "Base.h"
#include "Logger/Log.h"
#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#pragma warning(push)
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include <boost/function.hpp>
#pragma warning(pop)


namespace eg
{
    //! Forward declarations
    class Message;
    class Hash;
    namespace internal {class Locker;}

    /*!
    \brief Base Asset class for managing shared resources.

    Any object which will load data, share and manage assets
    needs to be derived from this interface. It provides concept
    of Lock() where it will assure that no changes to resource will occur while
    Locker_t token is in scope.
    */
    class IAsset
    {
    public:
        virtual ~IAsset() = 0 {}

        //! Each derived class will redeclare this with its own unique ID
        static const guid_t kType = 0x0;

        /*!
        \brief Return hash value representing this asset

        Returned unique ID of this asset instance.
        \return uint64_t representing unique ID for this asset instance
        */
        virtual uint64_t GetId() const = 0;
        //! Return name of this asset. This may or may not be unique
        virtual const std::string& Name() const = 0;
        //! Valid path for Assets it it has one or empty string
        virtual const std::string& Fqn() const = 0;

        //! Return factory type. If you have derive class, you can use
        //! DeriveClass::kType value to determined factory,
        //! or at base level use this method.
        virtual guid_t Type() const = 0;

        //! ANy derive class can provide this implementation, but it's only used in the derive class
        //! context, so not all class need this functioanlity
        virtual void HandleMessage(Message& /*msg*/) {;}

        // DEPRECATED
        /*!
        \brief Lock token to control lifespan

        This is returned from Lock() method when it successfully locked the object.
        When goes out of scope it will unlock this object.
        */
        typedef boost::shared_ptr<internal::Locker> Locker_t;
        /*!
        \brief Return Locker_t object when it successfully locked this object.

        This will return Locker_t object when it can lock resource (can not be modified by other threads,
        like during rendering of mesh). It tries to lock and if it succeed it returns immediately with
        valid Locker_t object, otherwise returns (Locker_t)NULL object.
        \return Locker_t representing locked status token. When token goes out of scope
                it will unlock this object.
        */
        virtual Locker_t Lock() = 0;

        //! This is used by derive asset classes for it's factory and management
        //! by user. Derive classes usually will provide static method
        //! for registration of factories with AssetLoadSystem, and will
        //! return Token which will control life of Factory and of those
        //! assets implicitly.
        struct Factory
        {
            virtual ~Factory() = 0;
        };

        typedef boost::shared_ptr<Factory> Token;
    };

    inline IAsset::Factory::~Factory()
    {}

    namespace yaget
    {
        //! Helper function to print name and id of variuos objects (this is for IAsset)
        inline std::string name_id(const IAsset* asset)
        {
            if (asset)
                return "'" + asset->Name() + ":" + boost::lexical_cast<std::string>(asset->GetId()) + "'";

            return "':'";
        }

        //! This class provides helpers for managing
        //! loading of data from different versions
        //!
        template <typename T>
        class VersionHandler
        {
        public:
            typedef boost::function<void (T& /*asset*/, std::istream& /*stream*/)> VersionLoader_t;

            VersionHandler()
            {}

            VersionLoader_t& operator[](int i)
            {
                return mLoaders[i];
            }

            void load(uint32_t i, T& asset, std::istream& stream)
            {
                if (mLoaders.empty()) {return;}

                std::map<uint32_t, VersionLoader_t>::iterator it = mLoaders.find(i);
                if (it == mLoaders.end())
                {
                    if (i <(*mLoaders.rbegin()).first)
                    {
                        log_error << "Requested version " << i << " does not have a handler";
                        throw ex::serialize("Requested version does not have a handler.");
                    }

                    log_warning << "Requested version " << i << " is newer then handler: " << (*mLoaders.rbegin()).first;
                }

                for (std::map<uint32_t, VersionLoader_t>::iterator it_v = mLoaders.begin(); it_v != it; ++it_v)
                {
                    (*it_v).second(asset, stream);
                }
                if (it != mLoaders.end())
                {
                    (*it).second(asset, stream);
                }
            }

        private:
            std::map<uint32_t, VersionLoader_t> mLoaders;
        };


    } // namespace yaget

#define trAsset wxT("Asset")

} // namespace eg

#endif // ASSET_I_ASSET_H


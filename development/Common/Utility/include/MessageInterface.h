///////////////////////////////////////////////////////////////////////
// MessageInterface.h
//
//  Copyright 1/23/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This file provides message interface for message sender
//      and message target.
//
//
//  #include "MessageInterface.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MESSAGE_INTERFACE_H
#define MESSAGE_INTERFACE_H
#pragma once

#include "Base.h"
#include <cassert>
#include <boost/any.hpp>


namespace eg
{
    /*!
    Message types which get send to components
    */
    namespace mtype
    {
        //! Not used for any message
        static const guid_t kDummy = 0;

        /*!
        There are several messages trough out a run-time
        of application which are send on specific channels
        system wide
        */
        //! After all initialization is done. This only refers to Utility and Registrate
        //! and it's send by internal system
        //! boost::any not used.
        //! send from RegistrateModule::OnInit()
        static const guid_t kInitEnd = 0xf27ad7e4;

        //! Right before we start shut down prcedure
        //! boost::any - not used.
        static const guid_t kShutdownBegin = 0x127ad809;

        //! All shut down is done. This only refers to plugin systems
        //! boost::any not used.
        static const guid_t kShutdownEnd = 0x127ad80d;

        //! Send for every new to register plugin name
        //! boost::any is wsString which will contain name of plugin.
        //! wsString name = msg.GetValue<wsString>();
        static const guid_t kRegisterPlugin = 0x9279fb49;

        //! Send every frame from internal subsystems,
        //! boost::any is float representing time since last frame.
        //! float deltatTime = msg.GetValue<float>();
        static const guid_t kFrameTick = 0x918b5491;

        //! This is send from within modal and blocking input, but
        //! still allow refreshing of render view's
        //! the delta time for this refresh tick will always be 0
        //! boost::any not used.
        static const guid_t kFrameTickRefresh = 0xf622ebba;

        //! Send by asset resource factory when it starts to load asset - RENAME TO: kResourceLoadBegin
        static const guid_t kBeginLoadResource = 0x91a00281;

        //! Request application quit. Closing will happend on the next idle loop
        static const guid_t kRequestQuit = 0x91735723;

        //! This is send to dispatcher when component is created
        //! first time for this object id and is send before
        //! component is created. IOW, there will be no Components
        //! belonging to this ObjectId.
        //! boost::any component::ObjectId
        //! component::ObjectId id = msg.GetValue<component::ObjectId>();
        static const guid_t kObjectNew = 0xd466fa9f;

        //! This is send by ObjectTemplates class when all
        //! components for specific object are created
        //! boost::any component::ObjectId
        //! component::ObjectId id = msg.GetValue<component::ObjectId>();
        static const guid_t kObjectConstructed = 0xf583e07a;

        //! Same as kObjectNew but when all Components are deleted
        //! and it's guarantied to be send after last remaining Component
        //! is completely removed.
        //! boost::any component::ObjectId
        //! component::ObjectId id = msg.GetValue<component::ObjectId>();
        static const guid_t kObjectDelete = 0x1466fad4;

        //! This is send to dispatcher when component is created
        //! and it's assured that component is fully loaded and initialized
        //! boost::any component::InstanceId
        //! component::InstanceId id = msg.GetValue<component::InstanceId>();
        //! It also used in ObjectManager to send to all registered objects.
        static const guid_t kComponentNew = 0xd27f4e59;

        //! This is send to dispatcher when component is deleted
        //! and it's assured that component is still initialized
        //! boost::any component::InstanceId
        //! component::InstanceId id = msg.GetValue<component::InstanceId>();
        static const guid_t kComponentDelete = 0xd27f4e5c;

        //! This is send when component is selected
        //! boost::any component::InstanceId
        static const guid_t kComponentSelected = 0x72d62781;

        //! This is send when component is de-selected
        //! boost::any component::InstanceId
        static const guid_t kComponentDeselected = 0x72d62784;

        //! Send by framework after Frame::Size(...) method was called.
        //! boost::any Vector2
        //! used as x = width and y = height, After extracting you can cast it to unit32_t
        static const guid_t kVideoSizeChanged = 0xf4a67271;
    } // namespace mtype


    /*!
    Basic message object which gets passed to Target::HandleMessage method
    */
    class Message
    {
    public:
        Message()
        : mType(mtype::kDummy)
        {}

        Message(guid_t type)
        : mType(type)
        {}

        Message(guid_t type, const boost::any& anyData)
        : mType(type)
        , mAnyData(anyData)
        {}

        //! Return TRUE if there is any kind of data attached to this message, otherwise FALSE
        //! This does not trigger any errors
        bool IsValid() const
        {
            return !mAnyData.empty();
        }

        //! Return TRUE if there is any kind of data attached to this message and it's of type T
        //! This does not trigger any errors
        template <typename T>
        bool Is() const
        {
            return IsValid() && mAnyData.type() == typeid(T);
        }

        //! Attach data to this message
        void SetValue(const boost::any& anyData)
        {
            mAnyData = anyData;
        }


        template <typename T>
        operator T() const
        {
            return GetValue<T>();
        }

        //! Return data attached to this message.
        //! If there is no data or it's not of correct type
        //! it logs error and return value of type T()
        template <typename T>
        T GetValue() const
        {
            try
            {
                return boost::any_cast<T>(mAnyData);
            }
            catch (const boost::bad_any_cast& er)
            {
                er;
                assert(0); //"Fix me"
//#ifndef __cplusplus_cli
//                wxLogError("Message id %x mAnyData: %s", mType, er.what());
//#endif// __cplusplus_cli
                return T();
            }
        }

        virtual ~Message() {}

        //! This is not provided here, since entire MessageManager is
        //! in a separate DLL's, but I can actually implement this method in Message DLL,
        //! so IOW, this will not work if you do not include BaseMuck project.
        //! Automatic global sending of this message
        //void Send();
        //! Same comments apply about implementation of this method plus, it will make sure that
        //! Message will be send on the main thread at some point
        //void Post();

        guid_t mType;
        boost::any mAnyData;
    };

} // namespace eg

#endif // MESSAGE_INTERFACE_H


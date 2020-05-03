//////////////////////////////////////////////////////////////////////
// MessageFactory.h
//
//  Copyright 3/25/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provided Message Factory, where additional class can register for specific
//      handling of network messages for packing and unpacking with out changing any
//      network code. At this level, there is no network code.
//
//
//  #include "Network/MessageFactory.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef NETWORK_PACKET_FACTORY_H
#define NETWORK_PACKET_FACTORY_H
#pragma once

#if 0

#include "Registrate.h"
#include <map>
#include <boost/function.hpp>

// forward declarations
namespace RakNet {class BitStream;}
struct Packet;

namespace eg
{
    class Message;

    namespace network
    {
        // forward declarations
        class MessageHandler;

        //! std::string is name of parameter
        //! boost::any specific parameter data
        typedef std::map<std::string, boost::any> Paramters_t;

        template <typename T>
        inline T FindParam(const Paramters_t& parameters, const std::string& name, T defaultValue)
        {
            Paramters_t::const_iterator it = parameters.find(name);
            if (it != parameters.end())
            {
                try
                {
                    return boost::any_cast<T>((*it).second);
                }
                catch (const boost::bad_any_cast& /*er*/)
                {
                    return defaultValue;
                }
            }

            return defaultValue;
        }

        /*!
        Sample of code to create guarantied network packet
        \code
        Paramters_t parameters;
        parameters["PacketPriority"] = HIGH_PRIORITY;
        parameters["PacketReliability"] = RELIABLE_ORDERED;
        parameters["OrderingChannel"] = (char)0;
        parameters["SystemAddress"] = SystemAddress;
        parameters["Broadcast"] = false;
        \endcode
        */

        //! When Process is called, it can also call this callback function if provided
        //! \param RakNet::BitStream data to send to network
        //! \param Paramters_t extra user parameters, specific to network message and
        //!   std::string is name of parameter
        //!   boost::any specific parameter data
        //! \return TRUE, message was send, this does not means that it was received successfully, otherwise FALSE
        typedef boost::function<bool (RakNet::BitStream& /*dataToSend*/, const Paramters_t& /*parameters*/)> NetworkMeesageCallback_t;

        /*!
        Called by the network layer (this is used in client and server side)
        to serialize and deserialize network messages
        All the handlers provide 2 unique id's
        one is for which system (grouping) and second actual message type
        This is useful if we share MessageFactory objects across several modules
        and each module might handle the same type of message.
        */
        class MessageFactory
        {
            //@{
            //! no copy semantics
            MessageFactory(const MessageFactory&);
            MessageFactory& operator =(const MessageFactory&);
            //@}

        public:
            MessageFactory();

            //! This will call registered MessageHandler for type if one exist
            //! \param pPacket network message packet
            //! \param systemId unique system id this packet type will be processed by
            //! \param type what kind of message type it is
            //! \param messageCallback this can be called, which will send data over network
            //! \return TRUE, was able to Process, otherwise FALSE
            bool Process(Packet *pPacket, uint32_t systemId, int type, NetworkMeesageCallback_t messageCallback);
            //! This is the same method as above one, but will use global callback function set
            //! using  SetMessageCallabck(...)
            bool Process(Packet *pPacket, uint32_t systemId, int type);

            //! This will return pointer to handler for specific type
            //! or NULL if there is n't one
            //! \param type  what kind of message type this handler handles
            //! \return pointer to handler if one exist
            MessageHandler *FindHandler(uint32_t systemId, int type) const;

            //! This will provide process with message callback.
            void SetMessageCallabck(uint32_t systemId, NetworkMeesageCallback_t messageCallback);

            //! Remove registered message handler for specific systemId and type
            void removeMessageHandler(uint32_t systemId, int type);

            //! Return current network state, which can be one of the network::State enum
            uint32_t GetState() const;

        private:
            friend MessageHandler;
            //! Register message handler for specific systemId and type
            void addMessageHandler(MessageHandler *pMessageHandler, uint32_t systemId, int type);
            //! Triggered by mtype::kNetworkConnectionState
            void onNetworkState(const Message& msg);

            //! Each handler compromises of 2 keys
            //! first - unique subsystem id, like LoginServer, Client, ConnectionServer, etc
            //! second - message type to handle
            typedef std::pair<uint32_t, int> HandlerKey_t;
            //! map of message handler, where
            //! first key to message handler
            //! second pointer to instance of handler class
            typedef std::map<HandlerKey_t, MessageHandler *> MessageHndlers_t;
            // all registered handlers for messages
            MessageHndlers_t mHandlers;

            typedef std::map<uint32_t, NetworkMeesageCallback_t> MessageCallbacks_t;
            MessageCallbacks_t mMessageCallbacks;
            //! Current network state
            uint32_t mNetworkState;
        };


        /*!
        This is used to provide specialized derived classed to handle network
        messages to serialize and deserialize.
        */
        class MessageHandler
        {
        public:
            //! This will process data extracted from pData
            //! \param pPacket network message packet
            //! \param messageCallback this can be called, which will send data over network
            //! \return TRUE, was able to deserialize, otherwise FALSE
            virtual bool Process(Packet *pPacket, NetworkMeesageCallback_t messageCallback) = 0;

        protected:
            MessageHandler(uint32_t systemId, int type)
            {
                MessageFactory& mf = REGISTRATE(MessageFactory);
                mf.addMessageHandler(this, systemId, type);
            }
        };

        /*
        To use this as a singleton
        \code
        #include "Network/MessageFactory.h"
        MessageFactory *mf = registrate::p_cast<MessageFactory>(wxT("MessageFactory"));
        // or ...
        MessageFactory& mf = registrate::ref_cast<MessageFactory>(wxT("MessageFactory"));
        // or ...
        MessageFactory& mf = REGISTRATE(MessageFactory);   // macro alert
        \endcode
        */

    } // namespace network
} // namespace eg

#endif // 0
#endif // NETWORK_PACKET_FACTORY_H


///////////////////////////////////////////////////////////////////////
// INetwork.h
//
//  Copyright 03/18/2007 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      INetwork object interface
//
//
//  #include "INetwork.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef INETWORK_H
#define INETWORK_H
#pragma once

#include "Plugin/IPluginObject.h"
#include <vector>
#include <map>
#include <boost/any.hpp>

// forward decelerations
namespace RakNet { class BitStream; }

namespace eg
{
    namespace network
    {
        /*!
        Used by mtype::kLoginInfo message
        */
        struct UserInfo
        {
            std::string User;       ///< User Name for login
            std::string Password;   ///< User Password for login
        };

        /*!
        Used by mtype::kWorldList message as vector of WorldInfo
        and kWorldLogin to specify which world to connect to
        */
        struct WorldInfo
        {
            std::string Name;
        };

        /*!
        Send by the Login Server in response to ID_USER_WORLD_SELECTED.
        Server uses ID_USER_WORLD_CONNECTION_INFO. This allows client to
        connect to world server.

        It is also used by client to initiate ConnectionServer login in response to
        ID_USER_WORLD_SELECTED
        */
        struct WorldConnection
        {
            std::string ServerAddress;  ///< World server address for world connection
            unsigned short  Port;       ///< Port id of world server
            uint32_t LoginCookie;       ///< Pass this to ConnectionServer as a verification of valid player
        };

		/*!
		Structure which is send by ConnectionServer to LoginServer
		to register it's worlds and connection info
		*/
		struct WorldRegistration
		{
			std::string ClusterName;			///< Unique name for this entrire world servers (cluster)
			WorldConnection ConnectionInfo;		///< Connection to this server for clients use
			std::vector<WorldInfo> WorldList;	///< List of all worlds provided by this ConnectionServer
		};


		// used as a forward decleration
        typedef std::map<std::string, boost::any> Paramters_t;

        enum State
        {
			esEmpty,			///< no connection state
            esLoginServer,		///< connected to login server
            esConnectionServer,	///< connected to connections server (world)
        };

    } // namespace network

    /*!
    Message types which are used by network system (purely client side)
    */
    namespace mtype
    {
        //! This is send by MessageHandlers during transition from one
        //! connection state to another, like going from login server
        //! to connection server, etc.
        //! boost::any - uint32_t - cuurent state (this should be one of the network::State enum)
        static const guid_t kNetworkConnectionState = 0xf32bce74;

        //! Send by client with user name and password to tell Network
        //! layer to connect to login server
        //! boost::any - will contain UserInfo structure
        static const guid_t kLoginInfo = 0x730ae0c4;

        //! Send by Network layer when it received list of available world
        //! boost::any - std::vector<WorldInfo>
        static const guid_t kWorldList = 0x931279ed;

        //! Send by client to Login Server after user selects world to login
        //! boost::any - WorldInfo
        static const guid_t kWorldSelected = 0x931279f0;

        //! Send by client to Network Layer to connect to world using connection server
        //! where connection info is specified by WorldConnection
        //! boost::any - WorldConnection
        static const guid_t kWorldLogin = 0x732734f0;

        //! Send by network layer when there is an error
        //! boost::any - std::string - text of error message
        static const guid_t kNetworkError = 0x3327f888;

    } // namespace mtype


    /*!
    Interface to our Network transport object
    */
    class DLLIMPEXP_UTIL_CLASS INetwork : public IPluginObject
    {
    public:
        //! Connection to specific server using provided port
        //! Return TRUE if all is OK. This does not mean that there was successful connection
        //! to server. If there is connection already, it will do nothing, regardless of current
        //! server and port
        //! After successfully connecting to server, server will send
        //! ID_CONNECTION_REQUEST_ACCEPTED message
        virtual bool Connect(const std::string& serverAddress, unsigned short serverPort) = 0;

        //! This will disconnect and reset to non connected state
        virtual void ResetConnection() = 0;

        //! Send data to network connection
        //! How data is send depends on Paramters_t
        //! Return TRUE if packet was send. This does not guarantee that packet was delivered.
        virtual bool Send(RakNet::BitStream& dataToSend, const network::Paramters_t& parameters) = 0;
    };

} // namespace eg

#endif // INETWORK_H


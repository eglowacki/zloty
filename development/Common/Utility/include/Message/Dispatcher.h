///////////////////////////////////////////////////////////////////////
// Dispatcher.h
//
//  Copyright 11/27/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Main dispatcher of events
//      Used as registrate type
//
//
//  #include "Message/Dispatcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef MESSAGE_DISPATCHER_H
#define MESSAGE_DISPATCHER_H
#pragma once


#include "MessageInterface.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#pragma warning(push)
#pragma warning (disable : 4244)  // '' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning (disable : 4512)  // '' assignment operator could not be generated
    #include <boost/thread/mutex.hpp>
    #include <boost/signal.hpp>
#pragma warning(pop)


namespace eg
{
    class Message;

    /*!
    Global dispatcher of events. This class provides channels which are
    identified by unique id (guid) and each channel allows connection
    of callback of Dispatcher::Channel_t signature.
    \note This is registered with Registrate.

    Example of usage of channels:
    Free function
    \code
        void Shutdown(Message& msg);
        disp[mtype::kShutdown].connect(&Shutdown);
    \endcode

    Function object
    \code
        struct ShutdownStruct
        {
            void operator()(Message& msg);
        };
        disp[mtype::kShutdown].connect(ShutdownStruct());
    \endcode

    Class method
    \code
        void MyClass::Shutdown(Message& msg) {...}
        disp[mtype::kShutdown].connect(boost::bind(&MyClass::Shutdown, this, _1));
    \endcode


    Too trigger Channel_t, so all connection are notified
    \code
        Message msg;
        disp[mtype::kShutdown](msg);
    \endcode
    or just
    \code
        Message(mtype::kShutdown).Send();
    \endcode
    */
    class Dispatcher
    {
        // no copy semantics
        Dispatcher(const Dispatcher&);
        Dispatcher& operator=(const Dispatcher&);

    public:
        //! signal connections do not disconnect in dtor
        //! so we provide simple wrapper to auto disconnect
        //! in dtor with correct copy semantics when passing
        //! ownership
        struct connection
        {
            connection(const boost::signals::connection& con)
            : mCon(con)
            {
            }

            connection(const connection& con)
            : mCon(const_cast<connection&>(con).mCon.release())
            {
            }

            ~connection()
            {
                //mCon.disconnect();
            }

        private:
            connection& operator=(const connection& con);
            boost::signals::scoped_connection mCon;
        };

        typedef void Result_t;

        //! Channel type callback signature
        //! Result_t returned from callback
        //! Message ref which will contain data specific to this channel
        typedef boost::signal<typename Result_t (const Message&)> Channel_t;
        typedef boost::shared_ptr<Channel_t> SharedChannel_t;
        typedef std::map<guid_t, SharedChannel_t> ChannelList_t;

        Dispatcher();
        ~Dispatcher();

        Channel_t& operator[](guid_t);

        void Clear() {mChannelList.reset();}

    private:
        boost::shared_ptr<ChannelList_t> mChannelList;
    };

    typedef Dispatcher::connection dcon;


    struct next_tick
    {
        typedef boost::function<void ()> Callback_t;

        next_tick(Dispatcher& dsp, Callback_t callback)
        : callback(callback)
        , con(new boost::signals::connection)
        {
            *con = boost::signals::connection(dsp[mtype::kFrameTick].connect(*this));
        }

        void operator()(const Message& /*msg*/) const
        {
            callback();
            con->disconnect();
        }

        Callback_t callback;
        boost::shared_ptr<boost::signals::connection> con;
    };

} // namespace eg

#endif // MESSAGE_DISPATCHER_H


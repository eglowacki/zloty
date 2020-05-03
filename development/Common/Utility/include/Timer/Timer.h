///////////////////////////////////////////////////////////////////////
// Timer.h
//
//  Copyright 11/11/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Timer object which uses thread and boost::signal to notify wathcer
//      that time expired
//
//
//  #include "Timer/Timer.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef TIMER_TIMER_H
#define TIMER_TIMER_H
#pragma once

#if 0
#include <Synchronization/ConditionObject.h>
#include <boost/signal.hpp>

namespace eg
{

    /*!
    This will trigger expire signal when it's done is past.
    If you need to quit timer before it expires, create as joined,
    otherwise you can just detached (Free type)
    */
    class Timer : public wxThread
    {
    public:
        enum type {eJoined, eFree};

        Timer(float duration, const boost::any& userData = boost::any(), type attach = eFree, uint32_t id = 0, bool bAutoStart = false);
        Timer(float duration, float interval, type attach, uint32_t id = 0, bool bAutoStart = false);
        ~Timer();

        //! Call this methods to actually start timer
        void Start();
        //! Stop timer right now. This will not trigger Expired event
        void Stop();

        //! Sig's should be set before calling any Start(...) method
        //! Called after timer expired (duration, id, user_data)
        boost::signal<void (float, uint32_t, const boost::any&)> sigExpired;
        //! Called every tick (duration, interval, id, user_data) (NOT IMPLEMENTED YET)
        boost::signal<void (float, float, uint32_t, const boost::any&)> sigTick;

    protected:
        //! Main entry point called by new thread created by this object
        virtual void *Entry();

    private:
        uint32_t mId;
        float mDuration;
        Condition m_MoreTime;
        boost::any mUserData;
        bool mbAutoStart;
    };


    inline Timer::Timer(float duration, const boost::any& userData, type attach, uint32_t id, bool bAutoStart)
    : wxThread((attach == eJoined ? wxTHREAD_JOINABLE : wxTHREAD_DETACHED))
    , mId(id)
    , mDuration(duration)
    , m_MoreTime(duration)
    , mUserData(userData)
    , mbAutoStart(bAutoStart)
    {
        if (mbAutoStart)
        {
            Start();
        }
    }

    inline Timer::Timer(float duration, float /*interval*/, type attach, uint32_t id, bool bAutoStart)
    : wxThread((attach == eJoined ? wxTHREAD_JOINABLE : wxTHREAD_DETACHED))
    , mId(id)
    , mDuration(duration)
    , m_MoreTime(duration)
    , mbAutoStart(bAutoStart)
    {
        if (mbAutoStart)
        {
            Start();
        }
    }

    inline Timer::~Timer()
    {
        if (!IsDetached())
        {
            Stop();
        }
    }


    inline void *Timer::Entry()
    {
        do
        {

        } while (m_MoreTime);

        if (!m_MoreTime.IsQuit())
        {
            sigExpired(mDuration, mId, mUserData);
        }
        return 0;
    }

    inline void Timer::Start()
    {
        Create();
        Run();
    }

    inline void Timer::Stop()
    {
        m_MoreTime.Quit();
        Wait();
    }


} // namespace eg

#endif // 0
#endif // TIMER_TIMER_H


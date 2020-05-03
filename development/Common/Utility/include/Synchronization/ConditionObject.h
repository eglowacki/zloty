///////////////////////////////////////////////////////////////////////
// ConditionObject.h
//
//  Copyright 11/11/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Provides wrapper around condition and mutex to
//      provide condtion variable with Signal and Wait calls exposed
//
//
//  #include "Synchronization/ConditionObject.h"
//
//////////////////////////////////////////////////////////////////////
//! \file


#ifndef SYNCHRONIZATION_CONDITION_OBJECT_H
#define SYNCHRONIZATION_CONDITION_OBJECT_H
#pragma once

#include "Base.h"

//#include <wx/thread.h>
#include <boost/scoped_ptr.hpp>


namespace eg
{
    class Condition
    {
    public:
        Condition() {}
        Condition(float /*timeOut*/) {}
        ~Condition() {}

        void Signal() {}
        void Quit() {}
        bool IsQuit() const {return true;}

        operator bool() {return false;}
    };

#if 0
    class Condition
    {
    public:
        Condition();
        Condition(float timeOut);
        ~Condition();

        void Signal();
        void Quit();
        bool IsQuit() const;

        operator bool()
        {
            if (mQuit)
            {
                return false;
            }

            wxMutexLocker lock(*mpMutex);
            if (mTimeOut == 0)
            {
                mpCond->Wait();
            }
            else
            {
                if (mpCond->WaitTimeout(static_cast<unsigned long>(mTimeOut * 1000.0f)) != wxCOND_NO_ERROR)
                {
                    return false;
                }
            }

            return mQuit == false;
        }

    private:
        //! If this is anything then 0, then it will use this value
        //! as a mac time out in wait and it will return false
        float mTimeOut;
        //! Set by Quit() method to signal condition that it needs to to quit
        volatile bool mQuit;
        //{@
        //! Used to notify that some event accured, like
        //! new job in a queue, or it needs to quit
        boost::scoped_ptr<wxMutex> mpMutex;
        boost::scoped_ptr<wxCondition> mpCond;
        //}@
    };


    inline Condition::Condition() :
        mQuit(false),
        mTimeOut(0)
    {
        mpMutex.reset(new wxMutex);
        mpCond.reset(new wxCondition(*(mpMutex.get())));
    }

    inline Condition::Condition(float timeOut) :
        mQuit(false),
        mTimeOut(timeOut)
    {
        mpMutex.reset(new wxMutex);
        mpCond.reset(new wxCondition(*(mpMutex.get())));
    }

    inline Condition::~Condition()
    {
        // it should have been called Quit before dtor runs.
        //wxASSERT(mQuit);
        if (!mQuit)
        {
            Quit();
        }
    }

    inline void Condition::Signal()
    {
        mpCond->Signal();
    }

    inline void Condition::Quit()
    {
        mQuit = true;
        mpCond->Signal();
    }

    inline bool Condition::IsQuit() const
    {
        return mQuit;
    }

    /*

    inline operator Condition::bool()
    {
        if (mQuit)
        {
            return false;
        }

        wxMutexLocker lock(*mpMutex);
        mpCond->Wait();

        return mQuit == false;
    }
    */
#endif // 0

} // namespace eg

#endif // SYNCHRONIZATION_CONDITION_OBJECT_H


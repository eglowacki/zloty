///////////////////////////////////////////////////////////////////////
// .h
//
//  Copyright 5/1/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Wrapper class to provide critical section modeles
//      on Win32, since wxWidgtes does not have it
//
//
//  #include "Synchronization/CriticalSection.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef SYNCHRONIZATION_CRITICAL_SECTION_H
#define SYNCHRONIZATION_CRITICAL_SECTION_H
#pragma once

#if 0
#include <Base.h>


namespace eg
{

    class CSData;

    /*!
    Simple replication of critical section based on Win32
    */
    class CriticalSection
    {
        //@{
        //! no copy semantics
        CriticalSection(const CriticalSection&);
        CriticalSection& operator=(const CriticalSection&);
        //@}

    public:
        CriticalSection();
        ~CriticalSection();

        bool Try();
        void Lock();
        void Unlock();

    private:
        //CRITICAL_SECTION
        CSData *mCSData;
    };


    //! Helper class to lock and unlock in ctor and dtor respectively
    class CriticalSectionLocker
    {
    public:
        CriticalSectionLocker(CriticalSection& criticalSection) :
            mCriticalSection(criticalSection)
        {
            mCriticalSection.Lock();
        }

        ~CriticalSectionLocker()
        {
            mCriticalSection.Unlock();
        }

    private:
        CriticalSection& mCriticalSection;
    };


    //! Helper class to try lock critical section
    class CriticalSectionTry
    {
    public:
        CriticalSectionTry(CriticalSection& criticalSection) :
            mCriticalSection(criticalSection),
            mLocked(false)
        {
            mLocked = mCriticalSection.Try();
        }

        ~CriticalSectionTry()
        {
            if (mLocked)
            {
                mCriticalSection.Unlock();
            }
        }

        bool IsLocked() const
        {
            return mLocked;
        }

    private:
        CriticalSection& mCriticalSection;
        bool mLocked;
    };


} // namespace eg

#endif 0

#endif // SYNCHRONIZATION_CRITICAL_SECTION_H


///////////////////////////////////////////////////////////////////////
// EventObject.h
//
//  Copyright 5/9/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Event object model on win32
//
//
//  #include "Synchronization/EventObject.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef SYNCHRONIZATION_EVENT_OBJECT_H
#define SYNCHRONIZATION_EVENT_OBJECT_H
#pragma once

#if 0

#include <Base.h>


namespace eg
{
    class EOData;

    /*!
    Simple replication of critical section based on Win32
    */
    class EventObject
    {
        //@{
        //! no copy semantics
        EventObject(const EventObject&);
        EventObject& operator=(const EventObject&);
        //@}

    public:
        EventObject(bool bManualReset, bool bInitialState);
        ~EventObject();

        void Signal();
        void Reset();
        void Pulse();

        //! OS specific handle
        void *GetHandle() const;

    private:
        EOData *mEOData;
    };

} // namespace eg

#endif 0

#endif // SYNCHRONIZATION_EVENT_OBJECT_H


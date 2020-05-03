///////////////////////////////////////////////////////////////////////
// WaitObject.h
//
//  Copyright 5/9/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//
//
//  #include <Synchronization/WaitObject.h>
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef SYNCHRONIZATION_WAIT_OBJECT_H
#define SYNCHRONIZATION_WAIT_OBJECT_H

#include "EventObject.h"


namespace eg
{

    class WaitObject
    {
        const static int32_t kWaitInfinite = -1;

        //@{
        //! no copy semantics
        WaitObject(const WaitObject&);
        WaitObject& operator=(const WaitObject&);
        //@}

    public:
		WaitObject();
        void Wait(EventObject& eventObject, int32_t waitMiliseconds = kWaitInfinite);
    };


} // namespace eg

#endif // SYNCHRONIZATION_WAIT_OBJECT_H



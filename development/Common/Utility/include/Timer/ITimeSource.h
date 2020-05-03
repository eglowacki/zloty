///////////////////////////////////////////////////////////////////////
// ITimeSource.h
//
//  Copyright 12/27/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Interface to our time source which is platform and os specific
//
//
//  #include "Timer/ITimeSource.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef TIMER_ITIMESOURCE_H
#define TIMER_ITIMESOURCE_H
#pragma once


#include "Base.h"


namespace eg
{

    class ITimeSource
    {
    public:
        virtual ~ITimeSource() {};
        virtual double GetTime() const = 0;
    };


} // namespace eg

#endif // TIMER_ITIMESOURCE_H

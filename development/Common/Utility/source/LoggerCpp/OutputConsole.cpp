/**
 * @file    OutputConsole.cpp
 * @ingroup LoggerCpp
 * @brief   Output to the standard console using fprintf() with stdout
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/OutputConsole.h"
#include "Base.h"
#include <cstdio>
#include "Platform/WindowsLean.h"

using namespace yaget;
using namespace yaget::ylog;

// Constructor
OutputConsole::OutputConsole(const Config::Ptr& /*aConfigPtr*/)
    : mConHandle(GetStdHandle(STD_OUTPUT_HANDLE))
{ }

// Destructor
OutputConsole::~OutputConsole()
{ }

// Convert a Level to a Win32 console color text attribute
unsigned short OutputConsole::toWin32Attribute(Log::Level aLevel)
{
    unsigned short code;

    switch (aLevel)
    {
        case Log::Level::eDebug: code = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break; // white
        case Log::Level::eInfo: code = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        case Log::Level::eNotice: code = FOREGROUND_GREEN;
            break; // green
        case Log::Level::eWarning: code = FOREGROUND_RED | FOREGROUND_GREEN;
            break; // orange
        case Log::Level::eError: code = FOREGROUND_RED;
            break; // red
        case Log::Level::eCritic: code = FOREGROUND_RED | FOREGROUND_INTENSITY;
            break; // light red
        default: code = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            break; // white
    }

    return (code);
}

// Output the Log to the standard console using fprintf
void OutputConsole::OnOutput(const Channel::Ptr& /*aChannelPtr*/, const Log& aLog) const
{
    const auto& buffer = aLog.FormatedMessage(false);

    unsigned short backgroundAttrib = 0x0;
    unsigned short forgroundAttrib = 0xF;
    CONSOLE_SCREEN_BUFFER_INFO bInfo = {};
    if (GetConsoleScreenBufferInfo(mConHandle, &bInfo) == TRUE)
    {
        backgroundAttrib = bInfo.wAttributes & 0x00F0;
        forgroundAttrib  = bInfo.wAttributes & 0x000F;
    }

    // uses fprintf for atomic thread-safe operation
    SetConsoleTextAttribute(mConHandle, toWin32Attribute(aLog.getSeverity()) | backgroundAttrib);
    fprintf(stdout, buffer.c_str());
    SetConsoleTextAttribute(mConHandle, forgroundAttrib | backgroundAttrib);
    fflush(stdout);
}

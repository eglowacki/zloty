///////////////////////////////////////////////////////////////////////
// AppTraits.h
//
//  Copyright 11/30/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper file only to use in applications or users of this library.
//
//
//  #include "App/AppTraits.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef APP_APP_TRAITS_H
#define APP_APP_TRAITS_H
#pragma once

#if 0
#include "Registrate.h"
#include "Config/ConfigHelper.h"
#include <wx/apptrait.h>
#include <wx/filename.h>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>

namespace eg
{



    /*!
    This can be returned from wxApp:CreateTraits() if it's overwritten in user derived class.
    This will read config file and create requested logging channels.

    */
    class AppTraits : public wxGUIAppTraits
    {
    public:
        AppTraits()
        : wxGUIAppTraits()
        , mpLastActiveLog(0)
        {
        }

        virtual  ~AppTraits()
        {
        }

        virtual wxLog *CreateLogTarget()
        {
            mpLastActiveLog = 0;
            bool bHasFileLog = true;
            bool bHasFrameLog = false;
            bool bHasGuiLog = false;
            bool bHasConsoleLog = false;
            bool bHasMemoryLog = false;
            std::vector<std::string> outputs = config::ReadStringArray("Logs/Output/Output");
            for (std::vector<std::string>::const_iterator it = outputs.begin(); it != outputs.end(); ++it)
            {
                if (boost::iequals(*it, std::string("File")))
                {
                    bHasFileLog = true;
                }
                else if (boost::iequals(*it, std::string("Frame")))
                {
                    bHasFrameLog = true;
                }
                else if (boost::iequals(*it, std::string("Gui")))
                {
                    bHasGuiLog = true;
                }
                else if (boost::iequals(*it, std::string("Console")))
                {
                    bHasConsoleLog = true;
                }
                else if (boost::iequals(*it, std::string("Memory")))
                {
                    bHasMemoryLog = true;
                }
            }

            int32_t logLevel = config::ReadInt("Logs/Output/Level", wxLOG_Max);

            if (!msFileStream && bHasFileLog)
            {
                wxFileName fileName(registrate::GetExecutablePath(), registrate::GetAppName(), "log");
                std::string logFileName = fileName.GetFullPath();
                msFileStream.reset(new std::ofstream(logFileName.c_str()));
            }

            wxLog *pLog = 0;
            if (bHasFileLog)
            {
                wxLogStream *pLogFile = new wxLogStream(msFileStream.get());
                registrate::RegisterObject(pLogFile, "Log.File");
                pLog = pLogFile;
            }

            if (bHasFrameLog)
            {
                if (pLog)
                {
                    wxLog::SetActiveTarget(pLog);
                }

                wxLogWindow *pLogWindow = new wxLogWindow(NULL, std::string(registrate::GetAppName() + " Log").c_str());
                registrate::RegisterObject(pLogWindow, "Log.Frame");
                pLog = pLogWindow;
            }

            if (bHasGuiLog)
            {
                wxLogGui *pLogGui = 0;
                if (bHasFrameLog)
                {
                    pLogGui = new wxLogGui;
                    pLog = new wxLogChain(pLogGui);
                }
                else if (bHasFileLog)
                {
                    wxLog::SetActiveTarget(pLog);
                    pLogGui = new wxLogGui;
                    pLog = new wxLogChain(new wxLogGui);
                }
                else
                {
                    pLogGui = new wxLogGui;
                    pLog = pLogGui;
                }

                registrate::RegisterObject(pLogGui, "Log.Gui");
            }

            if (bHasConsoleLog)
            {
                if (pLog)
                {
                    wxLog::SetActiveTarget(pLog);
                }

                wxLogStderr *pLogStdErr = new wxLogStderr;
                pLog = new wxLogChain(pLogStdErr);
                registrate::RegisterObject(pLogStdErr, "Log.Console");
            }

            if (bHasMemoryLog)
            {
                if (pLog)
                {
                    wxLog::SetActiveTarget(pLog);
                }

                wxLogBuffer *pLogBuffer = new wxLogBuffer;
                pLog = new wxLogChain(pLogBuffer);
                registrate::RegisterObject(pLogBuffer, "Log.Memory");
            }

            wxLog::SetLogLevel(logLevel);
            mpLastActiveLog = pLog;
            return pLog;
        }

    private:
        static boost::shared_ptr<std::ofstream> msFileStream;
        wxLog *mpLastActiveLog;

        //std::map<std::string, wxLog *> mLogs;
    };


} // namespace eg

#endif // 0
#endif // APP_APP_TRAITS_H


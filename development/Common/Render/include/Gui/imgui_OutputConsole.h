#pragma once
/////////////////////////////////////////////////////////////////////////
// imgui_OutputConsole.h
//
//  Copyright 3/5/2017 Edgar Glowacki.
//
// NOTES:
//      Wrapper for vertex and pixel shader
//
//
// #include "imgui/imgui_OutputConsole.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file

#pragma once
#ifndef IMGUI_OUTPUT_CONSOLE_H
#define IMGUI_OUTPUT_CONSOLE_H

#include "LoggerCpp/Output.h"
#include "LoggerCpp/Config.h"
#include "RenderMathFacade.h"

namespace yaget
{
    namespace imgui
    {
        class OutputConsole : public ylog::Output
        {
        public:
            explicit OutputConsole(const ylog::Config::Ptr& aConfigPtr);
            virtual ~OutputConsole();

            /**
             * @brief Convert a Level to a math3d::Color text attribute
             *
             * @param[in] aLevel Log severity Level to convert
             *
             * @return math3d::Color color text attribute
             */
            static math3d::Color toWin32Attribute(ylog::Log::Level aLevel);

        private:
            virtual void OnOutput(const ylog::Channel::Ptr& aChannelPtr, const ylog::Log& aLog) const;
        };

    } // namespace imgui
} // namespace yaget

#endif // IMGUI_OUTPUT_CONSOLE_H

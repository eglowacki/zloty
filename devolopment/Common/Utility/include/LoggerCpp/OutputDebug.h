/**
 * @file    OutputDebug.h
 * @ingroup LoggerCpp
 * @brief   Output to the Visual Studio debugger using OutputDebugString()
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include "LoggerCpp/Output.h"
#include "LoggerCpp/Config.h"

namespace yaget
{
    namespace ylog
    {
        /**
         * @brief   Output to the Visual Studio debugger using OutputDebugString()
         * @ingroup LoggerCpp
         */
        class OutputDebug : public Output
        {
        public:
            explicit OutputDebug(const Config::Ptr& aConfigPtr);

        private:
            virtual void OnOutput(const Channel::Ptr& aChannelPtr, const Log& aLog) const;
            bool m_bSplitLines = false;
        };

    } // namespace ylog
} // namespace yaget

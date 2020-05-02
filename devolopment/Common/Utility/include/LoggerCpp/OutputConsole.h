/**
 * @file    OutputConsole.h
 * @ingroup LoggerCpp
 * @brief   Output to the standard console using fprintf() with stdout
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
         * @brief   Output to the standard console using fprintf() with stdout
         * @ingroup LoggerCpp
         */
        class OutputConsole : public Output
        {
        public:
            /// @brief Constructor : no config
            explicit OutputConsole(const Config::Ptr& aConfigPtr);

            /// @brief Destructor
            virtual ~OutputConsole();

            /**
             * @brief Convert a Level to a Win32 console color text attribute
             *
             * @param[in] aLevel Log severity Level to convert
             *
             * @return Win32 console color text attribute
             */
            static unsigned short toWin32Attribute(Log::Level aLevel);

        private:
            virtual void OnOutput(const Channel::Ptr& aChannelPtr, const Log& aLog) const;

            using ConHandle = void*;
            ConHandle mConHandle = nullptr;
        };

    } // namespace ylog
} // namespace yaget

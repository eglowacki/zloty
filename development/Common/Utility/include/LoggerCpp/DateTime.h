/**
 * @file    DateTime.h
 * @ingroup LoggerCpp
 * @brief   Current time precise to the millisecond.
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <string>

namespace yaget
{
    namespace ylog
    {
        /**
         * @brief   Current time precise to the millisecond.
         * @ingroup LoggerCpp
         *
         * Using a struct to enable easy direct access to public members.
         *
         * Under Windows, the time is given to the millisecond.
         * Under Linux, the time is given to the microsecond.
         */
        struct DateTime
        {
            /**
             * @brief Constructor
             */
            DateTime();

            /**
             * @brief Set to current time
             */
            void Make();

            std::string ToString() const;

            int year; ///< year    [0,30827]
            int month; ///< month   [1,12]
            int day; ///< day     [1,31]
            int hour; ///< hour    [0,23]
            int minute; ///< minute  [0,59]
            int second; ///< second  [0,59]
            int ms; ///< millisecond
            int us; ///< microsecond (not under Windows)
        };

    } // namespace ylog
} // namespace yaget

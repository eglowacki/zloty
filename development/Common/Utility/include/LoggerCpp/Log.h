/**
 * @file    Log.h
 * @ingroup LoggerCpp
 * @brief   A RAII (private) log object constructed by the Logger class
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include "LoggerCpp/DateTime.h"
#include "LoggerCpp/Utils.h"

#include <sstream>
#include <iomanip>  // For easy use of parametric manipulators (setfill, setprecision) by client code
#include <memory>

namespace yaget
{
    namespace ylog
    {
        // forward declaration
        class Logger;

        /**
         * @brief   A RAII (private) log object constructed by the Logger class
         * @ingroup LoggerCpp
         *
         * a Log represents a full line of log, at a certain Log::Level of severity.
         *
         * It is constructed and initialized by a call to Logger::debug(),
         * Logger::info(), ... or Logger::critic().
         * Is is then used by successive stream call "<<", and is naturally terminated
         * by it destructor at the end of the line, calling the Logger::output() method.
         *
         * It contains all required information for further formating, printing and transmitting
         * by the Logger class.
         */
        class Log
        {
            friend class Logger;

        public:
            /**
             * @brief Enumeration of the severity levels
             */
            enum class Level
            {
                eDebug = 0,
                eInfo,
                eNotice,
                eWarning,
                eError,
                eCritic
            };

            /**
             * @brief stream inserter operator
             *
             * @param[in] aValue    Value to be formatted and inserted into the Log string stream
             *
             * @return Currents Log instance
             */
            template <typename T>
            Log& operator<<(const T& aValue)
            {
                mpStream << aValue;
                return (*this);
            }

            /**
             * @brief Destructor : output the Log string stream
             */
            ~Log();

            /// @brief Severity Level of this Log
            inline Level getSeverity() const
            {
                return mSeverity;
            }

            /// @brief Timestamp of this Log
            inline const DateTime& getTime() const
            {
                return mTime;
            }

            /// @brief The underlying string stream
            inline const std::ostringstream& getStream() const
            {
                return mpStream;
            }

            void Write(const char* file, unsigned line, const char* functionName, uint32_t tag, bool bValid, const char* format, ...);

            inline const std::string& GetFileName() const
            {
                return mFileName;
            }

            inline int GetFileLine() const
            {
                return mFileLine;
            }

            inline const std::string& GetFunctionName() const
            {
                return mFunctionName;
            }

            inline uint32_t GetTag() const
            {
                return mTag;
            }

            inline const std::string& FormatedMessage(bool split) const
            {
                return split ? mFormatedLineMessageSplit : mFormatedLineMessage;
            }

            /**
             * @brief Convert a Level to its string representation
             *
             * @param[in] aLevel Log severity Level to convert
             *
             * @return Severity Level description
             */
            static const char* toString(Log::Level aLevel);

            /**
             * @brief Convert a string representation of a Level to its corresponding value
             *
             * @param[in] apLevel Log severity string Level
             *
             * @return Severity Level value
             */
            static Log::Level toLevel(const char* apLevel);

        private:
            Log(const Log&);
            Log& operator=(const Log&);

            /**
             * @brief Construct a RAII (private) log object for the Logger class
             *
             * @param[in] aLogger   Reference to the parent Logger
             * @param[in] aSeverity Severity of this Log
             */
            Log(const Logger& aLogger, Level aSeverity);

            void FormatLineMessage();

            const Logger& mLogger; ///< Reference to the parent Logger
            Level mSeverity; ///< Severity of this Log
            DateTime mTime; ///< Timestamp of the output
            //std::shared_ptr<std::ostringstream> mpStream; ///< The underlying string stream
            std::ostringstream mpStream; ///< The underlying string stream
            std::string mFileName;
            int mFileLine = 0;
            uint32_t mTag = 0;
            bool mIsFiltered = false;
            std::string mFunctionName;

            std::string mFormatedLineMessage;
            std::string mFormatedLineMessageSplit;
        };

    } // namespace ylog
} // namespace yaget

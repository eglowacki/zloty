/**
 * @file    OutputFile.h
 * @ingroup LoggerCpp
 * @brief   Output to the a file using fprintf
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include "LoggerCpp/Output.h"
#include "LoggerCpp/Config.h"

#include <string>

namespace yaget
{
    namespace ylog
    {
        /**
         * @brief   Output to the standard console using fprintf
         * @ingroup LoggerCpp
         */
        class OutputFile : public Output
        {
        public:
            /**
             * @brief Constructor : open the output file
             *
             * @param[in] aConfigPtr    Config the output file with "filename"
             */
            explicit OutputFile(const Config::Ptr& aConfigPtr);

            /// @brief Destructor : close the file
            virtual ~OutputFile();

        private:
            virtual void OnOutput(const Channel::Ptr& aChannelPtr, const Log& aLog) const;

            /// @brief Open the log file
            void open() const;
            /// @brief Close the log file
            void close() const;

            mutable FILE* mpFile; ///< @brief File pointer (mutable to be modified in the const output method)

            /**
             * @brief "filename" : Name of the log file
             */
            std::string mFilename;

            /**
             * @brief "filename_old" : Name of the log file renamed after max_size is reach
             */
            //std::string mFilenameOld;

            bool m_bSplitLines = false;

            static int mInstanceCounter;
        };

    } //  // namespace ylog
} // namespace yaget

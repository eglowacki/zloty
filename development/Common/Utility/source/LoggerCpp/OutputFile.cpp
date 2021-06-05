/**
 * @file    OutputFile.cpp
 * @ingroup LoggerCpp
 * @brief   Output to the standard console using printf
 *
 * Copyright (c) 2013 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */

#include "LoggerCpp/OutputFile.h"
#include "LoggerCpp/Exception.h"
#include "App/AppUtilities.h"
#include "App/FileUtilities.h"
#include "StringHelpers.h"

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>

#include <filesystem>
namespace fs = std::filesystem;

using namespace yaget;
using namespace yaget::ylog;


int OutputFile::OutputFile::mInstanceCounter = 0;

// Open the output file
OutputFile::OutputFile(const Config::Ptr& aConfigPtr)
: mpFile(nullptr)
, m_bSplitLines(conv::Convertor<bool>::FromString(aConfigPtr->get("split_lines", "false")))
{
    assert(aConfigPtr);

    const fs::path logFileName(aConfigPtr->get("filename", "$(LogFolder)/$(AppName).log"));

    // used to re-use the same log file if this is the same instance but user re-configured log system
    static Guid runtimeId;
    if (runtimeId != util::ApplicationRuntimeId())
    {
        runtimeId = util::ApplicationRuntimeId();

        util::FileCycler(logFileName.generic_string());
    }

    const std::string logPathName = fs::path(util::ExpendEnv(logFileName.generic_string(), nullptr)).generic_string();

    io::file::AssureDirectories(logPathName);
    mFilename = logPathName;

    ++mInstanceCounter;
    open();
}

// Close the file
OutputFile::~OutputFile()
{
    close();
}

#pragma warning(push)
#pragma warning(disable : 4996)   // warning C4996: 'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS.
// Open the file
void OutputFile::open() const
{
    mpFile = fopen(mFilename.c_str(), "at");
    if (!mpFile)
    {
        LOGGER_THROW("file '" << mFilename << "' not opened");
    }
    else if (mInstanceCounter > 1)
    {
        fprintf(mpFile, "Instance Run No: '%d' ---------------------------------------------------------\n", mInstanceCounter);
        fflush(mpFile);
    }
}
#pragma warning(pop) 

// Close the file if it is opened
void OutputFile::close() const
{
    if (mpFile)
    {
        fclose(mpFile);
        mpFile = nullptr;
    }
}

// Output the Log to the standard console using printf
void OutputFile::OnOutput(const Channel::Ptr& /*aChannelPtr*/, const Log& aLog) const
{
    if (mpFile)
    {
        const auto& buffer = aLog.FormatedMessage(m_bSplitLines);
        fprintf(mpFile, buffer);
        fflush(mpFile);
    }
}

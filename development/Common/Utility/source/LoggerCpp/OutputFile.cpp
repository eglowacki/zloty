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



using namespace yaget;
using namespace yaget::ylog;
namespace fs = std::filesystem;

// Open the output file
OutputFile::OutputFile(const Config::Ptr& aConfigPtr)
: mpFile(nullptr)
, m_bSplitLines(conv::Convertor<bool>::FromString(aConfigPtr->get("split_lines", "false")))
{
    assert(aConfigPtr);

    std::string logPathName = util::ExpendEnv(aConfigPtr->get("filename", "$(LogFolder)/$(AppName).txt"), nullptr);
    io::file::AssureDirectories(logPathName);
    logPathName = fs::weakly_canonical(logPathName).string();

    std::string ext = fs::path(logPathName).extension().string();

    fs::path p(logPathName);
    p.replace_extension(".old" + ext);
    std::string logOldPathName = p.string();

    mMaxStartupSize = aConfigPtr->get("max_startup_size", static_cast<long>(0));
    mMaxSize = aConfigPtr->get("max_size", static_cast<long>(1024) * 1024);
    mFilename = logPathName;
    mFilenameOld = logOldPathName;

    // Test the size of the existing log file, rename it and open a new one if needed
    struct stat statFile;
    int ret = stat(mFilename.c_str(), &statFile);
    if (0 == ret)
    {
        mSize = statFile.st_size;
    }

    if (mSize > mMaxStartupSize)
    {
        rotate();
    }
    else
    {
        open();
    }
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
        LOGGER_THROW("file \"" << mFilename << "\" not opened");
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
        mSize = 0;
    }
}

// Rotate a file : close, remove, rename, open
void OutputFile::rotate() const
{
    close();

    remove(mFilenameOld.c_str());
    rename(mFilename.c_str(), mFilenameOld.c_str());

    open();
}

// Output the Log to the standard console using printf
void OutputFile::OnOutput(const Channel::Ptr& /*aChannelPtr*/, const Log& aLog) const
{
    if (mSize > mMaxSize)
    {
        rotate();
    }

    if (mpFile)
    {
        const auto& buffer = aLog.FormatedMessage(m_bSplitLines);
        int nbWritten = fprintf(mpFile, buffer);
        fflush(mpFile);
        mSize += nbWritten;
    }
}

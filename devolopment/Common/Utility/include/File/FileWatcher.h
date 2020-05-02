///////////////////////////////////////////////////////////////////////
// FileWatcher.h
//
//  Copyright 12/28/2005 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      This provides objects to watch files and directories for
//      any changes
//
//
//  #include "File/FileWatcher.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_FILE_WATCHER_H
#define FILE_FILE_WATCHER_H
#pragma once

#include "Base.h"
#if 0
    #include <wx/defs.h>
    #include <wx/string.h>
    #include <wx/filename.h>
    #include <wx/thread.h>
    #include <vector>
    #include <map>
    #include <boost/signal.hpp>

namespace eg
{

    /*!
    This worker thread (WT) will listen for changes to specific files on the disk (platform depenendt)
    and trigger sigFileEvent.
    */
    class FileWatcher : public wxThread
    {
        // no copy semantics
        FileWatcher(const FileWatcher&);
        FileWatcher& operator==(const FileWatcher&);

    public:
        FileWatcher(const std::string& directoryPath);
        virtual ~FileWatcher();

        //! This allows to add which file we are interested in
        void AddFile(const std::string& fileName);
        //! Remove file from watch
        void RemoveFile(const std::string& fileName);

        //! Start watching directory for changes
        void Start();
        //! Stop watching directory for changes
        void Stop();

        //! Triggered for any file watched, when it get's modified.
        //! (FileName, DirectoryPath)
        boost::signal<void (const std::string&, const std::string&)> sigFileEvent;

    protected:
        // from wxThread
        virtual void *Entry();

    private:
        //! Returns collections for changed files
        //! \param modifiedFiles all files which were modified.
        //! \return true then there are more files to check, otherwise false and  nothing left since all files are processed.
        bool getModifiedFilesList(std::vector<std::string>& modifiedFiles);

        //! Directory path to watch for file
        std::string mDirectoryPath;

        //! this is used to keep when the file was modified last
        struct FileInfo
        {
            //! Specifies state of quried file
            //! sSame - did not change since last call
            //! sModified - changed since last call
            //! sLocked - locked and can not be red, try later
            enum State
            {
                sSame, sModified, sLocked
            };

            FileInfo(const std::string& fullFilePath);

            //! This will return TRUE if file was modified since last call to this method
            //! It will also update mLastModifiedTime
            State GetModifiedState();

            // entrire path and file name
            wxFileName mFullPathFileName;
            // last date when this file was modified
            wxDateTime mLastModifiedTime;
        };

        /*!
        Map of all watchable files,
        first  - file name only
        second - file info structure to keep last modified time and full path of the said file
        */
        typedef std::map<std::string, FileInfo> FileInfoList_t;
        typedef FileInfoList_t::iterator it_FIL;
        typedef FileInfoList_t::const_iterator cit_FIL;

        FileInfoList_t mFileInfoList;

        //! Used to lock the list when we add, remove and read the file list
        wxMutex mFileListMutex;

        //! This is used to notify thread that we are quiting
        typedef void *HANDLE;
        HANDLE mQuitHandle;
    };


} // namespace eg
#endif // 0

#endif // FILE_FILE_WATCHER_H


#include "File/FileWatcher.h"
#if 0
    #include <wx/log.h>
    #include <wx/utils.h>
    #include <algorithm>
    #define WIN32_LEAN_AND_MEAN
    #define WIN32_EXTRA_LEAN
    #include <windows.h>


namespace eg
{


    FileWatcher::FileWatcher(const std::string& directoryPath)
    : wxThread(wxTHREAD_JOINABLE)
    , mQuitHandle(0)
    , mDirectoryPath(directoryPath)
    {
    }


    FileWatcher::~FileWatcher()
    {
        Stop();
        Wait();
    }


    void FileWatcher::Start()
    {
        mQuitHandle = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        Create();
        Run();
    }


    void FileWatcher::Stop()
    {
        if (mQuitHandle)
        {
            ::SetEvent(mQuitHandle);
        }
    }


    void FileWatcher::AddFile(const std::string& fileName)
    {
        wxMutexLocker lock(mFileListMutex);
        it_FIL it = mFileInfoList.find(fileName);
        if (it == mFileInfoList.end())
        {
            // only add file name if there it's not already added
            std::string fullPath = mDirectoryPath + "\\" + fileName;
            mFileInfoList.insert(std::make_pair(fileName, FileInfo(fullPath)));
        }
    }


    void FileWatcher::RemoveFile(const std::string& fileName)
    {
        wxMutexLocker lock(mFileListMutex);
        mFileInfoList.erase(fileName);
    }


    void *FileWatcher::Entry()
    {
        HANDLE handles[2];
        handles[0] = ::FindFirstChangeNotificationA(mDirectoryPath.c_str(), TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
        if (handles[0] == INVALID_HANDLE_VALUE)
        {
            const wxChar *pErrorMsg = wxSysErrorMsg();
            wxLogError("Could not start watching directory '%s'. SysError: '%s'.", mDirectoryPath.c_str(), pErrorMsg);
            return 0;
        }

        handles[1] = mQuitHandle;

        while (true)
        {
            DWORD eventID = ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);

            if (eventID == WAIT_FAILED)
            {
                const wxChar *pErrorMsg = wxSysErrorMsg();
                wxLogError("Could not wait on watching directory '%s'. SysError: '%s'.", mDirectoryPath.c_str(), pErrorMsg);
                return 0;
            }
            else if (eventID == WAIT_OBJECT_0 + 0)
            {
                bool bMoreFiles = true;
                do
                {
                    std::vector<std::string> modifiedFiles;
                    bMoreFiles = getModifiedFilesList(modifiedFiles);
                    for (std::vector<std::string>::const_iterator it = modifiedFiles.begin(); it != modifiedFiles.end(); ++it)
                    {
                        sigFileEvent(*it, mDirectoryPath);
                    }

                    if (bMoreFiles)
                    {
                        // if we have more files to check, give some small time before we check it again
                        ::wxMilliSleep(10);
                    }
                } while (bMoreFiles);

            }
            else if (eventID == WAIT_OBJECT_0 + 1)
            {
                // quit the thread
                ::FindCloseChangeNotification(handles[0]);
                break;
            }

            if (!::FindNextChangeNotification(handles[0]))
            {
                const wxChar *pErrorMsg = wxSysErrorMsg();
                wxLogError("Could not find next ChangeNotification on directory '%s'. SysError: '%s'.", mDirectoryPath.c_str(), pErrorMsg);
                return 0;
            }
        }

        return 0;
    }


    bool FileWatcher::getModifiedFilesList(std::vector<std::string>& modifiedFiles)
    {
        bool bMoreFiles = false;
        wxMutexLocker lock(mFileListMutex);
        for (it_FIL it = mFileInfoList.begin(); it != mFileInfoList.end(); ++it)
        {
            FileInfo::State state = (*it).second.GetModifiedState();
            if (state == FileInfo::sModified)
            {
                modifiedFiles.push_back((*it).first);
            }
            else if (!bMoreFiles && state == FileInfo::sLocked)
            {
                bMoreFiles = true;
            }
        }

        return bMoreFiles;
    }


    FileWatcher::FileInfo::FileInfo(const std::string& fullFilePath) :
    mFullPathFileName(fullFilePath)
    {
        mLastModifiedTime = mFullPathFileName.GetModificationTime();
    }


    FileWatcher::FileInfo::State FileWatcher::FileInfo::GetModifiedState()
    {
        State state = sSame;

        if (mFullPathFileName.IsFileReadable())
        {
            wxDateTime currModifiedTime = mFullPathFileName.GetModificationTime();
            if (currModifiedTime.IsValid())
            {
                if (currModifiedTime.IsLaterThan(mLastModifiedTime))
                {
                    // this file was modified since the last call to this method
                    mLastModifiedTime = currModifiedTime;
                    state = sModified;
                }
            }
            else
            {
                state = sLocked;
            }
        }
        else
        {
            state = sLocked;
        }

        return state;
    }


} // namespace eg
#endif // 0


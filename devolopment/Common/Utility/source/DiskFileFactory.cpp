#include "Logger/Log.h"
#include "File/DiskFileFactory.h"
#include "Config/ConfigHelper.h"
#include "StringHelper.h"
#include "Message/Dispatcher.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>


namespace bfs = boost::filesystem;

namespace
{
    void get_all_files(const bfs::path& dir_path, std::vector<std::string>& file_names)
    {
        if (!bfs::exists(dir_path) || !bfs::is_directory(dir_path))
        {
            return;
        }

        bfs::directory_iterator it(dir_path), end_iter;

        for (; it!= end_iter; ++it)
        {
            if (bfs::is_directory(*it))
            {
                get_all_files(*it, file_names);
            }
            else if (bfs::is_regular_file(*it))
            {
                file_names.push_back((*it).generic_string());
            }
        }
    }


    /*
    namespace bfs = boost::filesystem;

    // Search for a file with the name 'filename' starting in directory 'dir_path',
    // copy the path of the file in 'pfound' if found, and return true.
    // Else return false.
    bool find_file(const bfs::path& dir_path, const std::string& file_name, bfs::path& pfound)
    {
        if (!exists(dir_path) || !is_directory(dir_path))
            return false;

        bfs::directory_iterator it(dir_path), end_iter;

        for (; it!= end_iter; ++it)
        {
            if (bfs::is_directory(*it))
            {
                if (find_file(*it, file_name, pfound))
                {
                    return true;
                }
            }

            else if ((*it).leaf() == file_name)
            {
                pfound = *iter;
                return true;
            }
        }

        return false;
    }
    */

} // namespace




namespace eg {

// to provide logging output for folders
std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& folders)
{
    BOOST_FOREACH(const std::string& name, folders)
    {
        out << "\n\t'" << name << "'";
    }
    return out;
}

DiskFileFactory::DiskFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& folders)
: mType(type)
, mPrority(prority)
, mFolderList(folders)
{
    if (mFolderList.empty())
    {
        log_warning << "There is no file path in DiskFile factory, type: " << type << " with prority: " << prority;
    }

    // \todo set up watch over this folders

    log_trace(tr_util) << "DiskFileFactory created, type: '" << mType << "', prority: " << logs::hex<uint32_t>(mPrority) << "\nfolders: " << mFolderList;
}


DiskFileFactory::~DiskFileFactory()
{
    log_trace(tr_util) << "DiskFile factory object deleted.";
}


void DiskFileFactory::onFileChanged(const std::string& name, const std::string& /*path*/)
{
    // \todo most likely we need to unmagle/mangle this name to fix user expected pattern
    StreamChangedEvent(name, *this);
}


VirtualFileFactory::istream_t DiskFileFactory::GetFileStream(const std::string& name) const
{
    std::string fullPath = findFullFilePath(name);
    if (!fullPath.empty())
    {
        istream_t stream(new std::fstream(fullPath.c_str(), std::ifstream::in|std::ifstream::binary));
        if (static_cast<const std::fstream *>(stream.get())->is_open())
        {
            return stream;
        }

        log_error << "File '" << fullPath << "' could not be opened for reading.";
    }

    return istream_t();
}


std::string DiskFileFactory::findFullFilePath(const std::string& name) const
{
    BOOST_FOREACH(const std::string& folder, mFolderList)
    {
        bfs::path filePath(folder);
        filePath /= name;
        if (bfs::exists(filePath))
        {
            return filePath.generic_string();
        }
    }

    return "";
}


VirtualFileFactory::ostream_t DiskFileFactory::AttachFileStream(const std::string& name)
{
    std::string fullPath = findFullFilePath(name);
    if (fullPath.empty())
    {
        if (!mFolderList.empty())
        {
            std::string folder = *mFolderList.begin();
            bfs::path filePath(folder);
            filePath /= name;

            bfs::path dirPath(filePath);
            dirPath.remove_filename();
            try
            {
                bfs::create_directories(dirPath);
                fullPath = filePath.string();
            }
            catch (const bfs::filesystem_error& e)
            {
                log_error << "Could not create directory '" << dirPath.generic_string() << "'. Error: " << e.what();
                return ostream_t();
            }
        }
    }

    ostream_t stream(new std::ofstream(fullPath.c_str(), std::ofstream::out|std::ofstream::trunc|std::ofstream::binary));
    if (static_cast<const std::ofstream *>(stream.get())->is_open())
    {
        return stream;
    }

    log_error << "Could not open file '" << fullPath << "'.";
    return ostream_t();
}


bool DiskFileFactory::IsFileStreamExist(const std::string& name) const
{
    return findFullFilePath(name) != "";
}


bool DiskFileFactory::CanAttachFileStream(const std::string& /*name*/) const
{
    // this should be data driven, so we can disable in production
    return true;
}


    /*
bool DiskFileFactory::WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged)
{
    std::string path = findFullFilePath(name);//mPathList.FindAbsoluteValidPath(name.c_str());
    if (!path.empty())
    {
        // strip path of name
        path.erase(path.size() - (name.size()+1));
        typedef std::map<std::string, boost::shared_ptr<FileWatcher> > FileWatchers_t;
        FileWatchers_t::const_iterator it = mFileWatchers.find(path);
        if (it != mFileWatchers.end())
        {
            (*it).second->sigFileEvent.connect(fileStreamChanged);
            (*it).second->AddFile(name);
            return true;
        }
    }

    log_error << "WatchFileStream on Disk is not implemented yet!!!";
    return false;
}


void DiskFileFactory::UnwatchFileStream(const std::string& name)
{
    std::string path = findFullFilePath(name);//mPathList.FindAbsoluteValidPath(name.c_str());
    if (!path.empty())
    {
        // strip path of name
        path.erase(path.size() - (name.size()+1));
        typedef std::map<std::string, boost::shared_ptr<FileWatcher> > FileWatchers_t;
        FileWatchers_t::const_iterator it = mFileWatchers.find(path);
        if (it != mFileWatchers.end())
        {
            // \note we also need to disconnect
            (*it).second->RemoveFile(name);
        }
    }
    log_error << "UnwatchFileStream on Disk is not implemented yet!!!";
}
*/


std::vector<std::string> DiskFileFactory::GetFileList(const std::string& filter) const
{
    typedef std::vector<std::string> FileList_t;
    FileList_t fileList;
    for (FileList_t::const_iterator it = mFolderList.begin(); it != mFolderList.end(); ++it)
    {
        FileList_t currentFileList;
        get_all_files(*it, currentFileList);

        std::string filePathQuery = *it + "/" + filter;
        for (FileList_t::const_iterator fit = currentFileList.begin(); fit != currentFileList.end(); ++fit)
        {
            std::string file = *fit;
            if (WildCompareISafe(filePathQuery, file))
            {
                // this file fits filter, so we need to strip common parts
                // and only be left with uniqe file name without any path
                file = file.erase(0, (*it).size() + 1);
                fileList.push_back(file);
            }
        }
    }

    return fileList;
}


std::string DiskFileFactory::GetFqn(const std::string& name) const
{
    std::string fullPath = findFullFilePath(name);
    return fullPath;
}

} // namespace eg

#include "File/VirtualFileSystem.h"
#include "Config/ConfigHelper.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>


namespace
{
    //! Regular expression for separating tag from file_name by '#' <tag_name>#<file_mame.ext>
    //const char *kTag_File = "^.+#";

    bool sortFactories(const eg::VirtualFileSystem::Factory_t& t1, const eg::VirtualFileSystem::Factory_t& t2)
    {
        return t1->Prority() < t2->Prority();
    }

} // namespace

namespace eg {


VirtualFileSystem::VirtualFileSystem()
{
    log_trace(tr_util) << "VirtualFileSystem object created.";
}


VirtualFileSystem::VirtualFileSystem(SettingCallback_t settings)
{
    settings(*this);
}


VirtualFileSystem::~VirtualFileSystem()
{
    log_trace(tr_util) << "VirtualFileSystem object deleted.";
}


VirtualFileFactory::istream_t VirtualFileSystem::GetFileStream(const std::string& name) const
{
    std::string key = GetKey(name);
    KeyFactories_t factories = findKeyFactories(key);

    std::string normalizedName = normalizeName(name);
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        VirtualFileFactory::istream_t inStream = (*it)->GetFileStream(normalizedName);
        if (inStream)
        {
            log_debug << "Requested file stream '" << normalizedName << "' for reading.";
            return inStream;
        }
    }

    log_debug << "Did not find file stream '" << normalizedName << "' for reading.";
    return VirtualFileFactory::istream_t();
}


VirtualFileFactory::ostream_t VirtualFileSystem::AttachFileStream(const std::string& name)
{
    std::string key = GetKey(name);
    KeyFactories_t factories = findKeyFactories(key);

    std::string normalizedName = normalizeName(name);
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        Factory_t factory = *it;
        VirtualFileFactory::ostream_t outStream = factory->AttachFileStream(normalizedName);
        if (outStream)
        {
            log_debug << "Attached to file stream '" << normalizedName << "' for writing.";
            return outStream;
        }
    }

    log_debug << "Did not find file stream '" << normalizeName(name) << "' for writing.";
    return VirtualFileFactory::ostream_t();
}


bool VirtualFileSystem::IsFileStreamExist(const std::string& name) const
{
    std::string key = GetKey(name);
    KeyFactories_t factories = findKeyFactories(key);

    std::string normalizedName = normalizeName(name);
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        if ((*it)->IsFileStreamExist(normalizedName))
        {
            return true;
        }
    }

    return false;
}


bool VirtualFileSystem::CanAttachFileStream(const std::string& name) const
{
    std::string key = GetKey(name);
    KeyFactories_t factories = findKeyFactories(key);

    std::string normalizedName = normalizeName(name);
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        if ((*it)->CanAttachFileStream(normalizedName.c_str()))
        {
            return true;
        }
    }

    return false;
}


void VirtualFileSystem::triggerFile(const std::string& name)
{
    StreamChangedEvent(name, *this);
}


void VirtualFileSystem::onStreamChanged(const std::string& name, VirtualFileFactory&)
{
    // we got event for stream data change (most likley file on disk was written to)
    StreamChangedEvent(name, *this);
}


void VirtualFileSystem::RegisterFactory(const std::string& key, Factory_t virtualFileFactory)
{
    assert(virtualFileFactory);
    boost::mutex::scoped_lock locker(mFileMutex);

    if (virtualFileFactory)
    {
        std::string validKey = boost::to_lower_copy(key);
        log_debug << "Registered File Factory '" << virtualFileFactory->Name() << "' for type '" << validKey << "'.";
        mRegisteredFactories[validKey].push_back(virtualFileFactory);
        // now sort it based on Prority level
        std::vector<Factory_t>& factories = mRegisteredFactories[validKey];
        std::sort(factories.begin(), factories.end(), sortFactories);

        virtualFileFactory->StreamChangedEvent.connect(boost::bind(&VirtualFileSystem::onStreamChanged, this, _1, _2));
    }
}


std::string VirtualFileSystem::GetCleanName(const std::string& name) const
{
    std::string key = GetKey(name);
    std::string cleaName = name;
    boost::ierase_last(cleaName, "." + key);
    return cleaName;
}


std::string VirtualFileSystem::GetKey(const std::string& name) const
{
    std::string validKey = name;
    size_t pos = name.find_first_of('#');
    if (pos != std::string::npos)
    {
      // we have tag marker
      validKey = name.substr(0, pos);
    }
    else
    {
        // there is no tag marker,
        // so extract extension
        pos = name.find_last_of('.');
        if (pos != std::string::npos)
        {
          // we have tag marker
          validKey = name.substr(pos+1);
        }
    }

    boost::to_lower(validKey);
    { // --- lock block
        boost::mutex::scoped_lock locker(mFileMutex);
        if (mRegisteredFactories.find(validKey) != mRegisteredFactories.end())
        {
            return validKey;
        }
    } // --- lock block

    log_debug << "Did not find valid key for: " << name;
    return "";
}


std::string VirtualFileSystem::GetFqn(const std::string& name) const
{
    std::string key = GetKey(name);
    KeyFactories_t factories = findKeyFactories(key);

    std::string normalizedName = normalizeName(name);
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        std::string fqn = (*it)->GetFqn(normalizedName);
        if (!fqn.empty())
        {
            return fqn;
        }
    }

    return name;
}


std::string VirtualFileSystem::normalizeName(const std::string& name) const
{
    size_t pos = name.find_first_of('#');
    if (pos != std::string::npos)
    {
      // we have tag marker
      std::string normName(name);
      normName.erase(0, pos);
      return normName;

    }
    return name;
}


std::vector<std::string> VirtualFileSystem::GetFileList(const std::string& filter) const
{
    std::string key = GetKey(filter);
    KeyFactories_t factories = findKeyFactories(key);

    std::string prefix = (filter == key) ? "*." : "";
    std::string normalizedName = prefix + normalizeName(filter);

    std::vector<std::string> fileList;
    for (KeyFactories_t::const_iterator it = factories.begin(); it != factories.end(); ++it)
    {
        std::vector<std::string> currFileList = (*it)->GetFileList(normalizedName);
        std::copy(currFileList.begin(), currFileList.end(), std::back_inserter(fileList));
    }

    return fileList;
}

VirtualFileSystem::FileTree_t VirtualFileSystem::GetFileTree(const std::string& /*filter*/) const
{
    FileTree_t fileTree;
    /*
    std::vector<std::string> types;
    if (filter.empty() || filter == "*" || filter == "*.*")
    {
        boost::mutex::scoped_lock locker(mFileMutex);
        for (Factories_t::const_iterator it = mRegisteredFactories.begin(); it != mRegisteredFactories.end(); ++it)
        {
            types.push_back("*." + (*it).first);
        }
    }
    else
    {
        types.push_back(filter);
    }

    for (std::vector<std::string>::const_iterator it = types.begin(); it != types.end(); ++it)
    {
        std::vector<std::string> fileList = GetFileList(*it);
        std::vector<std::string> fqns;
        std::set<std::string> folders;
        //! create all needed vectors of root folders and all directories and files under it
        for (std::vector<std::string>::const_iterator it_fl = fileList.begin(); it_fl != fileList.end(); ++it_fl)
        {
            // this is our full path name asset
            std::string fqn = GetFqn(*it_fl);
            fqns.push_back(fqn);

            //! just get the root folder for this asset
            std::string folder = fqn;
            boost::erase_tail(folder, (*it_fl).size());
            if (folders.find(folder) == folders.end())
            {
                folders.insert(folder);
            }
        }

        //! This will walk all fqns and fill tree container
        std::vector<std::string>::const_iterator it_fqn = fqns.begin();
        for (std::vector<std::string>::const_iterator it = fileList.begin(); it != fileList.end(); ++it, ++it_fqn)
        {
            // find root folder for this asset
            std::string folder =  boost::erase_last_copy(*it_fqn, *it);

            // look in assets if we have or not this folder created
            FileTree_t::iterator it_asset_folder = fileTree.find(folder);
            if (it_asset_folder == fileTree.end())
            {
                it_asset_folder = fileTree.insert(folder);
            }

            // we need to split this asset name into folders and file name
            // so we can construct all nodes to represnt this asset (directories and file)
            wxFileName assetPath(*it);
            const wxArrayString& assetDirs = assetPath.GetDirs();

            for (wxArrayString::const_iterator it_dir = assetDirs.begin(); it_dir != assetDirs.end(); ++it_dir)
            {
                std::string currDirName = *it_dir;
                FileTree_t::iterator it_dir_curr = (*it_asset_folder.node()).find(currDirName);
                if (it_dir_curr == (*it_asset_folder.node()).end())
                {
                    it_asset_folder = (*it_asset_folder.node()).insert(currDirName);
                }
                else
                {
                    it_asset_folder = it_dir_curr;
                }
            }

            // finaly add asset name to a last node
            std::string assetName = assetPath.GetFullName();
            (*it_asset_folder.node()).insert(assetName);
        }
    }
    */

    return fileTree;
}

VirtualFileSystem::KeyFactories_t VirtualFileSystem::findKeyFactories(const std::string& key) const
{
    boost::mutex::scoped_lock locker(mFileMutex);

    Factories_t::const_iterator it = mRegisteredFactories.find(key);
    if (it != mRegisteredFactories.end())
    {
        return (*it).second;
    }

    return KeyFactories_t();
}


// -------------------------------------------------------------------------
// Free functions BEGIN
// -------------------------------------------------------------------------
size_t GetIStreamSize(std::istream& istream)
{
    // checking to see if this always will be 0
    //if (!f.good() || f.eof() || !f.is_open()) { return 0; }

    size_t length = istream.tellg();
    if (length != (size_t)-1)
    {
        istream.seekg(0, std::ios::end);
        length = istream.tellg();
        istream.seekg(0, std::ios::beg);
        return length;
    }

    return 0;
}

// -------------------------------------------------------------------------
size_t GetOStreamSize(std::ostream& ostream)
{
    // checking to see if this always will be 0 here
    size_t length = ostream.tellp();
    return length = (length != (size_t)-1) ? length : 0;
}

void CopyStream(std::istream& istream, std::ostream& ostream)
{
    ostream << istream.rdbuf();
}

// -------------------------------------------------------------------------
// Free functions END
// -------------------------------------------------------------------------



} // namespace eg

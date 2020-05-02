#include "Watcher/AssetWatcher.h"
#include "File/VirtualFileSystem.h"
#include "Registrate.h"
#include <fstream>
#include <sstream>
#include <boost/bind.hpp>


namespace eg {

AssetWatcher::AssetWatcher()
//: mFileWatcher(registrate::GetExecutablePath() + std::string("/WatchTest/"))
{
    //mFileWatcher.sigFileEvent.connect(boost::bind(&AssetWatcher::OnFileChange, this, _1, _2));
    //mFileWatcher.Start();
}


AssetWatcher::~AssetWatcher()
{
}


void AssetWatcher::OnExit()
{
    //mFileWatcher.Stop();
    //if (mFileWatcher.IsRunning())
    //{
    //    mFileWatcher.Wait();
    //}
}

// called by file watcher object for every file registered with
void AssetWatcher::OnFileChange(const std::string& /*fileName*/, const std::string& /*key*/)
{
    //AddNewJob(asset::Tag(fileName, key));
}


// asset is ready to by updated, so let's trigger asset ready signal
void AssetWatcher::OnAssetUpdate(asset::Tag tag)
{
    if (tag.mData)
    {
        // only trigger event if there is any data associated with tag
        sigAssetReady(tag);
    }
}


bool AssetWatcher::OnAssetReady(asset::Tag& tag)
{
    //VirtualFileFactory::istream_t istreamH = VFS::Instance().GetFileStream(tag.mKey, tag.mName);

    // let's check file to see if we can open for reading
    std::ifstream assetFile(std::string(tag.mKey + tag.mName).c_str(), std::ifstream::in|std::ifstream::binary);
    if (assetFile.is_open())
    {
        std::stringstream *pData = new std::stringstream;
        *pData << assetFile;
        tag.mData.reset(pData);
        return true;
    }

    return false;
}


void AssetWatcher::AddFile(const std::string& /*fileName*/)
{
    //mFileWatcher.AddFile(fileName);
}


void AssetWatcher::RemoveFile(const std::string& /*fileName*/)
{
    //mFileWatcher.RemoveFile(fileName);
}


void AssetWatcher::Stop()
{
    //Quit();
}


} // namespace eg

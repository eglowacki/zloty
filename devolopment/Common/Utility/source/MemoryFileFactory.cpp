#pragma warning(push)
#pragma warning (disable : 4512)  // '' : assignment operator could not be generated
#include "File/MemoryFileFactory.h"
#include "Logger/Log.h"
#include <boost/shared_ptr.hpp>
#include <sstream>
#include <string>
#pragma warning(pop)



namespace eg {

MemoryFileFactory::MemoryFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& /*folders*/)
: mType(type)
, mPrority(prority)
{
    log_trace(tr_util) << "MemoryFile factory object created.";
}


MemoryFileFactory::~MemoryFileFactory()
{
    log_trace(tr_util) << "MemoryFile factory object deleted.";
}


VirtualFileFactory::istream_t MemoryFileFactory::GetFileStream(const std::string& name) const
{
    std::string fileName = boost::to_lower_copy(name);
    DataStream_t::const_iterator it = mStreamFiles.find(fileName);
    if (it != mStreamFiles.end())
    {
        return istream_t(new std::stringstream((*it).second->str()));
    }

    return istream_t();
}


VirtualFileFactory::ostream_t MemoryFileFactory::AttachFileStream(const std::string& name)
{
    std::string fileName = boost::to_lower_copy(name);
    mStreamFiles[fileName] = iostream_t(new std::stringstream());
    return mStreamFiles[fileName];
}


bool MemoryFileFactory::IsFileStreamExist(const std::string& name) const
{
    std::string fileName = boost::to_lower_copy(name);
    DataStream_t::const_iterator it = mStreamFiles.find(fileName);
    return it != mStreamFiles.end();
}


bool MemoryFileFactory::CanAttachFileStream(const std::string& /*name*/) const
{
    return true;
}

/*
bool MemoryFileFactory::WatchFileStream(const std::string& name, fFileStreamChanged fileStreamChanged)
{
    return false;
}


void MemoryFileFactory::UnwatchFileStream(const std::string& name)
{
}
*/

std::vector<std::string> MemoryFileFactory::GetFileList(const std::string& /*filter*/) const
{
    return std::vector<std::string>();
}

std::string MemoryFileFactory::GetFqn(const std::string& /*name*/) const
{
    return "";
}


} // namespace eg

#if 0
    #include "File/PackFileFactory.h"
    #include "Config/ConfigHelper.h"
    #include "Registrate.h"
    #include "StringHelper.h"
    #include <wx/filename.h>
    #include <wx/dir.h>
    #include <wx/regex.h>
    #include <wx/log.h>
    #include <fstream>
    #include <sstream>
    #include <vector>
    #include <sstream>
    #include <boost/scoped_ptr.hpp>
    #include <boost/shared_ptr.hpp>

namespace
{
    //! Return true, if there is no error, otherwise false and print to log
    bool CheckZipResult(ZRESULT result, const char *pFunctionName, const char *pFileName)
    {
        if (result != ZR_OK)
        {
            unsigned int size = FormatZipMessage(result, 0, 0);
            TCHAR *pBuff = new TCHAR[size];
            FormatZipMessage(result, pBuff, size);
            wxLogError("%s returned error: '%s' for file '%s'", pFunctionName, pBuff, pFileName ? pFileName : "");
            delete [] pBuff;
            return false;
        }

        return true;
    }

} // namespace

namespace eg
{

    PackFileFactory::PackFileFactory(const std::string& type, uint32_t prority, const std::vector<std::string>& folders)
    : mType(type)
    , mPrority(prority)
    {
        wxLogTrace(tr_util, "Created PackFileFactory object for Type: '%s' with Prority 0x%X.", type.c_str(), prority);

        std::string workingDir = registrate::GetExecutablePath();
        std::vector<std::string> registeredTypes;
        if (config::EnumerateGroup("VFS Factory/PackFile", registeredTypes))
        {
            std::vector<std::string>::const_iterator it = std::find(registeredTypes.begin(), registeredTypes.end(), mType);
            if (it != registeredTypes.end())
            {
                std::string factoryType = "VFS Factory/PackFile/" + *it;
                std::vector<std::string> packFiles = config::ReadStringArray(factoryType.c_str());
                for (std::vector<std::string>::const_iterator it_f = packFiles.begin(); it_f != packFiles.end(); ++it_f)
                {
                    // let's make entire path out of it
                    std::string packFileFullName = workingDir + "\\" + *it_f;
                    // std::string's find that file. Also, if it has wild card format, we'll get several ones
                    wxFileName fileObject(packFileFullName);
                    fileObject.MakeAbsolute();
                    wxArrayString files;
                    std::string packPath = fileObject.GetPath();
                    std::string packFilter = fileObject.GetFullName();
                    wxDir::GetAllFiles(packPath, &files, packFilter, wxDIR_FILES);

                    for (wxArrayString::const_iterator it_pack = files.begin(); it_pack != files.end(); ++it_pack)
                    {
                        if (HZIP packHandle = OpenZip((*it_pack).c_str(), 0))
                        {
                            mPackFileHandles.push_back(packHandle);
                            wxLogTrace(tr_util, "Registered pack '%s' for type %s.", (*it_pack).c_str(), (*it).c_str());
                        }
                    }
                }
            }
        }
    }


    PackFileFactory::~PackFileFactory()
    {
        for (std::vector<HZIP>::iterator it = mPackFileHandles.begin(); it != mPackFileHandles.end(); ++it)
        {
            ZRESULT result = CloseZip(*it);
            CheckZipResult(result, "CloseZip", 0);
        }

        wxLogTrace(tr_util, "Deleted.");
    }

    VirtualFileFactory::istream_t PackFileFactory::GetFileStream(const std::string& name) const
    {
        for (std::vector<HZIP>::const_iterator it = mPackFileHandles.begin(); it != mPackFileHandles.end(); ++it)
        {
            int index = -1;
            ZIPENTRY ze;
            ZRESULT result = FindZipItem(*it, name.c_str(), true, &index, &ze);
            // don't check for error here, since file might not exist in this pack
            if (result == ZR_OK)
            {
                uint8_t *pBuffer = new uint8_t[ze.unc_size];
                ZRESULT result = UnzipItem(*it, ze.index, pBuffer, ze.unc_size);
                if (CheckZipResult(result, "UnzipItem", name.c_str()))
                {
                    // we have valid memory buffer, now let's create istream from it
                    std::string fileData((char *)pBuffer, ze.unc_size);
                    delete [] pBuffer;
                    return istream_t(new std::istringstream(fileData));
                }
            }
        }

        return istream_t();
    }

    VirtualFileFactory::ostream_t PackFileFactory::AttachFileStream(const std::string& /*name*/)
    {
        return ostream_t();
    }


    bool PackFileFactory::IsFileStreamExist(const std::string& name) const
    {
        return GetFileStream(name) != istream_t();
    }


    bool PackFileFactory::CanAttachFileStream(const std::string& /*name*/) const
    {
        return false;
    }


    bool PackFileFactory::WatchFileStream(const std::string& /*name*/, fFileStreamChanged /*fileStreamChanged*/)
    {
        return false;
    }


    void PackFileFactory::UnwatchFileStream(const std::string& /*name*/)
    {
    }


    std::vector<std::string> PackFileFactory::GetFileList(const std::string& filter) const
    {
        std::vector<std::string> fileList;
        for (std::vector<HZIP>::const_iterator it = mPackFileHandles.begin(); it != mPackFileHandles.end(); ++it)
        {
            ZIPENTRY archiveEntry;
            // first, let's get number of zipeed files in this archive
            ZRESULT result = GetZipItem(*it, -1, &archiveEntry);
            if (CheckZipResult(result, "GetZipItem", filter.c_str()))
            {
                for (int i = 0; i < archiveEntry.index; i++)
                {
                    ZIPENTRY ze;
                    ZRESULT result = GetZipItem(*it, i, &ze);
                    if (CheckZipResult(result, "GetZipItem", filter.c_str()))
                    {
                        // le't see if this file matches our filter
                        if (WildCompare(filter.c_str(), ze.name))
                        {
                            fileList.push_back(ze.name);
                        }
                    }
                }
            }
        }

        return fileList;
    }


    std::string PackFileFactory::GetFqn(const std::string& /*name*/) const
    {
        return "";
    }


} // namespace eg
#endif // 0


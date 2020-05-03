///////////////////////////////////////////////////////////////////////
// ArchiveHelpers.h
//
//  Copyright 11/12/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper functions and objects to simplify with archiving
//
//
//  #include "File/ArchiveHelpers.h"
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef FILE_ARCHIVE_HELPERS_H
#define FILE_ARCHIVE_HELPERS_H
#pragma once

#include "File/VirtualFileSystem.h"


namespace eg
{
    /*!
    To save object to archive
    /code
        MyType myObject;
        VirtualFileFactory::ostream_t os = REGISTRATE(VirtualFileSystem).AttachFileStream("my_file.ext");
        SaveToArchive<boost::archive::text_oarchive>(myObject, *os);
    /endcode

    To load object from archive
    /code
        MyType myObject;
        VirtualFileFactory::istream_t is = REGISTRATE(VirtualFileSystem).GetFileStream("my_file.ext");
        LoadFromArchive<boost::archive::text_iarchive>(myObject, *is);
    /endcode
    */

    template <typename AR, typename T>
    bool SaveToArchive(const T& dataObject, std::ostream& stream);
    template <typename AR, typename T>
    bool LoadFromArchive(T& dataObject, std::istream& stream);


    //-------------------------------------------------------------------------------------
    template <typename AR, typename T>
    inline bool SaveToArchive(const T& dataObject, std::ostream& stream)
    {
        stream::SaveMarker saveMarker(stream);
        try
        {
            AR archive(stream);
            archive << dataObject;
        }
        catch (boost::archive::archive_exception& /*ex*/)
        {
            return false;
        }
        catch (std::exception& ex)
        {
            log_error << "Exception in SaveToArchive: " << ex.what();
            return false;
        }

        return true;
    }

    template <typename AR, typename T>
    inline bool LoadFromArchive(T& dataObject, std::istream& stream)
    {
        stream::LoadMarker loadMarker(stream);
        try
        {
            AR archive(stream);
            archive >> dataObject;
        }
        catch (boost::archive::archive_exception& ex)
        {
            log_error << "LoadFromArchive failed with archive_exception: " << ex.what();
            return false;
        }
        catch (std::exception& ex)
        {
            log_error << "Exception in LoadFromArchive: " << ex.what();
            return false;
        }

        return true;
    }

} // namespace eg

#endif // FILE_ARCHIVE_HELPERS_H


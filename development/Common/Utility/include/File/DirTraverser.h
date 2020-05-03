///////////////////////////////////////////////////////////////////////
// DirTraverser.h
//
//  Copyright 1/21/2006 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helper class to traverse files in a folder
//
//
//  #include <DirTraverser.h>
//
//////////////////////////////////////////////////////////////////////
//! \file

#ifndef DIR_TRAVERSER_H
#define DIR_TRAVERSER_H

#include <Base.h>
#include <wx/dir.h>


namespace eg
{
    /*!
    This class needs to be dervided ad
    */
    class DirTraverser : public wxDirTraverser
    {
    public:
        DirTraverser(const wxString& folderPath, const wxString& fileFilter) :
            mFolderPath(folderPath),
            mFileFilter(fileFilter)
        {
        }

        //! Kick in traversal of files. Return number of files traversed
        size_t Traverse()
        {
            size_t numEntries = 0;
            // open current dir for traversal
            wxDir dirrectoryObject;
            if (dirrectoryObject.Open(mFolderPath))
            {
                 numEntries = dirrectoryObject.Traverse(*this, mFileFilter);
            }

            Done(numEntries);
            return numEntries;
        }

        // from wxDirTraverser
        virtual wxDirTraverseResult OnDir(const wxString& dirname) {return wxDIR_IGNORE;}
        //! Provide this in your derived class
        //virtual wxDirTraverseResult OnFile(const wxString& filename) = 0;

    private:
        //! This is called after traversing all of the folder
        virtual void Done(size_t numEntries)
        {
        }

        wxString mFolderPath;
        wxString mFileFilter;
    };

} // namespace eg

#endif // DIR_TRAVERSER_H


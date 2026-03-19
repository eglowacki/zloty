// UNZIPPING functions -- for unzipping.
//
// This file is a repackaged form of extracts from the zlib code available at
// www.gzip.org/zlib, by Jean-Loup Gailly and Mark Adler.  The original
// copyright notice may be found in XUnzip.cpp.  The repackaging was done by
// Lucian Wischik to simplify and extend its use in Windows/C++.  Also
// encryption and unicode filenames have been added.
//
// e.g.
//
//   SetCurrentDirectory("c:\\docs\\stuff");
//   HZIP hz = OpenZip("c:\\stuff.zip",0);
//   ZIPENTRY ze; GetZipItem(hz,-1,&ze); int numitems=ze.index;
//   for (int i=0; i<numitems; i++)
//   { GetZipItem(hz,i,&ze);
//     UnzipItem(hz,i,ze.name);
//   }
//   CloseZip(hz);
//
//
//   HRSRC hrsrc = FindResource(hInstance,MAKEINTRESOURCE(1),RT_RCDATA);
//   HANDLE hglob = LoadResource(hInstance,hrsrc);
//   void *zipbuf=LockResource(hglob);
//   unsigned int ziplen=SizeofResource(hInstance,hrsrc);
//   HZIP hz = OpenZip(zipbuf, ziplen, 0);
//     - unzip to a membuffer -
//   ZIPENTRY ze; int i; FindZipItem(hz,"file.dat",true,&i,&ze);
//   char *ibuf = new char[ze.unc_size];
//   UnzipItem(hz,i, ibuf, ze.unc_size);
//   delete[] ibuf;
//
//   - unzip to a fixed membuff -
//   ZIPENTRY ze; int i; FindZipItem(hz,"file.dat",true,&i,&ze);
//   char ibuf[1024]; ZRESULT zr=ZR_MORE; unsigned long totsize=0;
//   while (zr==ZR_MORE)
//   { zr = UnzipItem(hz,i, ibuf,1024);
//     unsigned long bufsize=1024; if (zr==ZR_OK) bufsize=ze.unc_size-totsize;
//     totsize+=bufsize;
//   }
//
//   - unzip to a pipe -
//   HANDLE hwrite; HANDLE hthread=CreateWavReaderThread(&hwrite);
//   int i; ZIPENTRY ze; FindZipItem(hz,"sound.wav",true,&i,&ze);
//   UnzipItemHandle(hz,i, hwrite);
//   CloseHandle(hwrite);
//   WaitForSingleObject(hthread,INFINITE);
//   CloseHandle(hwrite); CloseHandle(hthread);
//     - finished -
//   CloseZip(hz);
//   // note: no need to free resources obtained through Find/Load/LockResource
//
//
//   SetCurrentDirectory("c:\\docs\\pipedzipstuff");
//   HANDLE hread,hwrite; CreatePipe(&hread,&hwrite,0,0);
//   CreateZipWriterThread(hwrite);
//   HZIP hz = OpenZipHandle(hread,0);
//   for (int i=0; ; i++)
//   { ZIPENTRY ze;
//     ZRESULT zr=GetZipItem(hz,i,&ze); if (zr!=ZR_OK) break; // no more
//     UnzipItem(hz,i, ze.name);
//   }
//   CloseZip(hz);

#ifndef ZIP_UTILS_XUNZIP_H_
#define ZIP_UTILS_XUNZIP_H_

#ifdef ZIP_STD
#include <cstdio>
#include <ctime>

using HZIP__ = struct HZIP__;
/**
 * @brief An HZIP identifies a zip file that is being created.
 */
using HZIP = struct HZIP__ *;

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

using TCHAR = char;
using HANDLE = FILE *;
using FILETIME = time_t;
using ZIP_FILETIME = FILETIME;
#else

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

using HZIP__ = struct HZIP__;
/**
 * @brief An HZIP identifies a zip file that is being created.
 */
using HZIP = struct HZIP__ *;

#ifdef _UNICODE
using TCHAR = wchar_t;
#else
using TCHAR = char;
#endif

using DWORD = unsigned long;
using HANDLE = void *;

/**
 * @brief Actually typedef for FILETIME on Windows.
 */
struct ZIP_FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;

#if defined(_FILETIME_)
  operator FILETIME &() { return *reinterpret_cast<FILETIME *>(this); }
#endif
};
#endif

#include <cstddef>  // size_t

#include "XZresult.h"

#if defined(_MSC_VER)
/*
 * @brief Exports functions, data, and objects from a DLL.
 *
 * "If a class is marked declspec(dllexport), any specializations of class
 * templates in the class hierarchy are implicitly marked as
 * declspec(dllexport).  This means that class templates are explicitly
 * instantiated and the class's members must be defined."
 *
 * See https://docs.microsoft.com/en-us/cpp/cpp/dllexport-dllimport
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED_EXPORT __declspec(dllexport)
/*
 * @brief Imports functions, data, and objects from a DLL.
 *
 * See https://docs.microsoft.com/en-us/cpp/cpp/dllexport-dllimport
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED_IMPORT __declspec(dllimport)
#elif defined(__clang__) || defined(__GNUC__)
/*
 * @brief Exports functions, data, and objects from a shared library.
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED_EXPORT __attribute__((visibility("default")))
/*
 * @brief Imports functions, data, and objects from a shared library.
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED_IMPORT
#endif

#ifdef ZU_UNZIP_BUILD_SHARED
#ifdef ZU_UNZIP_BUILD_ITSELF
/*
 * @brief Export shared objects.
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED ZU_UNZIP_ATTRIBUTE_SHARED_EXPORT
#else
/*
 * @brief Import shared objects.
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED ZU_UNZIP_ATTRIBUTE_SHARED_IMPORT
#endif
#else
/*
 * @brief Nothing for static library.
 */
#define ZU_UNZIP_ATTRIBUTE_SHARED
#endif

/**
 * @brief Zip archive entry.
 */
struct ZIPENTRY {
  int index;                         // index of this file within the zip
  TCHAR name[MAX_PATH];              // filename within the zip
  DWORD attr;                        // attributes, as in GetFileAttributes.
  ZIP_FILETIME atime, ctime, mtime;  // access, create, modify filetimes
  long comp_size;  // sizes of item, compressed and uncompressed. These
  long unc_size;   // may be -1 if not yet known (e.g. being streamed in)
};

// OpenZip - opens a zip file and returns a handle with which you can
// subsequently examine its contents.
//
// You can open a zip file from:
// from a pipe: OpenZipHandle(hpipe_read,0);
// from a file (by handle): OpenZipHandle(hfile,0);
// from a file (by name):   OpenZip("c:\\test.zip","password");
// from a memory block:     OpenZip(bufstart, buflen,0);
//
// If the file is opened through a pipe, then items may only be accessed in
// increasing order, and an item may only be unzipped once, although GetZipItem
// can be called immediately before and after unzipping it.  If it's opened in
// any other way, then full random access is possible.
//
// NOTE: pipe input is not yet implemented.
// NOTE: zip passwords are ascii, not unicode.
// NOTE: for windows-ce, you cannot close the handle until after CloseZip.  But
// for real windows, the zip makes its own copy of your handle, so you can close
// yours anytime.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] HZIP OpenZip(const TCHAR *fn,
                                                     const char *password);
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] HZIP OpenZip(void *z, unsigned int len,
                                                     const char *password);
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] HZIP OpenZipHandle(
    HANDLE h, const char *password);

// GetZipItem - call this to get information about an item in the zip.
//
// If index is -1 and the file wasn't opened through a pipe, then it returns
// information about the whole zipfile (and in particular ze.  Index returns the
// number of index items).
//
// NOTE: the item might be a directory (ze.attr & FILE_ATTRIBUTE_DIRECTORY).
// See below for notes on what happens when you unzip such an item.
// NOTE: if you are opening the zip through a pipe, then random access is not
// possible and GetZipItem(-1) fails and you can't discover the number of items
// except by calling GetZipItem on each one of them in turn, starting at 0,
// until eventually the call fails.  Also, in the event that you are opening
// through a pipe and the zip was itself created into a pipe, then then
// comp_size and sometimes unc_size as well may not be known until after the
// item has been unzipped.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT GetZipItem(HZIP hz, int index,
                                                           ZIPENTRY *ze);

// FindZipItem - finds an item by name.
//
// ic means 'insensitive to case'.  It returns the index of the item, and
// returns information about it.  If nothing was found, then index is set to -1
// and the function returns an error code.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT FindZipItem(HZIP hz,
                                                            const TCHAR *name,
                                                            bool ic, int *index,
                                                            ZIPENTRY *ze);

// UnzipItem - given an index to an item, unzips it.
//
// You can unzip to:
// to a pipe:             UnzipItemHandle(hz,i, hpipe_write);
// to a file (by handle): UnzipItemHandle(hz,i, hfile);
// to a file (by name):   UnzipItem(hz,i, ze.name);
// to a memory block:     UnzipItem(hz,i, buf,buflen);
//
// In the final case, if the buffer isn't large enough to hold it all, then the
// return code indicates that more is yet to come.  If it was large enough, and
// you want to know precisely how big, GetZipItem.
//
// NOTE: zip files are normally stored with relative pathnames.  If you unzip
// with ZIP_FILENAME a relative pathname then the item gets created relative to
// the current directory - it first ensures that all necessary subdirectories
// have been created.  Also, the item may itself be a directory.  If you unzip a
// directory with ZIP_FILENAME, then the directory gets created.  If you unzip
// it to a handle or a memory block, then nothing gets created and it emits 0
// bytes.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT UnzipItem(HZIP hz, int index,
                                                          const TCHAR *fn);
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT UnzipItem(HZIP hz, int index,
                                                          void *z,
                                                          unsigned int len);
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT UnzipItemHandle(HZIP hz,
                                                                int index,
                                                                HANDLE h);

// If unzipping to a filename, and it's a relative filename, then it will be
// relative to here.  (defaults to current-directory).
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT SetUnzipBaseDir(
    HZIP hz, const TCHAR *dir);

// Now we indulge in a little skullduggery so that the code works whether the
// user has included just zip or both zip and unzip.
//
// Idea: if header files for both zip and unzip are present, then presumably the
// cpp files for zip and unzip are both present, so we will call one or the
// other of them based on a dynamic choice.  If the header file for only one is
// present, then we will bind to that particular one.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] ZRESULT CloseZipU(HZIP hz);

// FormatZipMessage - given an error code, formats it as a string.
//
// It returns the length of the error message.  If buf/len points to a real
// buffer, then it also writes as much as possible into there.
ZU_UNZIP_ATTRIBUTE_SHARED size_t FormatZipMessageU(ZRESULT code, TCHAR *buf,
                                                   size_t len);

// IsZipHandleU - is handle unzip one.
ZU_UNZIP_ATTRIBUTE_SHARED [[nodiscard]] bool IsZipHandleU(HZIP hz);

#ifdef ZIP_UTILS_XZIP_H_
#undef CloseZip
// CloseZip - the zip handle must be closed with this function.
[[nodiscard]] inline ZRESULT CloseZip(HZIP hz) {
  return IsZipHandleU(hz) ? CloseZipU(hz) : CloseZipZ(hz);
}
#else
// CloseZip - the zip handle must be closed with this function.
#define CloseZip CloseZipU

// FormatZipMessage - given an error code, formats it as a string.
//
// It returns the length of the error message.  If buf/len points to a real
// buffer, then it also writes as much as possible into there.
#define FormatZipMessage FormatZipMessageU
#endif

#endif  // ZIP_UTILS_XUNZIP_H_

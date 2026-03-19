// Return codes from any of the zip functions.

#ifndef ZIP_UTILS_XZRESULT_H_
#define ZIP_UTILS_XZRESULT_H_

typedef unsigned long DWORD;

// return codes from any of the zip functions.  Listed later.
enum ZRESULT : DWORD {
  // success
  ZR_OK = 0x00000000,
  // nb. the pseudo-code zr-recent is never returned but can be passed to
  // FormatZipMessage.
  ZR_RECENT = 0x00000001,

  // The following come from general system stuff (e.g. files not openable)

  ZR_GENMASK = 0x0000FF00,
  // couldn't duplicate the handle
  ZR_NODUPH = 0x00000100,
  // couldn't create/open the file
  ZR_NOFILE = 0x00000200,
  // failed to allocate some resource
  ZR_NOALLOC = 0x00000300,
  // a general error writing to the file
  ZR_WRITE = 0x00000400,
  // couldn't find that file in the zip
  ZR_NOTFOUND = 0x00000500,
  // there's still more data to be unzipped
  ZR_MORE = 0x00000600,
  // the zipfile is corrupt or not a zipfile
  ZR_CORRUPT = 0x00000700,
  // a general error reading the file
  ZR_READ = 0x00000800,
  // we didn't get the right password to unzip the file
  ZR_PASSWORD = 0x00001000,
  // can't get current directory
  ZR_CWD = 0x00001100,
  // can't create directory
  ZR_MKDIR = 0x00001200,
  // can't set file times
  ZR_SETFTIME = 0x00001300,

  // The following come from mistakes on the part of the caller

  ZR_CALLERMASK = 0x00FF0000,
  // general mistake with the arguments
  ZR_ARGS = 0x00010000,
  // tried to ZipGetMemory, but that only works on mmap zipfiles, which yours
  // wasn't
  ZR_NOTMMAP = 0x00020000,
  // the memory size is too small
  ZR_MEMSIZE = 0x00030000,
  // the thing was already failed when you called this function
  ZR_FAILED = 0x00040000,
  // the zip creation has already been closed
  ZR_ENDED = 0x00050000,
  // the indicated input file size turned out mistaken
  ZR_MISSIZE = 0x00060000,
  // the file had already been partially unzipped
  ZR_PARTIALUNZ = 0x00070000,
  // tried to mix creating/opening a zip
  ZR_ZMODE = 0x00080000,

  // The following come from bugs within the zip library itself

  ZR_BUGMASK = 0xFF000000,
  // initialization didn't work
  ZR_NOTINITED = 0x01000000,
  // trying to seek in an unseekable file or seek failed
  ZR_SEEK = 0x02000000,
  // changed its mind on storage, but not allowed
  ZR_NOCHANGE = 0x04000000,
  // an internal error in the de/inflation code
  ZR_FLATE = 0x05000000
};

#endif  // ZIP_UTILS_XZRESULT_H_

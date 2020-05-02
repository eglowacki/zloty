/*////////////////////////////////////////////////////////////////////////////
 *  Project:
 *    Memory_and_Exception_Trace
 *
 * ///////////////////////////////////////////////////////////////////////////
 *  File:
 *    Stackwalker.h
 *
 *  Remarks:
 *
 *
 *  Note:
 *
 *
 *  Author:
 *    Jochen Kalmbach
 *
 * Additional Changes by:
 *    Edgar Glowacki
 *      * Removed xml output
 *      * Added switch to print to output debug window and file as an option
 *      * Reformat line number so it can be double clicked in Visual Studio panel
 *      * 1-17-2009
 *
 *////////////////////////////////////////////////////////////////////////////

#ifndef __STACKWALKER_H__
#define __STACKWALKER_H__

// Only valid in the following environment: Intel platform, MS VC++ 5/6/7/7.1/8
#ifndef _X86_
    #error Only INTEL envirnoments are supported!
#endif

// Only MS VC++ 5 to 7
#if (_MSC_VER < 1100) || (_MSC_VER > 1500)
    #error Only MS VC++ 5/6/7/7.1/8/9 supported. Check if the '_CrtMemBlockHeader' has not changed with this compiler!
#endif

typedef enum eAllocCheckOutput
{
    ACOutput_Simple,
    ACOutput_Advanced
};

#define YAGET_LEAK_DETECTOR_VERSION 1

// Make extern "C", so it will also work with normal C-Programs
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
    extern int InitAllocCheckWN(eAllocCheckOutput eOutput, LPCTSTR pszFilename, ULONG ulShowStackAtAlloc = 0);
    extern int InitAllocCheck(BOOL debugOutput, BOOL fileOutput, eAllocCheckOutput eOutput = ACOutput_Simple, BOOL bSetUnhandledExeptionFilter = TRUE, ULONG ulShowStackAtAlloc = 0);  // will create the filename by itself

    extern ULONG DeInitAllocCheck();

    extern DWORD StackwalkFilter(EXCEPTION_POINTERS *ep, DWORD status, LPCTSTR pszLogFile);

    extern void OnlyInstallUnhandeldExceptionFilter(eAllocCheckOutput eOutput = ACOutput_Simple);

#ifdef __cplusplus
}
#endif // __cplusplus


//  Additional Changes:
//    Edgar Glowacki (eg)
// Note: helper construct to initialize and de-initialize on program
// enter and exit respectively.
namespace eg
{
    namespace mem
    {
        struct LeakFinder
        {
            LeakFinder(bool debugOutput, bool fileOutput) {InitAllocCheck(debugOutput, fileOutput);}
            ~LeakFinder() {DeInitAllocCheck();}
        };

    } // mem
} // namespace eg

#endif  // __STACKWALKER_H__

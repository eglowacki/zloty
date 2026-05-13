//  ===========================================================================
//  File    Splash.h
//  Desc    The interface of the CSplash class
//  ===========================================================================
#pragma once

#include "Windows.h"
#include <string>

namespace yaget
{

//  ===========================================================================
//  Class   CSplash
//  Desc    Use it for displaying splash screen for applications
//          Works only on Win2000 , WinXP and later versions of Windows
//  ===========================================================================
class Splash
{
public:
    //  =======================================================================
    //  Func   CSplash
    //  Desc   Default constructor
    //  =======================================================================
    Splash();
    
    //  =======================================================================
    //  Func   CSplash
    //  Desc   Constructor
    //  Arg    Path of the Bitmap that will be show on the splash screen
    //  Arg    The color on the bitmap that will be made transparent
    //  =======================================================================
    Splash(const char* fileName, COLORREF colTrans);

    //  =======================================================================
    //  Func   ~CSplash
    //  Desc   Desctructor
    //  =======================================================================
    virtual ~Splash();

    //  =======================================================================
    //  Func   ShowSplash
    //  Desc   Launches the non-modal splash screen
    //  Ret    void 
    //  =======================================================================
    void ShowSplash();

    enum class TextLine
    {
        First,
        Second,
        Max
    };

    void Print(const char* message, TextLine line);

    //  =======================================================================
    //  Func   CloseSplash
    //  Desc   Closes the splash screen started with ShowSplash
    //  Ret    int 
    //  =======================================================================
    int CloseSplash();


    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    //  =======================================================================
    //  Func   SetTransparentColor
    //  Desc   This is used to make one of the color transparent
    //  Ret    1 if succesfull
    //  Arg    The colors RGB value. Not required if the color is specified 
    //         using the constructor
    //  =======================================================================
    bool SetTransparentColor(COLORREF col);

    //  =======================================================================
    //  Func   SetBitmap
    //  Desc   Call this with the path of the bitmap. Not required to be used
    //         when the construcutor with the image path has been used.
    //  Ret    1 if succesfull
    //  Arg    Either the file path or the handle to an already loaded bitmap
    //  =======================================================================
    DWORD SetBitmap(const char* lpszFileName);
    DWORD SetBitmap(HBITMAP hBitmap);

    void Init();
    void  OnPaint(HWND hwnd);
    bool MakeTransparent() const;
    HWND RegAndCreateWindow();
    void FreeResources();

    void PrintText(HDC hDC, const char* message, TextLine line);

    COLORREF mColorTransparance = 0;
    DWORD mWidth = 0;
    DWORD mHeight = 0;
    HBITMAP mBitmap = nullptr;
    LPCTSTR mClassName = nullptr;

    HWND mWindowHandle = nullptr;

    std::string mMessages[static_cast<size_t>(TextLine::Max)];

    HFONT mMessageFont = nullptr;
};


} // namespace yaget

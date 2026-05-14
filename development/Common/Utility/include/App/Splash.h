///////////////////////////////////////////////////////////////////////
// Splash.h
//
//  Copyright 5/13/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Use it for displaying splash screen for applications
//
//
//  #include "App/Splash.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Windows.h"
#include <string>

namespace yaget
{
    //--------------------------------------------------------------------------------------------------
    class Splash
    {
    public:
        Splash();
        Splash(const std::string& fileName, COLORREF colTrans, RECT monitorRect);
        ~Splash();

        enum class TextLine
        {
            First,
            Second,
            Max
        };

        void Print(const char* message, TextLine line);
        void ShowSplash();
        int CloseSplash();

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        // This is used to make one of the color transparent
        bool SetTransparentColor(COLORREF col);

        DWORD SetBitmap(const char* fileName);
        DWORD SetBitmap(HBITMAP hBitmap);

        void Init();
        void OnPaint(HWND hwnd);
        bool MakeTransparent() const;
        HWND RegAndCreateWindow();
        void FreeResources();

        void PrintText(HDC hDC, const char* message, TextLine line);

        RECT mMonitorRect;
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

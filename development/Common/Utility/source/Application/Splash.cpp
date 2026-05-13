//  ===========================================================================
//  File    Splash.cpp
//  Desc    The implementation file for the CSplash class.
#include "App/Splash.h"
#include "windowsx.h"
#include "Debugging/DevConfiguration.h"

//  ===========================================================================
//  The following is used for layering support which is used in the 
//  splash screen for transparency. In VC 6 these are not defined in the headers
//  for user32.dll and hence we use mechanisms so that it can work in VC 6.
//  We define the flags here and write code so that we can load the function
//  from User32.dll explicitely and use it. This code requires Win2k and above
//  to work.
//  ===========================================================================
using lpfnSetLayeredWindowAttributes = BOOL(WINAPI *)(HWND hWnd, COLORREF cr, BYTE bAlpha, DWORD dwFlags);

lpfnSetLayeredWindowAttributes g_pSetLayeredWindowAttributes;

#define WS_EX_LAYERED 0x00080000
//#define LWA_COLORKEY 1 // Use color as the transparency color.
//#define LWA_ALPHA    2 // Use bAlpha to determine the opacity of the layer


//--------------------------------------------------------------------------------------------------
static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static yaget::Splash* splashWindow = nullptr;
    if (uMsg == WM_CREATE)
    {
        splashWindow = static_cast<yaget::Splash*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
    }
    if (splashWindow)
    {
        return splashWindow->WindowProc(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


//--------------------------------------------------------------------------------------------------
LRESULT CALLBACK yaget::Splash::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //  We need to handle on the WM_PAINT message
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::Print(const char* message, TextLine line)
{
    mMessages[static_cast<size_t>(line)] = message;
    InvalidateRgn(mWindowHandle, nullptr, true);
    UpdateWindow(mWindowHandle);
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::OnPaint(HWND hwnd)
{
    if (!mBitmap)
    {
        return;
    }

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(mWindowHandle, &rect);

    HDC hMemDC = CreateCompatibleDC(hDC);
    auto hOldBmp = static_cast<HBITMAP>(SelectObject(hMemDC, mBitmap));

    BitBlt(hDC, 0, 0, mWidth, mHeight, hMemDC, 0, 0, SRCCOPY);

    for (auto i = 0; i < static_cast<size_t>(TextLine::Max); ++i)
    {
        const auto& message = mMessages[i];
        if (!message.empty())
        {
            PrintText(hDC, message.c_str(), static_cast<TextLine>(i));
        }
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);

    EndPaint(hwnd, &ps);
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::PrintText(HDC hMemDC, const char* message, TextLine line)
{
    LONG textBottomOffset = line == TextLine::First ? 50 : 10;
    LONG textHeight = 80;
    LONG textShadowOffset = 2;
    LONG textHorizontalOffset = 100;

    auto originalFont = static_cast<HFONT>(SelectObject(hMemDC, mMessageFont));

    SetTextColor(hMemDC, RGB(0, 0, 0));
    RECT textRect2{
        textHorizontalOffset + textShadowOffset, mHeight - (textBottomOffset + textHeight - textShadowOffset), mWidth - (textHorizontalOffset - textShadowOffset), mHeight - (textBottomOffset - textShadowOffset)
    };
    SetBkMode(hMemDC, TRANSPARENT);
    DrawText(hMemDC, message, -1, &textRect2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hMemDC, RGB(255, 0, 0));
    RECT textRect1{ textHorizontalOffset, mHeight - (textBottomOffset + textHeight), mWidth - textHorizontalOffset, mHeight - textBottomOffset };
    SetBkMode(hMemDC, TRANSPARENT);
    DrawText(hMemDC, message, -1, &textRect1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hMemDC, originalFont);
    SetBkMode(hMemDC, OPAQUE);
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::Init()
{
    mWindowHandle = nullptr;
    mClassName = TEXT("SPLASH");
    mColorTransparance = 0;
    mMessageFont = CreateFont(36, 20, 0, 0,FW_BOLD, false, false, false,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                              CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,TEXT("Consolas"));
    if (HMODULE hUser32 = GetModuleHandle(TEXT("USER32.DLL")))
    {
        g_pSetLayeredWindowAttributes = (lpfnSetLayeredWindowAttributes)GetProcAddress(hUser32, "SetLayeredWindowAttributes");
    }
}


//--------------------------------------------------------------------------------------------------
yaget::Splash::Splash()
{
    if (!dev::CurrentConfiguration().mDebug.mFlags.SuppressUI)
    {
        Init();
    }
}


//--------------------------------------------------------------------------------------------------
yaget::Splash::Splash(const std::string& fileName, COLORREF colTrans)
{
    if (!dev::CurrentConfiguration().mDebug.mFlags.SuppressUI)
    {
        Init();

        SetBitmap(fileName.c_str());
        SetTransparentColor(colTrans);
    }
}


//--------------------------------------------------------------------------------------------------
yaget::Splash::~Splash()
{
    FreeResources();
    if (mMessageFont)
    {
        DeleteObject(mMessageFont);
        mMessageFont = nullptr;
    }
}


//--------------------------------------------------------------------------------------------------
HWND yaget::Splash::RegAndCreateWindow()
{
    WNDCLASSEX wndclass;
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    wndclass.lpfnWndProc = ExtWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = DLGWINDOWEXTRA;
    wndclass.hInstance = GetModuleHandle(nullptr);
    wndclass.hIcon = nullptr;
    wndclass.hCursor = LoadCursor(nullptr, IDC_WAIT);
    wndclass.hbrBackground = static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
    wndclass.lpszMenuName = nullptr;
    wndclass.lpszClassName = mClassName;
    wndclass.hIconSm = nullptr;

    if (!RegisterClassEx(&wndclass))
    {
        return nullptr;
    }

    DWORD nScrWidth = GetSystemMetrics(SM_CXFULLSCREEN);
    DWORD nScrHeight = GetSystemMetrics(SM_CYFULLSCREEN);

    int x = (nScrWidth - mWidth) / 2;
    int y = (nScrHeight - mHeight) / 2;
    mWindowHandle = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, mClassName,
                                   TEXT("Banner"), WS_POPUP, x, y,
                                   mWidth, mHeight, nullptr, nullptr, nullptr, this);

    if (mWindowHandle)
    {
        MakeTransparent();
        ShowWindow(mWindowHandle, SW_SHOW);
        UpdateWindow(mWindowHandle);
    }
    return mWindowHandle;
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::ShowSplash()
{
    if (!dev::CurrentConfiguration().mDebug.mFlags.SuppressUI)
    {
        CloseSplash();
        RegAndCreateWindow();
    }
}


//--------------------------------------------------------------------------------------------------
DWORD yaget::Splash::SetBitmap(const char* fileName)
{
    auto hBitmap = static_cast<HBITMAP>(LoadImage(0, fileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
    return SetBitmap(hBitmap);
}


//--------------------------------------------------------------------------------------------------
DWORD yaget::Splash::SetBitmap(HBITMAP hBitmap)
{
    FreeResources();

    if (hBitmap)
    {
        mBitmap = hBitmap;
        BITMAP csBitmapSize;
        auto nRetValue = ::GetObject(hBitmap, sizeof(csBitmapSize), &csBitmapSize);
        if (nRetValue == 0)
        {
            FreeResources();
            return 0;
        }
        mWidth = static_cast<DWORD>(csBitmapSize.bmWidth);
        mHeight = static_cast<DWORD>(csBitmapSize.bmHeight);
    }

    return 1;
}


//--------------------------------------------------------------------------------------------------
void yaget::Splash::FreeResources()
{
    if (mBitmap)
    {
        DeleteObject(mBitmap);
        mBitmap = nullptr;
    }
}


//--------------------------------------------------------------------------------------------------
int yaget::Splash::CloseSplash()
{
    if (mWindowHandle)
    {
        DestroyWindow(mWindowHandle);
        mWindowHandle = nullptr;
        UnregisterClass(mClassName, ::GetModuleHandle(nullptr));
        return 1;
    }
    return 0;
}


//--------------------------------------------------------------------------------------------------
bool yaget::Splash::SetTransparentColor(COLORREF col)
{
    mColorTransparance = col;
    return MakeTransparent();
}


//--------------------------------------------------------------------------------------------------
bool yaget::Splash::MakeTransparent() const
{
    if (mWindowHandle && g_pSetLayeredWindowAttributes && mColorTransparance)
    {
        //  set layered style for the window
        SetWindowLong(mWindowHandle, GWL_EXSTYLE, GetWindowLong(mWindowHandle, GWL_EXSTYLE) | WS_EX_LAYERED);
        //  call it with 0 alpha for the given color
        g_pSetLayeredWindowAttributes(mWindowHandle, mColorTransparance, 0, LWA_COLORKEY);
    }
    return true;
}

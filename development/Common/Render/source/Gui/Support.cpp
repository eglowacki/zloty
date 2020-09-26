#include "Gui/Support.h"
#include "Device.h"
#include "App/Application.h"
#include "Platform/WindowsLean.h"
#include "Logger/YLog.h"
#include "Debugging/DevConfiguration.h"
#include "imgui.h"

#include "imgui/imgui_internal.h"

// so we do not have bring in header file for this functions
extern IMGUI_IMPL_API bool ImGui_ImplWin32_Init(void* hwnd);
extern IMGUI_IMPL_API bool ImGui_ImplDX11_Init(ID3D11Device* hardwareDevice, ID3D11DeviceContext* hardwareDeviceContext);
extern IMGUI_IMPL_API void ImGui_ImplDX11_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplWin32_Shutdown();
extern IMGUI_IMPL_API void ImGui_ImplWin32_NewFrame();
extern IMGUI_IMPL_API void ImGui_ImplDX11_NewFrame();
extern IMGUI_IMPL_API void ImGui_ImplDX11_RenderDrawData(ImDrawData* draw_data);
extern IMGUI_IMPL_API void ImGui_ImplDX11_InvalidateDeviceObjects();
extern IMGUI_IMPL_API bool ImGui_ImplDX11_CreateDeviceObjects();
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    std::mutex InputMutex;  // protect adding new input messages to imgui.input and imgui.render
    int DrawLoop = 0;       // only used in debugging to trigger assert on invalid DrawLoop counter, 
                            // to verify matching 

    // make sort/search case insensitive
    struct iSearch 
    {
        bool operator()(const std::string& a, const std::string& b) const 
        {
            return yaget::conv::ToLower(a) < yaget::conv::ToLower(b);
        }
    };
    // Loaded fonts available for rendering
    // Name refers to type and size
    std::map<std::string, ImFont*, iSearch> GuiFonts;
    
    //--------------------------------------------------------------------------------------------------
    void LoadFonts(yaget::io::VirtualTransportSystem& vts, const yaget::io::VirtualTransportSystem::Sections& fonts)
    {
        using namespace yaget;
        using Section = io::VirtualTransportSystem::Section;
        const int kDefaultFontSize = 12;

        GuiFonts.clear();
        ImGuiIO& io = ImGui::GetIO();

        ImFont* newFont = io.Fonts->AddFontDefault();
        GuiFonts.insert(std::make_pair("Default", newFont));

        ImFontConfig fontCfgTemplate = {};
        fontCfgTemplate.FontDataOwnedByAtlas = false;

        io::BLobLoader<io::render::FontFaceAsset> fontFaceLoader(vts, fonts);
        for (auto it : fontFaceLoader.Assets())
        {
            io::SingleBLobLoader<io::render::FontAsset> fontLoader(vts, it->mFont);
            auto fontData = fontLoader.GetAsset();
            const float reScaledFontSize = it->mSize * yaget::dev::CurrentConfiguration().mRuntime.DpiScaleFactor;

            newFont = io.Fonts->AddFontFromMemoryTTF(fontData->mBuffer.first.get(), static_cast<int>(fontData->mBuffer.second), reScaledFontSize, &fontCfgTemplate);
            GuiFonts.insert(std::make_pair(it->mTag.mName, newFont));
        }
    }

} // namespace


//--------------------------------------------------------------------------------------------------
yaget::gui::Reseter::Reseter(render::Device& device)
    : mDevice(device)
{
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        GuiFonts.clear();
        io.Fonts->Clear();
        ImGui_ImplDX11_InvalidateDeviceObjects();
    }
}


//--------------------------------------------------------------------------------------------------
yaget::gui::Reseter::~Reseter()
{
    using Sections = io::VirtualTransportSystem::Sections;

    if (ImGui::GetCurrentContext())
    {
        io::VirtualTransportSystem& vts = mDevice.App().VTS();

        LoadFonts(vts, mDevice.AvailableFonts());

        ImGui_ImplDX11_CreateDeviceObjects();
    }
}


//--------------------------------------------------------------------------------------------------
yaget::gui::Fonter::Fonter(const std::string& name)
    : mName(name)
{
    YAGET_ASSERT(ImGui::GetCurrentContext(), "Calling Fonter: '%s' while gui is not initialized.", name.c_str());

    if (auto it = GuiFonts.find(name); it != GuiFonts.end())
    {
        ImGui::PushFont(it->second);
    }
    else
    {
        mName = "";
        YLOG_ERROR("IGUI", "Gui Font: '%s' not installed.", name.c_str());
    }
}


//--------------------------------------------------------------------------------------------------
yaget::gui::Fonter::~Fonter()
{
    if (!mName.empty())
    {
        ImGui::PopFont();
    }
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::Initialize(render::Device& device)
{
    using Sections = io::VirtualTransportSystem::Sections;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    static std::string iniFilePath = util::ExpendEnv("$(LogFolder)/$(AppName)." + std::string(io.IniFilename), nullptr);
    static std::string logFilePath = util::ExpendEnv("$(LogFolder)/$(AppName)." + std::string(io.LogFilename), nullptr);

    io.IniFilename = iniFilePath.c_str();
    io.LogFilename = logFilePath.c_str();

    io::VirtualTransportSystem& vts = device.App().VTS();
    HWND winHandle = device.App().GetSurface().Handle<HWND>();

    LoadFonts(vts, device.AvailableFonts());

    ImGui_ImplWin32_Init(winHandle);
    ImGui_ImplDX11_Init(device.GetDevice(), device.GetDeviceContext());

    YLOG_NOTICE("IGUI", "ImGui initialized, version: '%s'. ini path: '%s', log path: '%s'.", ImGui::GetVersion(), io.IniFilename, io.LogFilename);
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::NewFrame()
{
    std::unique_lock<std::mutex> locker(InputMutex);
    DrawLoop++;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::Draw()
{
    std::unique_lock<std::mutex> locker(InputMutex);
    DrawLoop--;
    YAGET_ASSERT(DrawLoop == 0, "Gui mismatch beetwen NewFrame Draw calls. Current counter: '%d'.", DrawLoop);

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


//--------------------------------------------------------------------------------------------------
int64_t yaget::gui::ProcessInput(void* handle, uint32_t message, uint64_t param1, int64_t param2)
{
    std::unique_lock<std::mutex> locker(InputMutex);

    ImGui_ImplWin32_WndProcHandler(reinterpret_cast<HWND>(handle), message, param1, param2);
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::layout::CenterFrame(int width, int height)
{
    ImGuiIO& io = ImGui::GetIO();
    float left = (io.DisplaySize.x - width) / 2;
    float top = (io.DisplaySize.y - height) / 2;

    ImGui::SetNextWindowPos(ImVec2(left, top), 0);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)), 0);
}


//--------------------------------------------------------------------------------------------------
void yaget::gui::layout::CenterText(const std::string& text, float red, float green, float blue, float alpha)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImGui::SameLine((windowSize.x - textSize.x) / 2);

    yaget::gui::Text(text, math3d::Color{ red, green, blue, alpha });
}


bool yaget::gui::ColorEdit4(const std::string& label, math3d::Color& color)
{
    float adjustedColor[4] = { color.x, color.y, color.z, color.w};
    bool result = ImGui::ColorEdit4(label.c_str(), adjustedColor);
    color = math3d::Color(adjustedColor);

    return result;
}


void yaget::gui::MakeDisabled(bool disabled, std::function<void()> callback)
{
    if (disabled)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    callback();

    if (disabled)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}


void yaget::gui::SetTooltip(const std::string& tooltipText)
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(tooltipText.c_str());
    }

}


void yaget::gui::Text(const std::string& text, const colors::Color& textColor)
{
    ImGui::PushStyleColor(ImGuiCol_Text, textColor);
    ImGui::Text(text.c_str());
    ImGui::PopStyleColor();
}


void yaget::gui::Text(const std::string& text, const std::string& colorName)
{
    yaget::gui::Text(text, dev::CurrentConfiguration().mGuiColors.at(colorName));
}


void yaget::gui::SameLine()
{
    ImGui::SameLine();
}


void yaget::gui::NewLine()
{
    ImGui::NewLine();
}

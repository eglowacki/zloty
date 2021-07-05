#include "Render/UI/Layer.h"
#include "App/Display.h"
#include "imgui-docking/backends/imgui_impl_win32.h"

//-------------------------------------------------------------------------------------------------
yaget::render::ui::Layer::Resizer::Resizer(Layer& layer)
    : mLayer{layer}
{
    //ImGui_ImplDX12_InvalidateDeviceObjects();
}


//-------------------------------------------------------------------------------------------------
yaget::render::ui::Layer::Resizer::~Resizer()
{
    //ImGui_ImplDX12_CreateDeviceObjects();
}

//-------------------------------------------------------------------------------------------------
yaget::render::ui::Layer::Layer(const app::DisplaySurface& displaySurface)
{
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(displaySurface.Handle<void*>());
    //ImGui_ImplDX12_Init(g_pd3dDevice,
    //    NUM_FRAMES_IN_FLIGHT,
    //    DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
    //    g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
    //    g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}


//-------------------------------------------------------------------------------------------------
yaget::render::ui::Layer::~Layer()
{
    // Cleanup
    //ImGui_ImplDX12_Shutdown();
    //ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ui::Layer::Begin()
{
    //// Start the Dear ImGui frame
    //ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}


//-------------------------------------------------------------------------------------------------
void yaget::render::ui::Layer::End()
{
    ImGui::Render();
    //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);

    //barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    //barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    //g_pd3dCommandList->ResourceBarrier(1, &barrier);
    //g_pd3dCommandList->Close();
    //
    //g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);
    
    // Update and Render additional Platform Windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        //ImGui::RenderPlatformWindowsDefault(NULL, (void*)g_pd3dCommandList);
    }
}


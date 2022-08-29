#include "Render/DesktopApplication.h"
//#include "Render/Platform/Adapter.h"

yaget::render::DesktopApplication::DesktopApplication(const std::string& title, items::Director& director, io::VirtualTransportSystem& vts, const args::Options& options, const yaget::render::info::Adapter& selectedAdapter)
    : WindowApplication(title, director, vts, options)
    , mDevice(app::WindowFrame(*this), selectedAdapter)
{
    if (Input().IsAction("Quit App"))
    {
        Input().RegisterSimpleActionCallback("Quit App", [this]() { RequestQuit(); });
    }
}


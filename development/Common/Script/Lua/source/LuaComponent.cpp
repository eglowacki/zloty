#include "Scripting/LuaComponent.h"
#include "VTS/VirtualTransportSystem.h"
#include "HashUtilities.h"

#include <filesystem>
namespace fs = std::filesystem;


yaget::scripting::LuaComponent::LuaComponent(comp::Id_t id, const io::Tags& tags)
    : comp::Component(id)
    , mTags(tags)
{
}

yaget::scripting::LuaComponent::~LuaComponent()
{

}

size_t yaget::scripting::LuaComponent::CalculateStateHash() const
{
    return conv::GenerateHash(mTags);
}

yaget::io::Tags yaget::scripting::LuaComponent::ParseTags(const io::Tags& tags) const
{
    io::Tags parsedTags;
    // Strings sysPath = py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>());
    // auto sysSize = sysPath.size();

    // for (const auto& tag : tags)
    // {
        // fs::path scriptPath = fs::path(tag.ResolveVTS()).parent_path();

        // auto it = std::find_if(std::begin(sysPath), std::end(sysPath), [&scriptPath](const auto& elem)
        // {
            // return scriptPath == fs::path(elem);
        // });

        // if (it == std::end(sysPath))
        // {
            // sysPath.push_back(scriptPath.generic_string());
        // }

        // parsedTags.push_back(tag);
    // }

    // if (sysSize != sysPath.size())
    // {
        // py::module::import("sys").attr("path") = py::cast(sysPath);
    // }

    return parsedTags;
}

void yaget::scripting::LuaComponent::Update(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/)
{

}

void yaget::scripting::LuaComponent::InitializeScript(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mTags = ParseTags(mTags);

    for (const auto& tag : mTags)
    {
        // try
        // {
            // std::string scriptFile = fs::path(tag.ResolveVTS()).stem().generic_string();

            // Strings sysPathAfter = py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>());

            // py::module calc = py::module::import(scriptFile.c_str());
        // }
        // catch (const py::error_already_set& e)
        // {
            // std::string errorMessage = e.what();
            // YLOG_ERROR("SCRT", "Could not import Python file '%s'. %s. Search Path: '[%s]'.", yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str(), errorMessage.c_str(),
                // conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str());

            // continue;
        // }
    }
}

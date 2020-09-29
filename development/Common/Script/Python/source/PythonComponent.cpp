#include "Scripting/PythonComponent.h"
#include "VTS/VirtualTransportSystem.h"
#include "HashUtilities.h"
#include "Streams/Watcher.h"
#include "Scripting/Runtime.h"

#include <pybind11/embed.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include <filesystem>
namespace fs = std::filesystem;

constexpr static const char* InitializeFunctionName = "Initialize"; // void f(comp::Id_t id)
constexpr static const char* UpdateFunctionName = "Update";         // void f(comp::Id_t id, ...) - depends on the caller, since it has control of calling python method

yaget::scripting::PythonComponent::PythonComponent(comp::Id_t id, const io::Tags& tags, io::Watcher& watcher)
    : comp::Component(id)
    , mWatcher(watcher)
{
    mTags = InitializeScript(tags);
    YLOG_CERROR("SCRT", !mTags.empty(), "Script Component '%d' does not have any tags attached. It will do safely nothing but will consume resourcs.", Id());
}

yaget::scripting::PythonComponent::~PythonComponent()
{
    for (const auto& tag : mTags)
    {
        mWatcher.Remove(tag.Hash());
    }
}

size_t yaget::scripting::PythonComponent::CalculateStateHash() const
{
    return conv::GenerateHash(mTags);
}

yaget::io::Tags yaget::scripting::PythonComponent::ParseTags(const io::Tags& tags)
{
    io::Tags parsedTags;
    Strings sysPath = py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>());
    const auto sysSize = sysPath.size();

    for (const auto& tag : tags)
    {
        fs::path scriptPath = fs::path(tag.ResolveVTS()).parent_path();

        auto it = std::find_if(std::begin(sysPath), std::end(sysPath), [&scriptPath](const auto& elem)
        {
            return scriptPath == fs::path(elem);
        });

        if (it == std::end(sysPath))
        {
            sysPath.push_back(scriptPath.generic_string());
        }

        parsedTags.push_back(tag);
    }

    if (sysSize != sysPath.size())
    {
        py::module::import("sys").attr("path") = py::cast(sysPath);
    }

    return parsedTags;
}

yaget::io::Tags yaget::scripting::PythonComponent::InitializeScript(const io::Tags& tags)
{
    auto parsedTags = ParseTags(tags);

    std::lock_guard<std::mutex> locker(mScriptStateMutex);

    YLOG_CERROR("SCRT", mScriptStates.empty(), "Script Component '%d' did not initialized tags, since already has '%d' script states already allocated. [%s]", Id(), mScriptStates.size(), conv::Combine(mScriptStates, "], [").c_str());

    for (const auto& tag : parsedTags)
    {
        try
        {
            YAGET_ASSERT(mScriptStates.find(tag) == std::end(mScriptStates), "Script: '%s' is laready initialized.", yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str());

            // before we even attempt to load and initialize script, we want to make an entry
            // in mScriptStates, in case the initial load failed, but we still want to watch for file
            // changes to allow user to fix mistake
            ScriptState::Reload reload = ScriptState::Reload::Yes;
            mScriptStates[tag] = { ScriptState::Imported::No, reload, {} };

            mWatcher.Add(tag.Hash(), tag.ResolveVTS(), [this, tag]()
            {
                std::lock_guard<std::mutex> locker(mScriptStateMutex);

                if (auto it = mScriptStates.find(tag); it != std::end(mScriptStates))
                {
                    it->second.mReload = ScriptState::Reload::Yes;
                }

                mReloadScripts = true;
            });

            const std::string& scriptFile = fs::path(tag.ResolveVTS()).stem().generic_string();
            py::module importedModule = py::module::import(scriptFile.c_str());

            // reload Yes, means it will not execute this code in Update method
            // setting to No, it means that script is ready and valid to Update it.
            if (ValidateInitialize(importedModule, tag))
            {
                reload = ScriptState::Reload::No;
            }

            mScriptStates[tag] = { ScriptState::Imported::Yes, reload, std::make_unique<py::module>(importedModule) };
        }
        catch (const py::error_already_set& e)
        {
            std::string errorMessage = e.what();

            const py::object& t = e.type();
            const py::object& v = e.value();
            const py::object& tr = e.trace();

            YLOG_ERROR("SCRT", "Could not import Python file '%s'. %s. Search Path: '[%s]'.", yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str(), errorMessage.c_str(),
                conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str());

            continue;
        }
    }

    return parsedTags;
}

bool yaget::scripting::PythonComponent::ValidateInitialize(pybind11::module& importedModule, const io::Tag& tag)
{
    if (dev::CurrentConfiguration().mRuntime.mShowScriptHelp)
    {
        const auto message = scripting::PrintHelp(importedModule);
        YLOG_INFO("SCRT", "Module Help: \n%s", message.c_str());
    }

    if (!py::hasattr(importedModule, UpdateFunctionName))
    {
        YLOG_ERROR("SCRT", "Missing '%s' function implementation in script:\n%s\nHINT: Available functions to implement are: %s(componentId) and %s(...).", 
            UpdateFunctionName, tag.ResolveVTS().c_str(), InitializeFunctionName, UpdateFunctionName);
        return false;
    }

    if (py::hasattr(importedModule, InitializeFunctionName))
    {
        return RunScript(importedModule, tag, InitializeFunctionName, [id = Id()](pybind11::object& pyEntry, pybind11::module& /*pyModule*/)
        {
            pyEntry(id);
        });
    }

    return true;
}

void yaget::scripting::PythonComponent::ReloadScript()
{
    std::lock_guard<std::mutex> locker(mScriptStateMutex);

    for (auto& [tag, script] : mScriptStates)
    {
        if (script.mReload == ScriptState::Reload::Yes)
        {
            try
            {
                if (script.mModule)
                {
                    auto& m = *script.mModule.get();

                    if (py::hasattr(m, UpdateFunctionName))
                    {
                        py::delattr(m, UpdateFunctionName);
                    }
                    if (py::hasattr(m, InitializeFunctionName))
                    {
                        py::delattr(m, InitializeFunctionName);
                    }

                    script.mModule->reload();
                }
                else
                {
                    const std::string& scriptFile = fs::path(tag.ResolveVTS()).stem().generic_string();
                    py::module importedModule = py::module::import(scriptFile.c_str());

                    script.mModule = std::make_unique<py::module>(importedModule);
                    script.mImported = ScriptState::Imported::Yes;// reload, std::make_unique<py::module>(importedModule)
                }

                if (ValidateInitialize(*script.mModule, tag))
                {
                    script.mReload = ScriptState::Reload::No;
                }
            }
            catch (const py::error_already_set& e)
            {
                std::string errorMessage = e.what();

                const py::object& t = e.type();
                const py::object& v = e.value();
                const py::object& tr = e.trace();

                YLOG_ERROR("SCRT", "Could not reload Python file '%s'. %s. Search Path: '[%s]'.", yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str(), errorMessage.c_str(),
                    conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str());

                continue;
            }
        }
    }
}

void yaget::scripting::PythonComponent::Update(const time::GameClock& gameClock, metrics::Channel& channel, const Executor& executor)
{
    if (mReloadScripts)
    {
        mReloadScripts = false;
        ReloadScript();
    }

    std::lock_guard<std::mutex> locker(mScriptStateMutex);
    for (auto& [tag, script] : mScriptStates)
    {
        if (script.mReload == ScriptState::Reload::No)
        {
            RunScript(*script.mModule, tag, UpdateFunctionName, executor, &gameClock, &channel);
        }
    }
}

bool yaget::scripting::PythonComponent::RunScript(pybind11::module& importedModule, const io::Tag& tag, const char* functionName, const Executor& executor, const time::GameClock* /*gameClock = nullptr*/, metrics::Channel* /*channel = nullptr*/)
{
    try
    {
        py::object pyEntry = importedModule.attr(functionName);
        executor(std::ref(pyEntry), std::ref(importedModule));

        return true;
    }
    catch (const py::error_already_set& e)
    {
        const std::string errorMessage = e.what();
        YLOG_ERROR("SCRT", "Could not execute '%s' Python function in file '%s'. %s. [%s].\n%s",
            functionName,
            yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str(),
            errorMessage.c_str(),
            conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str(),
            tag.ResolveVTS().c_str());
    }
    catch (const py::builtin_exception& e)
    {
        const std::string errorMessage = e.what();
        YLOG_ERROR("SCRT", "Error in '%s' Python function in file '%s'. %s. [%s].\n%s",
            functionName,
            yaget::io::VirtualTransportSystem::Section(tag).ToString().c_str(),
            errorMessage.c_str(),
            conv::Combine(py::cast<Strings>(py::module::import("sys").attr("path").cast<py::list>()), "], [").c_str(),
            tag.ResolveVTS().c_str());
    }

    return false;
}

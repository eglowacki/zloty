//////////////////////////////////////////////////////////////////////
// PythonComponent.h
//
//  Copyright 4/22/2020 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Python specific implementation of script component to execute python
//      script/code. It supports hot reloads with some caveats:
//          functions names will stay, even if that function does not
//          exists in re-loaded file. This will also have same effect
//          on global variables.
//
//  #include "Scripting/PythonComponent.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "YagetCore.h"
#include "Components/Component.h"
#include "Streams/Buffers.h"
#include <string>


namespace pybind11 { class module; class object; }

namespace yaget
{
    namespace io { class Watcher; }
    namespace time { class GameClock; }
    namespace metrics { class Channel; }
    namespace scripting { class PythonComponent; }

    namespace comp::db
    {
        struct Scripts {};

        template <>
        struct ComponentProperties<scripting::PythonComponent>
        {
            using Row = std::tuple<Scripts>;
            using Types = std::tuple<io::Tags>;
            static Types DefaultRow() { return Types{}; };
        };

    }

    namespace scripting
    {
        namespace db
        {
            struct Scripts : io::Tags {};

        }

        class PythonComponent: public comp::Component
        {
        public:
            //constexpr static const char* InitializeFunctionName = "Initialize"; // void f(comp::Id_t id)
            //constexpr static const char* UpdateFunctionName = "Update";         // void f(comp::Id_t id, ...) - depends on the caller, since it has control of calling python method

            using ctor_params = std::tuple<io::Tags /*tags*/>;

            PythonComponent(comp::Id_t id, const io::Tags& tags, io::Watcher& watcher);
            virtual ~PythonComponent();

            using Executor = std::function<void(pybind11::object& pyEntry, pybind11::module& pyModule)>;
            void Update(const time::GameClock& gameClock, metrics::Channel& channel, const Executor& executor);

        private:
            size_t CalculateStateHash() const override;

            io::Tags ParseTags(const io::Tags& tags);
            io::Tags InitializeScript(const io::Tags& tags);
            void ReloadScript();

            bool RunScript(pybind11::module& importedModule, const io::Tag& tag, const char* functionName, const Executor& executor, const time::GameClock* gameClock = nullptr, metrics::Channel* channel = nullptr);
            bool ValidateInitialize(pybind11::module& importedModule, const io::Tag& tag);

            io::Tags mTags;

            // Bookkeeping of scripts, do we need to imported or reloaded
            struct ScriptState
            {
                enum class Imported { Yes, No };
                enum class Reload { Yes, No };

                Imported mImported;// = Imported::No;
                Reload mReload;// = Reload::No;
                std::unique_ptr<pybind11::module> mModule;

                std::string mInitializeFuncName{ "Initialize" };    // void f(comp::Id_t id)
                std::string mUpdateFuncName{ "Update" };            // void f(comp::Id_t id, ...) - depends on the caller, since it has control of calling python method
            };

            using ScriptStates = std::map<io::Tag, ScriptState>;
            ScriptStates mScriptStates;
            std::mutex mScriptStateMutex;
            io::Watcher& mWatcher;
            std::atomic_bool mReloadScripts{ false };
        };

    } // namespace scripting
} // namespace yaget


namespace std
{
    // Has function for PythonComponent
    template <>
    struct hash<yaget::scripting::PythonComponent>
    {
        size_t operator()(const yaget::scripting::PythonComponent& script) const
        {
            script;
            //const math3d::Vector3& loc = location.GetPosition();
            //const math3d::Quaternion& orient = location.GetOrientation();
            //const math3d::Vector3& scale = location.GetScale();
            return 0;//yaget::conv::GenerateHash(loc, orient, scale);
        }
    };

} // namespace std

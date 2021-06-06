//////////////////////////////////////////////////////////////////////
// GameWorldSystem.h
//
//  Copyright 7/14/2019 Edgar Glowacki
//
//  Maintained by: Edgar
//
//  NOTES:
//      Helpers and defines for ponger game systems and it's data
//
//
//  #include "Ponger/GameWorldSystem.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/Coordinator.h"
#include "Components/GameCoordinator.h"
#include "Components/GameSystem.h"
#include "Components/LocationComponent.h"
#include "Components/PhysicsComponent.h"
#include "Ponger/PongerComponents.h"
#include "Components/ScriptComponent.h"
#include "Scripting/PythonComponent.h"
#include "Scripting/Runtime.h"
#include "VTS/VirtualTransportSystem.h"


namespace yaget::items { class Director; }
namespace yaget { class IdGameCache; }
namespace yaget::io { class Watcher; }

namespace ponger
{
    // Helpful aliases for Ponger game
    // first setup what kind of items/entities we need for the game
    // in this case, we use two types, regular items in a scene and global components
    // under one unique id
    using GlobalEntity = comp::RowPolicy<comp::PhysicsWorldComponent*, scripting::PythonComponent*>;
    using Entity = comp::RowPolicy<comp::LocationComponent*, comp::PhysicsComponent*, ponger::DebugComponent*, comp::ScriptComponent*, scripting::PythonComponent*>;

    //The actual coordinator of our game which uses RowPolicy outlined above
    using GamePolicy = comp::CoordinatorPolicy<ponger::Entity, ponger::GlobalEntity>;

    // Our main UpdateWorld System, which drives physics
    using WorldUpdateSystem = comp::GameSystem<yaget::NoEndMarkerGlobal, comp::PhysicsWorldComponent*>;
    // Create physics system implementation for WorldUpdateSystem
    struct WorldUpdateSystemImpl
    {
        void operator()(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, comp::PhysicsWorldComponent* physWorld);
    };


    // Change colors for entities system
    using ColorizerSystem = comp::GameSystem<yaget::NoEndMarkerEntity, ponger::DebugComponent*>;
    // Create Colorizer System implementation for ColorizerSystem
    struct ColorizerSystemImpl
    {
        void operator()(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, ponger::DebugComponent* debugComponent);
    };
    
    struct ContextHolder
    {
        yaget::scripting::ContextHandle mScriptContext;
    };
    using ModuleHolder = std::shared_ptr<ContextHolder>;

    class PythonSystem : public comp::GameSystem<yaget::NoEndMarkerEntity, scripting::PythonComponent*, comp::LocationComponent*>
    {
        using BaseClass = comp::GameSystem<yaget::NoEndMarkerEntity, scripting::PythonComponent*, comp::LocationComponent*>;
        using Coordinator = comp::Coordinator<GamePolicy::Entity>;

    public:
        PythonSystem(ModuleHolder moduleHolder);

        template <typename T>
        void Initialize(const time::GameClock& gameClock, metrics::Channel& channel, comp::Coordinator<T>& coordinator)
        {
            BaseClass::Initialize<T>(gameClock, channel, coordinator);

            OnInitialize(gameClock, channel, coordinator);
        }

    private:
        void OnInitialize(const time::GameClock& gameClock, metrics::Channel& channel, Coordinator& coordinator);
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, scripting::PythonComponent* scriptComponent, comp::LocationComponent* locationComponent);

        ModuleHolder mModuleHolder;
    };

    //-------------------------------------------------------------------------------------------------
    class GameDirectorSystem : public comp::GameSystem<yaget::NoEndMarkerGlobal, scripting::PythonComponent*>
    {
        using BaseClass = comp::GameSystem<yaget::NoEndMarkerGlobal, scripting::PythonComponent*>;

    public:
        using set_coordinators = bool;

        using Section = io::VirtualTransportSystem::Section;

        using GlobalCoordinator = comp::Coordinator<GamePolicy::Global>;
        using EntityCoordinator = comp::Coordinator<GamePolicy::Entity>;

        GameDirectorSystem(ModuleHolder moduleHolder, items::Director& director, yaget::IdGameCache& idGameCache, yaget::io::VirtualTransportSystem& vts, yaget::io::Watcher& watcher, const Section& startupScript);

        void SetCoordinators(const time::GameClock& /*gameClock*/, metrics::Channel& /*channel*/, GlobalCoordinator& globalEntity, EntityCoordinator& entityCoordinators)
        {
            mGlobalEntity = &globalEntity;
            mEntityCoordinators = &entityCoordinators;
        }

        template <typename T>
        void Initialize(const time::GameClock& gameClock, metrics::Channel& channel)
        {
            BaseClass::Initialize<T>(gameClock, channel, GC());
            OnInitialize(gameClock, channel);
        }

        template <typename T>
        void Shutdown(const time::GameClock& gameClock, metrics::Channel& channel)
        {
            BaseClass::Shutdown<T>(gameClock, channel, GC());
            OnShutdown(gameClock, channel);
        }

    private:
        void OnInitialize(const time::GameClock& gameClock, metrics::Channel& channel);
        void OnShutdown(const time::GameClock& gameClock, metrics::Channel& channel);
        void OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, scripting::PythonComponent* scriptComponent);

        GlobalCoordinator& GC() const { YAGET_ASSERT(mGlobalEntity, "Calling Global Coordinator getter before this object was initialized."); return *mGlobalEntity; }
        EntityCoordinator& EC() const { YAGET_ASSERT(mGlobalEntity, "Calling Entity Coordinator getter before this object was initialized."); return *mEntityCoordinators; }

        items::Director& mDirector;
        yaget::IdGameCache& mIdGameCache;
        yaget::io::VirtualTransportSystem& mVTS;
        yaget::io::Watcher& mWatcher;
        ModuleHolder mModuleHolder;
        const io::Tags mStartupTags;

        // minimum needed global object managed by coordinator
        comp::Id_t mWorldId = comp::INVALID_ID;
        comp::Id_t mTestId = comp::INVALID_ID;


        GlobalCoordinator* mGlobalEntity = nullptr;
        EntityCoordinator* mEntityCoordinators = nullptr;
    };

} // namespace ponger

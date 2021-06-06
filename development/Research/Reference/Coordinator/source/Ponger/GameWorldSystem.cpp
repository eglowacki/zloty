#include "Ponger/GameWorldSystem.h"
#include "Ponger/PongerTypes.h"
#include "VTS/VirtualTransportSystem.h"
#include "IdGameCache.h"
#include "Scripting/PythonComponent.h"
#include "Scripting/Runtime.h"
#include "Items/ItemsDirector.h"
#include "Ponger/PongerComponents.h"
#include "Components/GameCoordinatorGenerator.h"
#include "sqlite/AccessHelpers.h"

#include <type_traits>
#include <utility>

#include <pybind11/stl.h>

namespace py = pybind11;


namespace
{
    void BindGameCoordinator(py::module& m, yaget::io::Watcher& watcher)
    {
        using namespace yaget;

        using GlobalCoordinator = ponger::GameDirectorSystem::GlobalCoordinator;
        using EntityCoordinator = ponger::GameDirectorSystem::EntityCoordinator;

        //comp::PhysicsWorldComponent
        py::class_<GlobalCoordinator>(m, "GlobalCoordinator")
            .def("AddScriptComponent", [&watcher](GlobalCoordinator& ec, comp::Id_t id) { return ec.AddComponent<scripting::PythonComponent>(id, std::vector<io::Tag>{}, watcher); }, py::return_value_policy::reference)
            .def("AddPhysicsWorldComponent", [](GlobalCoordinator& ec, comp::Id_t id) { return ec.AddComponent<comp::PhysicsWorldComponent>(id); }, py::return_value_policy::reference)
            //.def("AddDebugComponent", [&watcher](GlobalCoordinator& ec, comp::Id_t id) { return ec.AddComponent<ponger::DebugComponent>(id, io::Tag{}); }, py::return_value_policy::reference)
        ;

        py::class_<EntityCoordinator>(m, "EntityCoordinator")
            .def("AddScriptComponent", [&watcher](EntityCoordinator& ec, comp::Id_t id) { return ec.AddComponent<scripting::PythonComponent>(id, std::vector<io::Tag>{}, watcher); }, py::return_value_policy::reference)
            .def("AddLocationComponent", [](EntityCoordinator& ec, comp::Id_t id) { return ec.AddComponent<comp::LocationComponent>(id); }, py::return_value_policy::reference)
            .def("AddDebugComponent", [](EntityCoordinator& ec, comp::Id_t id) { return ec.AddComponent<ponger::DebugComponent>(id, io::Tag{}); }, py::return_value_policy::reference)
        ;

        py::enum_<ponger::GameDirectorSystem::FireTimer>(m, "FireTimer")
            .value("Once", ponger::GameDirectorSystem::FireTimer::Once)
            .value("Repeat", ponger::GameDirectorSystem::FireTimer::Repeat)
            .value("Stop", ponger::GameDirectorSystem::FireTimer::Stop)
        ;

        py::class_<items::Director>(m, "Director", py::dynamic_attr())
        ;
    }
} // namespace


YAGET_COMPILE_SUPPRESS_START(4100, "'': unreferenced local variable")
YAGET_COMPILE_SUPPRESS_START(4189, "'': local variable is initialized but not referenced")

void ponger::WorldUpdateSystemImpl::operator()(yaget::comp::Id_t /*id*/, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& channel, yaget::comp::PhysicsWorldComponent* physWorld)
{
    physWorld->Update(gameClock, channel);
}

void ponger::ColorizerSystemImpl::operator()(yaget::comp::Id_t /*id*/, const yaget::time::GameClock& gameClock, yaget::metrics::Channel& /*channel*/, ponger::DebugComponent* debugComponent)
{
    const float elapsedSeconds = time::FromTo<float>(gameClock.GetLogicTime(), time::kMicrosecondUnit, time::kSecondUnit) * 4.0f;

    const float interpolatedColor = (std::sin(elapsedSeconds) + 1.0f) * 0.5f;
    debugComponent->SetColor(math3d::Color(interpolatedColor, interpolatedColor, interpolatedColor, 1));
};

ponger::PythonSystem::PythonSystem(ModuleHolder moduleHolder)
    : BaseClass("PythonSystem", [this](auto&&... params) { OnUpdate(params...); })
    , mModuleHolder(std::move(moduleHolder))
{
}

void ponger::PythonSystem::OnInitialize(const time::GameClock& gameClock, metrics::Channel& channel, Coordinator& coordinator)
{
    if (!mModuleHolder->mScriptContext)
    {
        mModuleHolder->mScriptContext = scripting::CreateContext("Ponger");
    }
}

void ponger::PythonSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, scripting::PythonComponent* scriptComponent, comp::LocationComponent* locationComponent)
{
    auto executor = [id, &gameClock, scriptComponent, locationComponent](py::object& pyEntry, py::module& /*pyModule*/)
    {
        pyEntry(id, std::cref(gameClock), scriptComponent, locationComponent);
    };

    scriptComponent->Update(gameClock, channel, executor);
}

ponger::GameDirectorSystem::GameDirectorSystem(ModuleHolder moduleHolder, items::Director& director, yaget::IdGameCache& idGameCache, yaget::io::VirtualTransportSystem& vts, yaget::io::Watcher& watcher, const Section& startupScript)
    : BaseClass("PythonGameDirectorSystem", [this](auto&&... params) { OnUpdate(params...); })
    , mDirector(director)
    , mIdGameCache(idGameCache)
    , mVTS(vts)
    , mWatcher(watcher)
    , mModuleHolder(std::move(moduleHolder))
    , mStartupTags(vts.GetTags({ startupScript }))
{
    SQLite& db = mDirector.DB();

    auto preCacher = [this, &db]<typename T0>(const T0& element)
    {
        using BaseType = typename std::remove_pointer<typename std::decay<T0>::type>::type;

        comp::db::PreCacheInsertComponent<BaseType>(db);
    };

    meta::for_each_type<EntityCoordinator::Row>(preCacher);
    meta::for_each_type<GlobalCoordinator::Row>(preCacher);
}


void ponger::GameDirectorSystem::OnInitialize(const time::GameClock& gameClock, metrics::Channel& channel)
{
    using Section = io::VirtualTransportSystem::Section;

    YAGET_ASSERT(mWorldId == comp::INVALID_ID, "WorldId '%d' is already valid, it should be '%d'. Is this double initialization?", mWorldId, comp::INVALID_ID);

    if (!mModuleHolder->mScriptContext)
    {
        mModuleHolder->mScriptContext = scripting::CreateContext("Ponger");
    }

    // initialize all needed python bindings and expose supporting objects, functions
    const bool result = scripting::ExecuteInitialize(mModuleHolder->mScriptContext.get(), [this, &gameClock](py::module& m)
    {
        py::module idModule = m.def_submodule("idspace", "Encapsulates burnable and persistent id");
        idModule.def("BurnId", [this]() { return idspace::get_burnable(mIdGameCache); });
        idModule.def("PersistentId", [this]() { return idspace::get_persistent(mIdGameCache); });

        BindGameCoordinator(m, mWatcher);

        py::object pyEntityCoord = py::cast(std::ref(EC()), py::return_value_policy::reference);
        m.attr("EntityCoord") = pyEntityCoord;
        
        py::object pyGlobalCoord = py::cast(std::ref(GC()), py::return_value_policy::reference);
        m.attr("GlobalCoord") = pyGlobalCoord;

        py::module timerModule = m.def_submodule("timer", "Encapsulates timer functionality for controlling Updates");

        timerModule.def("Activate", [this, &gameClock](comp::Id_t id, time::Microsecond_t triggerDuration, FireTimer fireTimer)
        {
            ActivateTimer(id, gameClock, triggerDuration, fireTimer);
        });
        timerModule.def("Reset", [this, &gameClock](comp::Id_t id, time::Microsecond_t triggerDuration, FireTimer fireTimer)
        {
            ActivateTimer(id, gameClock, 0, FireTimer::Stop);
        });
        timerModule.def("Stop", [this, &gameClock](comp::Id_t id)
        {
            ActivateTimer(id, gameClock, time::INVALID_TIME, FireTimer::Stop);
        });

        py::object pyGameDirector = py::cast(std::ref(mDirector), py::return_value_policy::reference);
        pyGameDirector.doc() = "Specialized Game Director instance.";
        m.add_object("GameDirector", pyGameDirector);
    });

    if (result)
    {
        SQLite& db = mDirector.DB();

        //comp::db::PreCacheInsertComponent<comp::LocationComponent>(db);

        auto locationData = comp::db::ComponentRowTypes<comp::LocationComponent>::DefaultRow();
        std::get<0>(locationData) = 417;

        bool result417 = comp::db::InsertComponent<comp::LocationComponent>(db, locationData);

        //const auto& command = comp::db::ItemQuery<ItemRow>(firstOne);
        //auto entity = db.GetRow<Entity>(command);

        //YLOG_CERROR("PONG", entity.bValid, "Dod not find entity: '%d' in Ponger Item DB.", firstOne);

        //// we can iterate over Entity (bools) and for each true, use current 'index position' (how do we get that index) to get element from ItemRow. By matching this way
        //// we execute some arbitrary user (lambda) function to create the specific component
        //
        //
        //if (entity.Result)
        //{
        //    EC().AddComponent<comp::LocationComponent>(firstOne);
        //}
        //if (entity.Result1)
        //{
        //    EC().AddComponent<comp::PhysicsComponent>(firstOne, static_cast<btDiscreteDynamicsWorld*>(nullptr), comp::PhysicsComponent::Params{});
        //}
        //if (entity.Result2)
        //{
        //    // this tag should come from db table
        //    const io::Tag tag = mVTS.GetTag({ "Descriptions@Rectangle" });
        //    EC().AddComponent<ponger::DebugComponent>(firstOne, tag);
        //}
        //if (entity.Result3)
        //{
        //    // deprecated
        //    EC().AddComponent<comp::ScriptComponent>(firstOne, comp::ScriptComponent::ScriptFunction{});
        //}
        //if (entity.Result4)
        //{
        //    // this tag/tags should come from db table
        //    const auto tag = mVTS.GetTag({"Scripts@test"});

        //    EC().AddComponent<scripting::PythonComponent>(firstOne, io::Tags{ tag }, mWatcher);
        //}
    }
}

void ponger::GameDirectorSystem::OnUpdate(comp::Id_t id, const time::GameClock& gameClock, metrics::Channel& channel, scripting::PythonComponent* scriptComponent)
{
    scriptComponent->Update(gameClock, channel, [id, &gameClock, scriptComponent](py::object& pyEntry, py::module& /*pyModule*/)
    {
        pyEntry(id, std::cref(gameClock), scriptComponent);
    });
}

void ponger::GameDirectorSystem::OnShutdown(const time::GameClock& gameClock, metrics::Channel& channel)
{
    if (comp::IsIdValid(mWorldId))
    {
        GC().RemoveComponents(mWorldId);
        mWorldId = comp::INVALID_ID;
    }

    if (comp::IsIdValid(mTestId))
    {
        EC().RemoveComponents(mTestId);
    }
}

YAGET_COMPILE_SUPPRESS_END
YAGET_COMPILE_SUPPRESS_END

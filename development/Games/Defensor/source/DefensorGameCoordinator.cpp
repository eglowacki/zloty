#include "DefensorGameCoordinator.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Items/ItemsDirector.h"


namespace //Internal_DefensorGameCoordinator
{
    void SetupDirectorWithComponents(yaget::IdGameCache& idCache, defensor::game::DefensorSystemsCoordinator& systemsCoordinator)
    {
        using namespace yaget;

        const auto itemId0 = idspace::get_persistent(idCache);
        const auto itemId1 = idspace::get_persistent(idCache);
        const auto itemId2 = idspace::get_persistent(idCache);

        const auto locationComponent0 = systemsCoordinator.AddComponent<comp::LocationComponent3>(itemId0, math3d::Vector3{1, 2, 3}, math3d::Quaternion{4, 5, 6, 7}, math3d::Vector3{8, 9, 10});
        const auto unitComponent0 = systemsCoordinator.AddComponent<comp::UnitComponent>(itemId0, 100);
        const auto scriptComponent0 = systemsCoordinator.AddComponent<comp::ScriptComponent>(itemId0, "Init", comp::db_script::Section::Types{"FighterInit@Script/Fighters"});

        const auto locationComponent1= systemsCoordinator.AddComponent<comp::LocationComponent3>(itemId1, math3d::Vector3{11, 12, 13}, math3d::Quaternion{14, 15, 16, 17}, math3d::Vector3{18, 19, 110});
        const auto unitComponent1 = systemsCoordinator.AddComponent<comp::UnitComponent>(itemId1, 200);

        const auto locationComponent2 = systemsCoordinator.AddComponent<comp::LocationComponent3>(itemId2, math3d::Vector3{21, 22, 23}, math3d::Quaternion{24, 25, 26, 27}, math3d::Vector3{28, 29, 210});

        const auto result0 = systemsCoordinator.SaveComponent(locationComponent0);
        const auto result1 = systemsCoordinator.SaveComponent(unitComponent0);
        const auto result2 = systemsCoordinator.SaveComponent(scriptComponent0);

        const auto result11 = systemsCoordinator.SaveComponent(locationComponent1);
        const auto result12 = systemsCoordinator.SaveComponent(unitComponent1);

        const auto result21 = systemsCoordinator.SaveComponent(locationComponent2);

        const auto stageId = comp::GLOBAL_ID_MARKER;
        const auto stageComponent = systemsCoordinator.AddComponent<items::StageComponent>(stageId, "", items::db_stage::BlendOp::Replace);
        systemsCoordinator.SaveComponent(stageComponent);

        const auto text1 = fmt::format("\nitemId0: {}/{}\nitemId1: {}/{}\nitemId2: {}/{}\nstage:   {}/{}",
                                          itemId0, comp::ItemId(itemId0).ToString(),
                                          itemId1, comp::ItemId(itemId1).ToString(),
                                          itemId2, comp::ItemId(itemId2).ToString(),
                                          stageId, comp::ItemId(stageId).ToString());
        YLOG_INFO("DEF", text1.c_str());

        auto& director = systemsCoordinator.Director();
        director.AddStage("Boot");
        director.AddStage("Level 1");
        director.AddStage("Level 2");

        director.AddStageItem("Boot", itemId0);
        director.AddStageItems("Level 1", { itemId1, itemId2 });
    }

    //void AddDirectorWithComponents(yaget::IdGameCache& idCache, defensor::game::DefensorSystemsCoordinator& systemsCoordinator)
    //{
    //    using namespace yaget;

    //    const auto itemId0 = idspace::get_persistent(idCache);
    //    const auto stageComponent0 = systemsCoordinator.AddComponent<items::StageComponent>(itemId0, std::string{});
    //    const auto result0 = systemsCoordinator.SaveComponent(stageComponent0);

    //    const auto text1 = fmt::format("\nitemId0: {}/{}", itemId0, comp::ItemId(itemId0).ToString());
    //    YLOG_INFO("DEF", text1.c_str());
    //}

    void LoadDirector(defensor::game::DefensorSystemsCoordinator& systemsCoordinator, const yaget::comp::ItemIds& itemIds)
    {
        using namespace yaget;

        for (const auto id : itemIds)
        {
            /*const auto result =*/ systemsCoordinator.LoadItem(id);
        }
    }

}


//-------------------------------------------------------------------------------------------------
defensor::game::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, Application& app)
    : SystemsCoordinator(m, app)
{
    //// this should get loaded at start from some 'data' file. We can try to leverage Stages
    //// and having Stagger class to trigger which Stage to load
    //// Stagger.ExecuteStage(stageName|stageId)
    //// Stagger.PushStage(stageName|stageId)
    //// Stagger.PopStage();
    //auto tag = app.VTS().GetTag({"assets@GUI/MainMenu"});
    //auto mainMenuAsset = std::make_shared<io::StringAsset>(tag, io::CreateBuffer("main() {};"), app.VTS());

    enum class SetupDirector { Init, Add, Load };

    const auto directorStartup = app.Options.find<std::string>("director_startup", "load");

    const SetupDirector setupDirector = directorStartup == "init" ? SetupDirector::Init : (directorStartup == "add" ? SetupDirector::Add : SetupDirector::Load);
    YLOG_NOTICE("DEF", "Director startup: '%d' { Init(0), Add(1), Load(2) }", setupDirector);

    if (setupDirector == SetupDirector::Init)
    {
        SetupDirectorWithComponents(app.IdCache, *this);
        //const auto itemId0 = comp::MarkAsPersistent(1000);
        //const auto itemId1 = comp::MarkAsPersistent(1001);
        //const auto itemId2 = comp::MarkAsPersistent(1002);
        //const auto stageId = comp::GLOBAL_ID_MARKER;

    }
    else if (setupDirector == SetupDirector::Load)
    {
        constexpr auto stageId = comp::GLOBAL_ID_MARKER;

        const auto& startingStage = dev::CurrentConfiguration().mInit.mStartingStage;
        if (auto stageComponent = LoadComponent<items::StageComponent>(stageId))
        {
            auto observer = [stageComponent](auto oldValue, auto newValue)
            {
                int z = 0;
                z;
            };

            stageComponent->Connect<items::db_stage::Name>(observer);

            stageComponent->SetValue<items::db_stage::Name>(startingStage);
        }
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::game::DefensorSystemsCoordinator::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    SystemsCoordinator::Tick(gameClock, channel);
}

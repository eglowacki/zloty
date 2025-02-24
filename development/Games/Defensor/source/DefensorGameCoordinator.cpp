#include "DefensorGameCoordinator.h"

#include "Items/ItemsDirector.h"

//-------------------------------------------------------------------------------------------------
defensor::game::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, Application& app)
    : SystemsCoordinator(m, app)
{
    auto stageComponent = AddComponent<items::StageComponent>(comp::GLOBAL_ID_MARKER, "MainMenu");

    //// this should get loaded at start from some 'data' file. We can try to leverage Stages
    //// and having Stagger class to trigger which Stage to load
    //// Stagger.ExecuteStage(stageName|stageId)
    //// Stagger.PushStage(stageName|stageId)
    //// Stagger.PopStage();
    //auto tag = app.VTS().GetTag({"assets@GUI/MainMenu"});
    //auto mainMenuAsset = std::make_shared<io::StringAsset>(tag, io::CreateBuffer("main() {};"), app.VTS());
    //auto scriptComponent = AddComponent<comp::ScriptComponent>(comp::GLOBAL_ID_MARKER, "MainMenu", "assets@GUI/MainMenu", mainMenuAsset);
    //auto menuComponent = AddComponent<comp::MenuComponent>(comp::GLOBAL_ID_MARKER);
    //auto inputComponent = AddComponent<comp::InputComponent>(comp::GLOBAL_ID_MARKER, app.Input());
    //inputComponent->AddInputEvent("New Game", [menuComponent](){menuComponent->OnInput("New Game");});
    //inputComponent->AddInputEvent("Load Game", [menuComponent](){menuComponent->OnInput("Load Game");});
    //inputComponent->AddInputEvent("Save Game", [menuComponent](){menuComponent->OnInput("Save Game");});
    //inputComponent->AddInputEvent("Options", [menuComponent](){menuComponent->OnInput("Options");});
    //inputComponent->AddInputEvent("Foobar", [menuComponent](){menuComponent->OnInput("Foobar");});
    //inputComponent->AddInputEvent("Credits", [menuComponent](){menuComponent->OnInput("Credits");});
    //inputComponent->AddInputEvent("Quit", [&app](){app.RequestQuit();});

    //app.Director().SaveComponentState(scriptComponent);
    //app.Director().SaveComponentState(menuComponent);
    //app.Director().SaveComponentState(inputComponent);

    //using Row = std::tuple<comp::ScriptComponent*, comp::MenuComponent*, comp::InputComponent*>;
    //auto menuItem = app.Director().LoadItemState<Row>(comp::GLOBAL_ID_MARKER);

    //int z = 0;
    //z;

    ////template <typename Row>
    ////Row Director::LoadItemState(comp::Id_t id)

}

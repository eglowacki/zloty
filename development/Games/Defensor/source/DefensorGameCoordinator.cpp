#include "DefensorGameCoordinator.h"
#include "Script/ComponentBinder.h"
#include "StringHelpers.h"
#include "Script/MathBinder.h"
#include "Script/Executant.h"
#include "Components/ComponentsPatternAccess.h"


template<class F>
struct ArgType;

template<class T>
struct ArgType<std::function<void(T)>> 
{
    using type = T;
};

auto any_match(std::any a) 
{
    throw std::runtime_error("no handler for any_match");
}

template<class T, class... Ts>
auto any_match(std::any a, T&& handler, Ts&&... handlers) 
{
    using arg_type = typename ArgType<decltype(std::function(handler))>::type;

    if (auto ptr = std::any_cast<arg_type>(&a); ptr)
    {
        return handler(*ptr);
    }
    else
    {
        return any_match(a, std::forward<Ts>(handlers)...);
    }
}


//-------------------------------------------------------------------------------------------------
defensor::game::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, Application& app)
    : SystemsCoordinator(m, app)
{
    //Bar(BindConstructor<comp::VelocityComponent>());
    //"Persistance@PlayerTest"

    items::BindUserFunctions(*this);
    script::BindComponents<CoordinatorSet_t::FullRow, DefensorSystemsCoordinator>(mScriptState, "yaget", *this);
    script::BindMath(mScriptState, "yaget");

    const auto& itemsFile = dev::CurrentConfiguration().mInit.mItemsFile;
    if (!itemsFile.empty())
    {
        io::SingleBLobLoader<io::LuaAsset> fileLoader(app.VTS(), itemsFile);
        if (auto asset = fileLoader.GetAsset(); asset)
        {
            const auto& source = asset->mSource;
            script::Run(mScriptState, source, asset->mTag, script::Execute::Safe);

            auto yaget_lua_env = mScriptState["yaget"].get_or_create<sol::table>();

#if 0
            using SC = meta::strip_qualifiers_t<decltype(*this)>;

            using GlobalComponentsRow1 = comp::metatypes::global_components_row_t<SC>;
            auto globalItem = LoadItem<GlobalComponentsRow1>(4611686018427387903);

            using LocalComponentsRow1 = comp::metatypes::local_components_row_t<SC>;

            using GlobalComponentsRow2 = comp::metatypes::global_components_row_t<CoordinatorSet_t>;
            using LocalComponentsRow2 = comp::metatypes::local_components_row_t<CoordinatorSet_t>;
            
            GlobalComponentsRow1* globalComponentsRow1 = nullptr;
            LocalComponentsRow1* localComponentsRow1 = nullptr;
            GlobalComponentsRow2* globalComponentsRow2 = nullptr;
            LocalComponentsRow2* localComponentsRow2 = nullptr;

            auto& director = Director();

            auto* locationComponent3   = LoadComponent<comp::LocationComponent3>(4611686018427388904);
            auto* inputComponent          = LoadComponent<comp::InputComponent>(4611686018427388904);
            auto* unitComponent            = LoadComponent<comp::UnitComponent>(4611686018427388904);
            auto* scriptComponent         = LoadComponent<comp::ScriptComponent>(4611686018427388904);
            auto* nameComponent            = LoadComponent<comp::NameComponent>(4611686018427388904);
            auto* velocityComponent     = LoadComponent<comp::VelocityComponent>(4611686018427388904);
            auto* materialComponent     = LoadComponent<comp::MaterialComponent>(4611686018427388904);
            auto* bulletComponent         = LoadComponent<comp::BulletComponent>(4611686018427388904);
            auto* textComponent            = LoadComponent<comp::TextComponent>(4611686018427388904);
            auto* menuScreenComponent = LoadComponent<comp::MenuScreenComponent>(4611686018427388904);

            auto* menuComponent            = LoadComponent<comp::MenuComponent>(4611686018427387903);
            auto* stageComponent          = LoadComponent<items::StageComponent>(4611686018427387903);
#endif

            app.Input().PushContext("Edit");
        }
    }
    else
    {
        //constexpr auto stageId = comp::GLOBAL_ID_MARKER;
        //if (auto stageComponent = LoadComponent<items::StageComponent>(stageId))
        //{
        //    const auto& startingStage = dev::CurrentConfiguration().mInit.mStartingStage;
        //    if (!startingStage.empty())
        //    {
        //        stageComponent->SetValue<items::db_stage::Name>(startingStage);
        //    }

        //    app.Input().PushContext("Game");
        //}
        //else
        //{
        //    app.Input().PushContext("Edit");
        //}
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::game::DefensorSystemsCoordinator::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mMessaging.Process(Messaging::DispatcherType::Logic);

    SystemsCoordinator::Tick(gameClock, channel);
}

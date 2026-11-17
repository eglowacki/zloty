#include "DefensorGameCoordinator.h"
#include "Script/ComponentBinder.h"
#include "StringHelpers.h"
#include "Script/MathBinder.h"
#include "Script/Executant.h"
#include "Script/AppUtilitiesBinder.h"
#include "Components/ComponentsPatternAccess.h"

#include <ctime>



YAGET_COMPILE_SUPPRESS_START(4189, "local variable is initialized but not referenced")
YAGET_COMPILE_SUPPRESS_START(4996, "'asctime': This function or variable may be unsafe")

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


#if 0
    //MenuComponent, StageComponent, LocationComponent3, InputComponent, UnitComponent, ScriptComponent, NameComponent, VelocityComponent, MaterialComponent, BulletComponent, TextComponent, MenuScreenComponent, RenderPassComponent
    using FullRow = comp::metatypes::coordinator_set_t<DefensorSystemsCoordinator>::FullRow;
    constexpr size_t NumComponents = 4;//std::tuple_size_v<FullRow>;
    const auto& componentNames = GetCoordinatorSet().mComponentNames;

    using PatternBits = std::bitset<NumComponents>;
    PatternBits patternBits;
    patternBits.flip();
    auto maxPatterns = patternBits.to_ullong();

    using ForwardComponents = std::map<std::string, std::set<std::string>>;
    ForwardComponents forwardComponents;
    Strings patternArray;
    Strings tupleBitsList;
    Strings tupleUsing;
    for (size_t i = 1; i <= maxPatterns; ++i)
    {
        PatternBits bits{i};
        Strings argComponents;

        meta::for_loop<FullRow>([&bits, &argComponents, &forwardComponents]<std::size_t T0>()
        {
            constexpr std::size_t Index = T0;
            if (Index < NumComponents)
            {
                if (bits.test(Index))
                {
                    using ComponentType = meta::strip_qualifiers_t<std::tuple_element_t<Index, FullRow>>;

                    auto componentName = meta::type_name_v<ComponentType>();

                    componentName = conv::ReplaceString(componentName, "class", "");
                    componentName = conv::Trim(componentName);

                    const auto result = componentName.find_last_of("::");
                    if (result != std::string::npos)
                    {
                        std::string namespaceName = componentName.substr(0, result - 1);
                        std::string className = componentName.substr(result + 1);

                        forwardComponents[namespaceName].insert(className);
                    }


                    argComponents.push_back(componentName + "*");
                }
            }
        });

        if (!argComponents.empty())
        {
            auto bitsString = bits.to_string();
            auto tupleComponents = conv::Combine(argComponents, ", ");
            auto pattern = std::format("    using Bit{} = std::tuple<{}>;", bitsString, tupleComponents);
            patternArray.push_back(pattern);

            tupleBitsList.push_back(std::format("        Bit{}{{}}", bitsString));
            tupleUsing.push_back(std::format("Bit{}", bitsString));
        }
    }

    std::string preIncludeString = std::format("{}\n{}\n", "#include <any>", "#include <tuple>");

    std::string forwardDeclarations;
    for (const auto& [namespaceName, classNames] : forwardComponents)
    {
        forwardDeclarations += std::format("\nnamespace {}\n{{\n", namespaceName);

        for (const auto& className : classNames)
        {
            forwardDeclarations += std::format("    class {};\n", className);
        }

        forwardDeclarations += std::format("}}\n");
    }

    std::string usingBitsStatement = conv::Combine(patternArray, "\n");
    std::string usingTupleStatement = std::format("    inline std::any BitArray[] =\n    {{\n{}\n    }};", conv::Combine(tupleBitsList, ",\n"));

    std::string ifStatement;
    std::string ifClause = "";
    for (const auto& bitType : tupleUsing)
    {
        ifStatement += std::format("        {}if (auto ptr = std::any_cast<{}>(&bitArrayElement))\n        {{\n            callback.template operator()<{}>();\n        }}\n", ifClause, bitType, bitType);
        ifClause = "else ";
    }

    {
        std::filesystem::path p("c:/EdgarDev/zloty/development/Games/Defensor/source/generated/ComponentsPatternFile.cpp");
        auto [result, errorMsg] = io::file::AssureDirectories(p.generic_string());
        if (result)
        {
            std::time_t timeResult = std::time(nullptr);
            auto fileTimeStamp = std::asctime(std::localtime(&timeResult));

            auto versionNumber = comp::db::GenerateSystemsCoordinator<DefensorSystemsCoordinator, comp::db::GenerateCoordinator::Version>();

            std::string componentsPatternFile = std::format("// Auto Generated File on: {}{}{}\n\nnamespace yaget::comp::patterns\n{{\n    constexpr int64_t Version = {};\n\n{}\n\n{}\n\n\n    void FindValue(size_t index, auto callback)\n    {{\n        const auto& bitArrayElement = comp::patterns::BitArray[index];\n\n{}    }}\n}}", 
                fileTimeStamp,
                preIncludeString,
                forwardDeclarations,
                versionNumber,
                usingBitsStatement, 
                usingTupleStatement,
                ifStatement);

            std::ofstream patternFile(p.generic_string());
            patternFile << componentsPatternFile;
        }
        else
        {
            YLOG_ERROR("DEF", "%s", errorMsg.c_str());
        }

    }
#endif

    auto f = [this]<typename CompBits>()
    {
        CompBits cb{};
        LoadItem<CompBits>(1000);
        int z = 0;
        z;
    };

    comp::patterns::ResolveValue(4, f);


    items::BindUserFunctions(*this);
    script::BindMath(mScriptState, "yaget");
    script::BindComponents<CoordinatorSet_t::FullRow, DefensorSystemsCoordinator>(mScriptState, "yaget", *this);
    script::BindAppUtilities(mScriptState, "yaget", mApp);

    const auto& itemsFile = dev::CurrentConfiguration().mInit.mItemsFile;
    if (!itemsFile.empty())
    {
        io::SingleBLobLoader<io::LuaAsset> fileLoader(app.VTS(), itemsFile);
        if (auto asset = fileLoader.GetAsset(); asset)
        {
            const auto& source = asset->mSource;
            script::Run(mScriptState, source, asset->mTag, script::Execute::Safe);

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

YAGET_COMPILE_SUPPRESS_END
YAGET_COMPILE_SUPPRESS_END


//-------------------------------------------------------------------------------------------------
void defensor::game::DefensorSystemsCoordinator::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    mMessaging.Process(Messaging::DispatcherType::Logic);

    SystemsCoordinator::Tick(gameClock, channel);
}

#include "DefensorGameCoordinator.h"
#include "VTS/ToolVirtualTransportSystem.h"
#include "Items/ItemsDirector.h"
#include "StringHelpers.h"
#include <ranges>

namespace fs = std::filesystem;

namespace yaget::conv
{
    
    template<typename... P>
    struct Convertor<std::tuple<P...>>
    {
        using ValueT = std::tuple<P...>;
        static ValueT FromString(const char* value)
        {
            ValueT v{};

            const std::string text(value);
            const Strings tokens = conv::Split(text, ";", true);

            meta::for_loop<ValueT>([&tokens, &v]<std::size_t T0>()
            {
                constexpr std::size_t elementIndex = T0;
                using ElementType = std::tuple_element_t<elementIndex, ValueT>;

                ElementType paramValue = {};
                if (elementIndex < tokens.size())
                {
                    const auto& paramString = tokens[elementIndex];
                    paramValue = conv::Convertor<ElementType>::FromString(paramString.c_str());
                }


                std::get<elementIndex>(v) = paramValue;
            });

            return v;
        }
        static std::string ToString(const ValueT& value)
        {
            Strings stringValues;

            meta::for_loop<ValueT>([&stringValues, &value]<std::size_t T0>()
            {
                constexpr std::size_t elementIndex = T0;

                using ElementType = std::tuple_element_t<elementIndex, ValueT>;
                const auto& elementValue = std::get<elementIndex>(value);

                auto stringResult = Convertor<ElementType>::ToString(elementValue);
                stringValues.push_back(stringResult);
            });

            auto result = conv::Combine(stringValues, ";");
            return result;
        }
    };

}


namespace
{
    class PersistentReader
    {
    public:
        PersistentReader(const yaget::Strings& stringData, yaget::IdGameCache& idCache)
            : mVersion(FindValue<int>("Version", stringData))
            , mStringData(PruneLines(stringData))
            , mIdCache(idCache)

        {
            YAGET_ASSERT(mVersion && mVersion <= SUPPORTED_VERSION, "Version in persistent file: '%d' is not supported by this parser version: '%d'.", mVersion, SUPPORTED_VERSION);
            YLOG_INFO("GSYS", "PersistentReader supported Version: %d, Valid Tokens: [%s]. Valid Keys: [%s].", SUPPORTED_VERSION, yaget::conv::Combine(mValidTokens, ", ").c_str(), yaget::conv::Combine(mValidKeys, ", ").c_str());

            ParseKeys(mStringData);

            yaget::error_handlers::ThrowOnCheck(Validate(), "Failed validation for persistent data");
        }

        void PersistData(defensor::game::DefensorSystemsCoordinator& systemsCoordinator) const
        {
            auto& director = systemsCoordinator.Director();
            // add all stage names and generate id's for it
            for (const auto& stageName : mStages)
            {
                director.AddStage(stageName);
            }

            // save all components for each item
            for (const auto& [itemId, items] : mItems)
            {
                for (const auto& line : items)
                {
                    if (auto tokens = yaget::conv::Split(line, ":", true); tokens.size() == 2)
                    {
                        auto componentName = tokens[0];
                        const auto& params = tokens[1];

                        if (!systemsCoordinator.IsComponentTyped(componentName))
                        {
                            if (systemsCoordinator.IsComponentTyped(componentName) + "Component")
                            {
                                componentName = componentName + "Component";
                            }
                        }

                        systemsCoordinator.PersistComponent(itemId, componentName, params);
                    }
                }
            }

            // save items to Stages
            for (const auto& [stageName, items] : mStageItems)
            {
                director.AddStageItems(stageName, items);
            }
        }

    private:
        const int SUPPORTED_VERSION = 1;

        const yaget::Strings mValidTokens =
        {
            "//",
            "Version"
        };

        const yaget::Strings mValidKeys =
        {
            "New Item",
            "New Global Item",
            "Stages"
        };

        void ParseKeys(const yaget::Strings& stringLines)
        {
            enum class ParserState : uint8_t
            {
                NoOp,
                NewItem,
                Stage
            };

            ParserState parserState = ParserState::NoOp;
            yaget::comp::Id_t itemId = yaget::comp::INVALID_ID;

            for (const auto& line : stringLines)
            {
                if (line.starts_with(mValidKeys[0]))
                {
                    itemId = yaget::idspace::get_persistent(mIdCache);
                    parserState = ParserState::NewItem;
                    continue;
                }
                else if (line.starts_with(mValidKeys[1]))
                {
                    itemId = yaget::comp::GLOBAL_ID_MARKER;
                    parserState = ParserState::NewItem;
                    continue;
                }
                else if (line.starts_with(mValidKeys[2]))
                {
                    itemId = yaget::comp::INVALID_ID;
                    parserState = ParserState::Stage;
                    continue;
                }

                if (parserState == ParserState::NewItem)
                {
                    if (line.starts_with("StageName"))
                    {
                        const auto& value = GetValue(line);
                        mStageItems[value].insert(itemId);
                    }
                    else
                    {
                        mItems[itemId].push_back(line);
                    }
                }
                else if (parserState == ParserState::Stage)
                {
                    mStages.push_back(line);
                }
            }
        }

        bool Validate() const
        {
            // make sure that any usage of Stage Names by New Item (Components)
            // is already 'registered' in Stages.
            for (const auto& stageName : mStageItems | std::views::keys)
            {
                if (std::ranges::find(mStages, stageName) == mStages.end())
                {
                    YLOG_INFO("GSYS", "Stage '%s' requested by component does not exist in registered Stages: %s", stageName.c_str(), yaget::conv::Combine(mStages, ", ").c_str());
                    return false;
                }
            }

            return true;
        }

        template <typename T>
        T FindValue(const std::string& key, const yaget::Strings& stringData) const
        {
            auto result = FindValue(key, stringData);
            return yaget::conv::Convertor<T>::FromString(result.c_str());
        }

        std::string FindValue(const std::string& key, const yaget::Strings& stringData) const
        {
            std::string result;

            for (const auto& line : stringData)
            {
                if (line.starts_with(key))
                {
                    result = GetValue(line);
                    break;
                }
            }

            return result;
        }

        std::string GetValue(const std::string& line) const
        {
            std::string result;

            if (auto tokens = yaget::conv::Split(line, ":", true); tokens.size() == 2)
            {
                const auto& value = tokens[1];

                result = value;
            }

            return result;
        }

        yaget::Strings PruneLines(const yaget::Strings& lines)
        {
            yaget::Strings results = lines;

            auto it = std::ranges::remove_if(results, [this](const auto& line)
            {
                for (const auto& token : mValidTokens)
                {
                    if (line.starts_with(token))
                    {
                        return true;
                    }
                }

                return false;
            }).begin();

            results.erase(it, results.end());

            return results;
        }

        int mVersion;
        yaget::Strings mStringData;
        yaget::IdGameCache& mIdCache;

        yaget::Strings mStages;
        std::map<std::string, yaget::comp::ItemIds> mStageItems;
        std::map<yaget::comp::Id_t, yaget::Strings> mItems;
    };

}


//-------------------------------------------------------------------------------------------------
defensor::game::DefensorSystemsCoordinator::DefensorSystemsCoordinator(Messaging& m, Application& app)
    : SystemsCoordinator(m, app)
{
    const auto& itemsFile = dev::CurrentConfiguration().mInit.mItemsFile;
    if (!itemsFile.empty())
    {
        io::SingleBLobLoader<io::StringsAsset> fileLoader(app.VTS(), itemsFile);
        auto asset = fileLoader.GetAsset();
        const auto& lines = asset ? asset->mStrings : Strings{};

        try
        {
            PersistentReader persistentReader(lines, app.IdCache);
            persistentReader.PersistData(*this);
        }
        catch (const ex::bad_init& e)
        {
            YLOG_ERROR("GSYS", "PersistentReader '%s' failed: %s", itemsFile.c_str(), e.what());
        }

        auto& inputSystem = GetGameSystem<ProcessInputSystem>();
        inputSystem.SetContext("Edit");
        app.Input().PushContext("Edit");
    }
    else
    {
        constexpr auto stageId = comp::GLOBAL_ID_MARKER;
        auto stageComponent = LoadComponent<items::StageComponent>(stageId);

        const auto& startingStage = dev::CurrentConfiguration().mInit.mStartingStage;
        if (!startingStage.empty())
        {
            stageComponent->SetValue<items::db_stage::Name>(startingStage);
        }

        auto& inputSystem = GetGameSystem<ProcessInputSystem>();
        inputSystem.SetContext("Game");
        app.Input().PushContext("Game");
    }
}


//-------------------------------------------------------------------------------------------------
void defensor::game::DefensorSystemsCoordinator::Tick(const time::GameClock& gameClock, metrics::Channel& channel)
{
    SystemsCoordinator::Tick(gameClock, channel);
}

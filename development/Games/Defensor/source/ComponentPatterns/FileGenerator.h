///////////////////////////////////////////////////////////////////////
// FileGenerator.h
//
//  Copyright 11/17/2026 Edgar Glowacki.
//
//  Maintained by: Edgar
//
//  NOTES:
//      Used in Component Patterns file creation
//      
//      auto f = [this]<typename CompBits>()
//      {
//          LoadItem<CompBits>(1000);
//      };
//      
//      comp::patterns::ResolveValue(4, f);
//
//  #include "ComponentPatterns/FileGenerator.h"
//
//////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/GameCoordinatorGenerator.h"



namespace yaget::comp::patterns::generator
{
    //----------------------------------------------------------------------------------------------------------------------------------
    template <typename SC>
    void GenerateFile()
    {
        //MenuComponent, StageComponent, LocationComponent3, InputComponent, UnitComponent, ScriptComponent, NameComponent, VelocityComponent, MaterialComponent, BulletComponent, TextComponent, MenuScreenComponent, RenderPassComponent
        using FullRow = comp::metatypes::coordinator_set_t<SC>::FullRow;
        constexpr size_t NumComponents = 4;//std::tuple_size_v<FullRow>;

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
            PatternBits bits{ i };
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
YAGET_COMPILE_SUPPRESS_START(4996, "'asctime': This function or variable may be unsafe")
                std::time_t timeResult = std::time(nullptr);
                auto fileTimeStamp = std::asctime(std::localtime(&timeResult));
YAGET_COMPILE_SUPPRESS_END

                auto versionNumber = comp::db::GenerateSystemsCoordinator<SC, comp::db::GenerateCoordinator::Version>();

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
    }
    
}


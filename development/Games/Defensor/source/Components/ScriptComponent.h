
/////////////////////////////////////////////////////////////////////////
// ScriptComponent.h
//
//  Copyright 8/11/2024 Edgar Glowacki.
//
// NOTES:
//      
//
//
// #include "Components/ScriptComponent.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file
#pragma once

#include "Components/PersistentBaseComponent.h"
#include "VTS/ResolvedAssets.h"

namespace yaget::comp
{
    namespace db_script
    {
        struct Script { using Types = std::string; };
        struct Section { using Types = std::string;/*io::VirtualTransportSystem::Section;*/ };

        using ValueTypes = std::tuple<Script, Section>;

    } // namespace db_script


    class ScriptComponent : public db::PersistentBaseComponent<db_script::ValueTypes>
    {
    public:
        using Asset = std::shared_ptr<io::StringAsset>;

        ScriptComponent(Id_t id, const db_script::Script::Types& scriptType, const db_script::Section::Types& section, const Asset& script)
            : PersistentBaseComponent(id, std::tie(scriptType, section))
            , mScript(script)
        {
        }

        void Execute();

    private:
        Asset mScript;
    };

} // namespace yaget::comp

/////////////////////////////////////////////////////////////////////////
// ScriptAsset.h
//
//  Copyright 4/5/2009 Edgar Glowacki.
//
// NOTES:
//
//
// #include "Script/ScriptAsset.h"
//
/////////////////////////////////////////////////////////////////////////
//! \file


#ifndef SCRIPTASSET_H
#define SCRIPTASSET_H
#pragma once

#include "Asset/IAsset.h"


struct lua_State;

namespace eg
{
    class AssetLoadSystem;
    class IdGameCache;
    class Dispatcher;
    class InputManager;
    class PluginManager;
    namespace {class ScriptAssetFactory;}


    class ScriptAsset : public IAsset
    {
    public:
        static const guid_t kType = 0xb41068d4;

        ScriptAsset(uint64_t id, const std::string& name, lua_State *luaState);

        void Execute();

        // from IAsset
        virtual uint64_t GetId() const;
        virtual const std::string& Name() const;
        virtual const std::string& Fqn() const;
        virtual Locker_t Lock();
        virtual guid_t Type() const;
        virtual void HandleMessage(const Message &msg);

        static Token Register(AssetLoadSystem &als, Dispatcher& dsp, InputManager& input, PluginManager& pluginManager);
        typedef boost::function<void (lua_State * /*L*/)> Bind_t;
        static void RegisterBinds(Token token, Bind_t bind);

    private:
        enum State {sNone, sError, sLoaded, sCompiled};
        State mState;
        uint64_t mId;
        std::string mName;
        std::string mFqn;
        lua_State *mGlobalLua;
        lua_State *mLocalLua;
        std::string mSource;
        bool mAutoExecute;

        friend ScriptAssetFactory;
        void load(std::istream& stream);
        void save(std::ostream& stream) const;
        void compile();

        static const uint32_t version = 1;
    };

    typedef boost::shared_ptr<ScriptAsset> ScriptAsset_t;

    namespace script
    {
        template <typename T>
        inline T *get(lua_State *L, const std::string& name)
        {
            luabind::object g = luabind::globals(L);
            luabind::object lua_yaget = luabind::gettable(g, "yaget");
            T *obj = luabind::object_cast<T *>(lua_yaget[name]);
            return obj;
        }

        template <typename T>
        inline T& get_ref(lua_State *L, const std::string& name)
        {
            luabind::object g = luabind::globals(L);
            luabind::object lua_yaget = luabind::gettable(g, "yaget");
            T *obj = luabind::object_cast<T *>(lua_yaget[name]);
            if (!obj)
            {
                throw ex::program("get_ref dereferencing yaget lua pointer");
            }
            return *obj;
        }

    } // namespace script



} // namespace eg

#endif // SCRIPTASSET_H


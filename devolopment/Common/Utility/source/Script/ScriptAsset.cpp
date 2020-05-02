#include "Script/ScriptAsset.h"
#include "Script.h"
#include "MessageInterface.h"
#include "Message/Dispatcher.h"
#include "File/AssetLoadSystem.h"
#include "File/VirtualFileSystem.h"
#include "File/DiskFileFactory.h"
#include "File/MemoryFileFactory.h"
#include "Input/InputManager.h"
#include "IdGameCache.h"
#include "Plugin/PluginManager.h"
#include "StringHelper.h"
#include "File/XmlHelpers.h"
#include "Script/ScriptErrorHandler.h"
#include <boost/bind.hpp>
#include <luabind/luabind.hpp>

namespace luabind {

    //template<>
    //eg::ScriptAsset* get_pointer(boost::shared_ptr<eg::ScriptAsset>& p)
    eg::ScriptAsset* get_pointer(eg::ScriptAsset_t& p)
    {
        return p.get();
    }

    //template<class A>
    boost::shared_ptr<const eg::ScriptAsset>* get_const_holder(boost::shared_ptr<eg::ScriptAsset>*)
    {
        return 0;
    }
}

namespace lb = luabind;

namespace eg {

namespace script
{
    extern void init_dispatcher(lua_State * /*L*/);     // empty
    extern void init_app(lua_State * /*L*/);            // sleep
    extern void init_log(lua_State * /*L*/);            // trace, debug, warning, error
    extern void init_connection(lua_State * /*L*/);     // connection(id), connect = function
    extern void init_action(lua_State * /*L*/);         // action(name), connect = function
    extern void init_timer(lua_State * /*L*/);          // timer, connect = function, rate = number, repeat = boolean
    extern void init_math(lua_State * /*L*/);           // all math functions


    extern void AddLoader(lua_State * /*L*/, lua_CFunction /*loaderCallback*/);
} // namespace script



namespace
{

    bool compileScript(lua_State *L, const std::string& source, const std::string& /*name*/, const std::string& fileName)
    {
        try
        {
            int result = luaL_loadbuffer(L, source.c_str(), source.size(), fileName.c_str());
            if (result == 0)
            {
                // all is ok
                log_trace(tr_script) << "Script Source '" << fileName << "' compiled.";
                return true;
            }
            else if (result == LUA_ERRSYNTAX)
            {
                // there was lua script syntax
                std::string tempErrStr = lua_tostring(L, -1);
                lua_settop(L, 0);
                lua_pushstring(L, tempErrStr.c_str());
                script::luaError(L, false);
                lb::object error_msg(lb::from_stack(L, -1));
                lua_pop(L, 1);
                log_error << "Script Source '" << fileName << "' had syntax error while compiling. Errors:\n" << error_msg;
            }
            else if (result == LUA_ERRMEM)
            {
                // general memory error, this should never happen
                log_error << "Script Source '" << fileName << "' had memory error while compiling (WTF).";
            }
        }
        catch (ex::script_runtime& e)
        {
            log_error << "Script Source '" << fileName << "' exception while loading (lua state needs to be reset). Exception: " << e.what();
        }

        return false;
    }


    //! Any major errors in c++ while compiling
    //! or executing the code will be trapped here
    int LuaPanicError_Runtime(lua_State *L)
    {
        // I'm not sure if I can access even lua state here to extract an error string
        lb::object error_msg(lb::from_stack(L, -1));
        std::string errMsg = "Runtime: " + boost::lexical_cast<std::string>(error_msg);
        throw ex::script_runtime(errMsg.c_str());
    }

    class ScriptAssetFactory : public IAsset::Factory
    {
        ScriptAssetFactory(const ScriptAssetFactory&);
        ScriptAssetFactory& operator=(const ScriptAssetFactory&);

    public:
        ScriptAssetFactory(AssetLoadSystem& als, Dispatcher& dsp, InputManager& input, PluginManager& pluginManager)
        : mAls(als)
        , mDispatcher(dsp)
        , mPluginManager(pluginManager)
        , mLoadType(AssetLoadSystem::eLoadBlocking)
        , L(luaL_newstate())
        {
            lb::open(L);
            luaL_openlibs(L);
            lua_atpanic(L, LuaPanicError_Runtime);

            lb::module(L, "yaget")
            [
                lb::class_<ScriptAsset, ScriptAsset_t>("Script"),
                lb::class_<Dispatcher>("Dispatcher"),
                lb::class_<AssetLoadSystem>("AssetLoadSystem"),
                lb::class_<InputManager>("InputManager")
                    .def("loadBindings", &InputManager::LoadBindings),
                lb::class_<PluginManager>("PluginManager"),
                lb::class_<ScriptAssetFactory>("ScriptAssetFactory")
                    .def("load", &ScriptAssetFactory::loadModule)
                    .def("unload", &ScriptAssetFactory::unloadModule)
                    .def("add", &ScriptAssetFactory::addVfs)
                    .def("script", &ScriptAssetFactory::getScript)
            ];


            //lb::object lua_yaget = lb::newtable(L);
            //lb::object r = lb::registry(L);
            lb::object g = lb::globals(L);
            lb::object lua_yaget = lb::gettable(g, "yaget");

            lua_yaget["dsp"] = static_cast<Dispatcher *>(&mDispatcher);
            lua_yaget["input"] = static_cast<InputManager *>(&input);

            lua_yaget["module"] = static_cast<ScriptAssetFactory *>(this);
            lua_yaget["vfs"] = static_cast<ScriptAssetFactory *>(this);
            lua_yaget["als"] = static_cast<AssetLoadSystem *>(&mAls);;

            script::AddLoader(L, ScriptAssetFactory::onModuleRequest);
            script::init_log(L);
            script::init_app(L);
            script::init_dispatcher(L);
            script::init_connection(L);
            script::init_action(L);
            script::init_timer(L);
            script::init_math(L);

            mAls.RegisterFactory(ScriptAsset::kType, boost::bind(&ScriptAssetFactory::New, this, _1, _2),
                                                     boost::bind(&ScriptAssetFactory::Load, this, _1, _2, _3),
                                                     boost::bind(&ScriptAssetFactory::Save, this, _1, _2, _3),
                                                     boost::bind(&ScriptAssetFactory::Query, this, _1, _2));

            log_trace(tr_script) << "ScriptAssetFactory object created using " << LUA_RELEASE << " release.";
        }

        ~ScriptAssetFactory()
        {
            mAls.UnregisterFactory(ScriptAsset::kType);
            if (L)
            {
                lua_close(L);
            }
            log_trace(tr_script) << "ScriptAssetFactory object deleted.";
        }

        void registerBinds(ScriptAsset::Bind_t bind)
        {
            bind(L);
        }

    private:
        bool loadModule(const std::string& name)
        {
            return mPluginManager.ptr(name) != 0;
        }

        void unloadModule(const std::string& name)
        {
            lua_gc(L, LUA_GCCOLLECT, 0);
            mPluginManager.unload(name);
        }

        ScriptAsset_t getScript(const std::string& name) const
        {
            std::string normName = boost::algorithm::ierase_last_copy(name, ".lua");
            std::string fileOptionName = normName + ".lua.options";
            if (VirtualFileFactory::ostream_t options = mAls.vfs().AttachFileStream(fileOptionName))
            {
                (*options) << "<autoexecute>true</autoexecute>";
            }
            if (ScriptAsset_t asset = mAls.GetAsset<ScriptAsset>(normName, AssetLoadSystem::eLoadBlocking))
            {
                asset->mAutoExecute = true;
                asset->Execute();
                return asset;
            }

            return ScriptAsset_t();
        }

        void addVfs(const std::string& type, lb::object const& folders, const std::string& factory)
        {
            config::VirtualFileSetting setting;

            try
            {
                if (lb::type(folders) == LUA_TTABLE)
                {
                    for (lb::iterator i(folders), end; i != end; ++i)
                    {
                        std::string folder = lb::object_cast<std::string>(*i);
                        setting.folders.push_back(folder);
                    }
                }
                else
                {
                    std::string folder = lb::object_cast<std::string>(folders);
                    setting.folders.push_back(folder);
                }

                setting.type = type;
                if (boost::iequals(factory, "Disk"))
                {
                    config::AddVirtualFile<DiskFileFactory>(mAls.vfs(), setting);
                }
                else if (boost::iequals(factory, "Memory"))
                {
                    config::AddVirtualFile<MemoryFileFactory>(mAls.vfs(), setting);
                }
            }
            catch (lb::cast_failed& ex)
            {
                log_error << "Invalid folders parameter, type '" << type << "', factory '" << factory << "'. Exception: " << ex.what();
                luaL_error(L, "Invalid folders parameter, type '%s', factory '%s'. Exception: %s", type.c_str(), factory.c_str(), ex.what());
            }
        }

        static int onModuleRequest(lua_State *L)
        {
            std::string module = lua_tostring(L, 1);
            AssetLoadSystem& als = script::get_ref<AssetLoadSystem>(L, "als");
            VirtualFileSystem& vfs = als.vfs();

            std::string fileName = module + ".lua";
            if (VirtualFileFactory::istream_t stream = vfs.GetFileStream(fileName))
            {
                std::string source;
                (*stream) >> std::noskipws;
                std::copy(std::istream_iterator<char>(*stream), std::istream_iterator<char>(), std::back_inserter(source));

                std::string filePath = vfs.GetFqn(fileName);
                if (compileScript(L, source, module, filePath))
                {
                    return 1;
                }
            }
            else
            {
                log_error << "Could not load ScriptAsset '" << fileName << "' from FileSystem.";
            }

            return 0;
        }

        IAsset *New(const std::string& name, uint64_t asssetId)
        {
            IAsset *pAsset = new ScriptAsset(asssetId, name, L);
            return pAsset;
        }

        void Load(AssetLoadSystem::IAsset_t asset, VirtualFileFactory::istream_t stream, const std::string& name)
        {
            boost::shared_ptr<ScriptAsset> scriptAsset = boost::dynamic_pointer_cast<ScriptAsset>(asset);
            if (scriptAsset)
            {
                if (stream && stream::size(*stream))
                {
                    if (WildCompareISafe("*.lua.options", name))
                    {
                        // we have options let's process it
                        std::string options;
                        (*stream) >> options;
                        int z = 0;

                        //stream::stream_addapter<std::istream> astream(*stream);
                        //astream >> scriptAsset->mAutoExecute;
                    }
                    else if (WildCompareISafe("*.lua", name))
                    {
                        std::string fqn = mAls.vfs().GetFqn(name);
                        scriptAsset->mFqn = fqn;
                        scriptAsset->load(*stream);
                    }
                }
                else if (!stream)
                {
                    if (!scriptAsset->mSource.empty())
                    {
                        // based on what type a load scriptAsset
                        // async or blocking, we'll pick correct notification
                        // to compile and execute
                        if (mLoadType == AssetLoadSystem::eLoadBlocking)
                        {
                            scriptAsset->compile();
                            if (scriptAsset->mAutoExecute)
                            {
                                scriptAsset->Execute();
                            }
                        }
                        else
                        {
                            if (scriptAsset->mAutoExecute)
                            {
                                struct next_exec
                                {
                                    next_tick::Callback_t one;
                                    next_tick::Callback_t two;
                                    void operator()() const
                                    {
                                        one();
                                        two();
                                    }
                                };

                                next_exec exec;
                                exec.one = boost::bind(&ScriptAsset::compile, scriptAsset.get());
                                exec.two = boost::bind(&ScriptAsset::Execute, scriptAsset.get());
                                next_tick tick(mDispatcher, exec);
                            }
                            else
                            {
                                next_tick tick(mDispatcher, boost::bind(&ScriptAsset::compile, scriptAsset.get()));
                            }
                        }
                    }
                }
            }
        }

        void Save(AssetLoadSystem::IAsset_t asset, VirtualFileFactory::ostream_t stream, const std::string& name)
        {
            name;
            // we can catch ex::serialize exception here if needed.
            // AssetLoadSystem does this already.
            boost::shared_ptr<ScriptAsset> scriptAsset = boost::dynamic_pointer_cast<ScriptAsset>(asset);
            if (scriptAsset && stream)
            {
                if (WildCompareISafe("*.lua.options", name))
                {
                    stream::stream_addapter<std::ostream> astream(*stream);
                    astream << scriptAsset->mAutoExecute;
                }
                else if (WildCompareISafe("*.lua", name))
                {
                    scriptAsset->save(*stream);
                }
            }
        }

        std::vector<std::string> Query(AssetLoadSystem::IAsset_t asset, const VirtualFileSystem& vfs)
        {
            vfs;
            const std::string& name = asset->Name();
            // here we control the order of iteration over streams for usage in Load(...) and Save(...) methods
            std::vector<std::string> files;
            std::string file = name;
            file += ".lua.options";
            files.push_back(file);
            file = name;
            file += ".lua";
            files.push_back(file);
            return files;
        }

        AssetLoadSystem& mAls;
        Dispatcher& mDispatcher;
        PluginManager& mPluginManager;
        AssetLoadSystem::Load mLoadType;
        lua_State *L;
    };


} // namespace


IAsset::Token ScriptAsset::Register(AssetLoadSystem &als, Dispatcher& dsp, InputManager& input, PluginManager& pluginManager)
{
    Token factory(new ScriptAssetFactory(als, dsp, input, pluginManager));
    return factory;
}


void ScriptAsset::RegisterBinds(Token token, Bind_t bind)
{
    if (ScriptAssetFactory *factory = dynamic_cast<ScriptAssetFactory *>(token.get()))
    {
        factory->registerBinds(bind);
    }
    else
    {
        log_error << "Token used in RegisterBinds is not of ScriptAssetFactory type.";
    }

}

ScriptAsset::ScriptAsset(uint64_t id, const std::string& name, lua_State *luaState)
: mState(sNone)
, mId(id)
, mName(name)
, mFqn(name)
, mGlobalLua(luaState)
, mLocalLua(luaState)
, mAutoExecute(false)
{
}


uint64_t ScriptAsset::GetId() const
{
    return mId;
}


const std::string& ScriptAsset::Name() const
{
    return mName;
}


const std::string& ScriptAsset::Fqn() const
{
    return mFqn;
}


IAsset::Locker_t ScriptAsset::Lock()
{
    return Locker_t();
}


guid_t ScriptAsset::Type() const
{
    return ScriptAsset::kType;
}


void ScriptAsset::HandleMessage(const Message &msg)
{
    msg;
}


/*
void ScriptAsset::SetSource(const std::string& source)
{
    mSource = source;
}
*/


void ScriptAsset::save(std::ostream& stream) const
{
    log_trace(tr_script) << "Saving '" << Name() << "' <ScriptAsset> asset. version = " << ScriptAsset::version;

    stream << std::noskipws;
    stream << mSource;
}


void ScriptAsset::load(std::istream& stream)
{
    // this call could have originate from diffrent thread
    // so we do not want to mess with lua and only copy to memory
    mSource.clear();
    stream >> std::noskipws;
    std::copy(std::istream_iterator<char>(stream), std::istream_iterator<char>(), std::back_inserter(mSource));

    mState = sLoaded;
}


void ScriptAsset::compile()
{
    if (mState == sLoaded)
    {
        lua_pushstring(mLocalLua, Name().c_str());
        if (compileScript(mLocalLua, mSource, Name(), mFqn))
        {
            mState = sCompiled;
            lua_settable(mLocalLua, LUA_REGISTRYINDEX);
        }
        else
        {
            mState = sError;
        }
    }
    else
    {
        log_error << "Script Source '" << (Name() + ".lua") << "' was trying to compile but mState is " << mState << ", should be " << sLoaded;
    }
}


void ScriptAsset::Execute()
{
//  if (mState == sLoaded)
//  {
//      compile();
//  }

    if (mState == sCompiled)
    {
        std::string fileName = Name() + ".lua";
        try
        {
            lua_pushstring(mLocalLua, Name().c_str());
            lua_gettable(mLocalLua, LUA_REGISTRYINDEX);

            int base = lua_gettop(mLocalLua);

            lua_pushcfunction(mLocalLua, script::luaError);
            lua_insert(mLocalLua, base);

            int status = lua_pcall(mLocalLua, 0, 0, base);
            if (status == LUA_ERRRUN)
            {
                mState = sError;
                lb::object error_msg(lb::from_stack(mLocalLua, -1));
                log_error << "Script Source '" << fileName << "' had execution error:\n" << error_msg;
                lua_pop(mLocalLua, 1);
            }
            else if (status == LUA_ERRMEM)
            {
                mState = sError;
                script::stackDump(mLocalLua);
                log_error << "Script Source '" << fileName << "' had memory error while executing.";
            }
            else if (status == LUA_ERRERR)
            {
                mState = sError;
                script::stackDump(mLocalLua);
                log_error << "Script Source '" << fileName << "' 'error handler' while executing.";
            }
            /*
            else
            {
                luabind::object g = luabind::globals(mGlobalLua);
                luabind::object lua_yaget = luabind::gettable(g, "yaget");
                bool error = luabind::object_cast<bool>(lua_yaget);
                if (error)
                {
                    log_error << "Script Source '" <<  fileName << "' had inner script error.";
                    return;
                }
            }
            */

            lua_remove(mLocalLua, base);
        }
        catch (ex::script_runtime& e)
        {
            mState = sError;
            log_error << "Script '" << fileName << "' exception while executing (lua state needs to be reset). Exception: " << e.what();
        }
    }
}

} // namespace eg


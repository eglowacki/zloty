/////////////////////////////////////////////////////////////////////////
// lb_input.cpp
//
//  Copyright 4/11/2009 Edgar Glowacki.
//
// NOTES:
//      Provides bindings for functions in app namespace
//
//
//
/////////////////////////////////////////////////////////////////////////
//! \file
#include "Input/InputManager.h"
#include "MessageInterface.h"
#include "Script/ScriptErrorHandler.h"
#include "Logger/Log.h"
//#include "luacpp.h"
//#pragma warning(push)
//#pragma warning (disable : 4100)  // '' : unreferenced formal parameter
//#include <luabind/luabind.hpp>
//#pragma warning(pop)

using namespace eg;

namespace
{
    const int kNumTries = 5;

    typedef boost::function<void (const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/)> InputActionCallback_t;

    class LuaInputCallbackFunctor
    {
    public:
        LuaInputCallbackFunctor(const luabind::object &callback) : callback(callback), mErrorCounter(kNumTries)
        {}

        void operator()(const std::string& /*actionName*/, uint32_t /*timeStamp*/, int32_t /*mouseX*/, int32_t /*mouseY*/) const
        {
            try
            {
                luabind::call_function<void>(callback);
                mErrorCounter = kNumTries;
            }
            catch (luabind::error& e)
            {
                mErrorCounter--;
                lua_State* L = e.state();
                script::luaError(L, false);
                luabind::object error_msg(luabind::from_stack(L, -1));
                log_error << "Script runtime error, tries left(" << mErrorCounter << "):\n" << error_msg << ">";
                lua_pop(L, 1);
            }
        }

    private:
        luabind::object callback;
        mutable int mErrorCounter;
    };

    // Helper function that scripts can use to subscribe to event handlers
    void connect(const std::string& name, const luabind::object &callback)
    {
        luabind::object g = luabind::globals(callback.interpreter());
        luabind::object lua_yaget = luabind::gettable(g, "yaget");
        InputManager *input = luabind::object_cast<InputManager *>(lua_yaget["input"]);
        InputManager::ActionListener_t action = input->ListenForAction(name, LuaInputCallbackFunctor(callback));
    }

} // namespace

        //luabind::class_<Dispatcher::Channel_t>("signal_void"),
        //lb::class_<Dispatcher>("Dispatcher")
        //    .def("connect", &VirtualFileSystem::triggerFile)

namespace eg { namespace script {


void init_input(lua_State *L)
{
    using namespace luabind;

    module(L, "input")
    [
      def("connect", &connect)
    ];
}

}} // namespace script // namespace eg




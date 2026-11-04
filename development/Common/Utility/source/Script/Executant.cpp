#include "Script/Executant.h"
#include "Script/ScriptState.h"

#include <regex>


namespace
{
    //--------------------------------------------------------------------------------------------------
    int FindLineNumber(const std::string& textToCheck)
    {
        int lineNumber = 0;
        std::regex lineSection("\\]:[0-9]+:\\s+");
        std::regex numberSection("[0-9]+");

        std::smatch m;
        if (std::regex_search(textToCheck, m, lineSection))
        {
            for (const auto x : m)
            {
                std::string result = x;
                std::smatch numberMatch;
                if (std::regex_search(result, numberMatch, numberSection))
                {
                    lineNumber = yaget::conv::FromString<int>(numberMatch[0].str().c_str());
                    break;
                }
            }
        }

        return lineNumber;
    }


    //--------------------------------------------------------------------------------------------------
    struct LuaErrorHandler
    {
        LuaErrorHandler(const std::string& source, const yaget::io::Tag& tag)
            : mSource(source), mTag(tag)
        {
        }
        

        //--------------------------------------------------------------------------------------------------
        sol::protected_function_result operator()(lua_State* L, sol::protected_function_result result) const
        {
            using namespace yaget;

            sol::type t = sol::type_of(L, result.stack_index());
            std::string err = "sol: ";
            err += to_string(result.status());
            err += " error";

    #if SOL_IS_ON(SOL_EXCEPTIONS)
            std::exception_ptr eptr = std::current_exception();
            if (eptr)
            {
                err += " with a ";
                try
                {
                    std::rethrow_exception(eptr);
                }
                catch (const std::exception& ex)
                {
                    err += "std::exception -- ";
                    err.append(ex.what());
                }
                catch (const std::string& message)
                {
                    err += "thrown message -- ";
                    err.append(message);
                }
                catch (const char* message)
                {
                    err += "thrown message -- ";
                    err.append(message);
                }
                catch (...)
                {
                    err.append("thrown but unknown type, cannot serialize into error message");
                }
            }
    #endif // serialize exception information if possible

            if (t == sol::type::string)
            {
                err += ": ";
                sol::string_view serr = sol::stack::unqualified_get<sol::string_view>(L, result.stack_index());
                err.append(serr.data(), serr.size());
            }

            std::filesystem::path p(mTag.ResolveVTS());

            auto lineNumber = FindLineNumber(err);

            Strings sourceLines = conv::Split(mSource, "\r\n");
            std::string offendingSourceLine = lineNumber - 1 < sourceLines.size() ? sourceLines[lineNumber - 1] : "unknown source line";
            auto fileLine = std::format("\n{}({}) : error source line: '{}'.", p.make_preferred().string(), lineNumber, offendingSourceLine);
            err += fileLine;

            // replacing information of stack error into pfr
            int target = result.stack_index();
            if (result.pop_count() > 0)
            {
                sol::stack::remove(L, target, result.pop_count());
            }

            sol::stack::push(L, err);
            int top = lua_gettop(L);
            int towards = top - target;
            if (towards != 0)
            {
                lua_rotate(L, top, towards);
            }

            return result;
        }

        const std::string& mSource;
        const yaget::io::Tag& mTag;
    };


}


//--------------------------------------------------------------------------------------------------
bool yaget::script::Run(sol::state& luaState, const std::string& source, const io::Tag& tag, Execute execute)
{
    sol::protected_function_result result;

    if (execute == Execute::Safe)
    {
        result = luaState.safe_script(source, LuaErrorHandler{ source, tag });
    }
    else
    {
        result = luaState.script(source, LuaErrorHandler{ source, tag });
    }

    if (!result.valid())
    {
        sol::error err = result;
        std::string errorString = err.what();
        YLOG_ERROR("LUA", "Error occurred executing lua script: %s", errorString.c_str());
    }

    return result.valid();
}
